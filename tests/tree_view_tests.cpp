#include "test_framework.hpp"

#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/tree_view.hpp>

#include <string>

using namespace sasd::ui;

namespace {

class CountingTreeModel final : public TreeModel {
public:
    explicit CountingTreeModel(std::size_t root_count) : root_count_{root_count} {}

    [[nodiscard]] std::size_t childCount(const TreeNodePath& parent) const noexcept override {
        ++child_queries_;
        return parent.empty() ? root_count_ : 0;
    }

    [[nodiscard]] std::string_view textAt(const TreeNodePath& node) const override {
        ++text_queries_;
        text_ = "node-" + std::to_string(node.indices.front());
        return text_;
    }

    [[nodiscard]] std::size_t childQueries() const noexcept { return child_queries_; }
    [[nodiscard]] std::size_t textQueries() const noexcept { return text_queries_; }

private:
    std::size_t root_count_;
    mutable std::size_t child_queries_{0};
    mutable std::size_t text_queries_{0};
    mutable std::string text_;
};

} // namespace

TEST_CASE("TreeView materializes only a visible depth-first range") {
    StringTreeModel model({"A", "B"});
    const auto a = TreeNodePath{{0}};
    const auto child = model.append(a, "A-child");
    model.append(child, "A-grandchild");
    model.append({}, "C");

    TreeSelectionModel selection;
    selection.setModel(&model);
    TreeView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(std::nullopt, 5);
    CHECK(view.visibleRows().size() == 3);
    CHECK(view.visibleRows()[0].node == TreeNodePath{{0}});
    CHECK(view.visibleRows()[1].node == TreeNodePath{{1}});
    CHECK(view.visibleRows()[2].node == TreeNodePath{{2}});

    CHECK(view.expand(TreeNodePath{{0}}));
    const auto rows = view.visibleRows();
    CHECK(rows.size() == 4);
    CHECK(rows[1].node == TreeNodePath{{0, 0}});
    CHECK(rows[1].depth == 2);
}

TEST_CASE("TreeView uses an anchor without traversing a huge invisible sibling range") {
    CountingTreeModel model(1'000'000);
    TreeView view;
    view.setModel(&model);
    view.setViewport(TreeNodePath{{999'999}}, 2);

    const auto rows = view.visibleRows();
    CHECK(rows.size() == 1);
    CHECK(rows.front().node == TreeNodePath{{999'999}});
    CHECK(model.textQueries() == 1);
    CHECK(model.childQueries() < 12);
}

TEST_CASE("TreeView expansion and collapse normalize hidden descendant selection") {
    StringTreeModel model({"parent"});
    const auto parent = TreeNodePath{{0}};
    const auto child = model.append(parent, "child");
    TreeSelectionModel selection;
    selection.setModel(&model);
    TreeView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);

    CHECK(view.expand(parent));
    CHECK(selection.select(child));
    view.setViewport(std::nullopt, 1);
    CHECK(view.collapse(parent));
    CHECK(selection.selectedNode() == std::optional<TreeNodePath>{parent});
    CHECK(view.visibleRows().size() == 1);
}

TEST_CASE("TreeView keyboard navigation follows visible depth-first order") {
    StringTreeModel model({"parent", "sibling"});
    const auto parent = TreeNodePath{{0}};
    const auto child = model.append(parent, "child");
    TreeSelectionModel selection;
    selection.setModel(&model);
    TreeView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(std::nullopt, 2);
    view.arrange({0, 0, 20, 2});
    FocusManager focus;
    CHECK(focus.requestFocus(view));

    CHECK(view.handleEvent(KeyEvent{Key::down, true, KeyModifier::none}) == EventResult::handled);
    CHECK(selection.selectedNode() == std::optional<TreeNodePath>{parent});
    CHECK(view.handleEvent(KeyEvent{Key::right, true, KeyModifier::none}) == EventResult::handled);
    CHECK(view.isExpanded(parent));
    CHECK(view.handleEvent(KeyEvent{Key::right, true, KeyModifier::none}) == EventResult::handled);
    CHECK(selection.selectedNode() == std::optional<TreeNodePath>{child});
    CHECK(view.handleEvent(KeyEvent{Key::left, true, KeyModifier::none}) == EventResult::handled);
    CHECK(selection.selectedNode() == std::optional<TreeNodePath>{parent});
}

TEST_CASE("TreeView clears expansion and visible anchors on model reset") {
    StringTreeModel model({"parent"});
    const auto parent = TreeNodePath{{0}};
    model.append(parent, "child");
    TreeView view;
    view.setModel(&model);
    CHECK(view.expand(parent));
    view.setViewport(parent, 2);
    model.reset({"new"});
    CHECK(!view.isExpanded(parent));
    CHECK(!view.firstVisibleNode().has_value());
    CHECK(view.visibleRows().size() == 1);
}

TEST_CASE("TreeView rejects an anchor hidden by a collapsed ancestor") {
    StringTreeModel model({"parent"});
    const auto parent = TreeNodePath{{0}};
    const auto child = model.append(parent, "child");
    TreeView view;
    view.setModel(&model);
    view.setViewport(child, 1);

    CHECK(!view.firstVisibleNode().has_value());
    CHECK(view.visibleRows().size() == 1);
    CHECK(view.visibleRows().front().node == parent);
}
