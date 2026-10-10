#include "test_framework.hpp"

#include <sasd/ui/terminal/tree_view_presentation.hpp>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal TreeView owns visible rows and draws portable indicators") {
    StringTreeModel model({"Parent"});
    const auto parent = TreeNodePath{{0}};
    model.append(parent, "日本語");
    TreeSelectionModel selection;
    selection.setModel(&model);
    CHECK(selection.select(parent));
    TreeView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    CHECK(view.expand(parent));
    view.setViewport(std::nullopt, 2);

    const auto presentation = TerminalTreeViewPresentation::snapshot(view, {2, 3, 24, 2});
    CHECK(presentation.has_value());
    CHECK(presentation->rows.size() == 2);
    CHECK(presentation->rows[0].indicator_bounds == Rect{6, 3, 3, 1});
    CHECK(presentation->rows[1].node == TreeNodePath{{0, 0}});

    ScreenBuffer buffer({40, 10});
    CHECK(TerminalTreeViewPresentation::render(buffer, *presentation));
    CHECK(buffer.at({6, 3}).code_point == U'[');
    CHECK(buffer.at({14, 4}).code_point == U'\u65e5');
}

TEST_CASE("Terminal TreeView hit testing distinguishes indicator and row") {
    StringTreeModel model({"Parent"});
    const auto parent = TreeNodePath{{0}};
    model.append(parent, "child");
    TreeView view;
    view.setModel(&model);
    view.setViewport(std::nullopt, 1);
    const auto presentation = TerminalTreeViewPresentation::snapshot(view, {4, 5, 12, 1});
    CHECK(presentation.has_value());

    const auto indicator = TerminalTreeViewPresentation::hitAt(*presentation, {8, 5});
    CHECK(indicator.has_value());
    CHECK(indicator->kind == TerminalTreeViewHitKind::indicator);
    const auto row = TerminalTreeViewPresentation::hitAt(*presentation, {12, 5});
    CHECK(row.has_value());
    CHECK(row->kind == TerminalTreeViewHitKind::row);
    CHECK(!TerminalTreeViewPresentation::hitAt(*presentation, {4, 6}).has_value());
}

TEST_CASE("Terminal TreeView rejects stale selection and toggle snapshots") {
    StringTreeModel model({"Parent"});
    const auto parent = TreeNodePath{{0}};
    model.append(parent, "child");
    TreeSelectionModel selection;
    selection.setModel(&model);
    TreeView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(std::nullopt, 1);
    const auto presentation = TerminalTreeViewPresentation::snapshot(view, {0, 0, 12, 1});
    CHECK(presentation.has_value());

    model.setText(parent, "renamed");
    CHECK(!TerminalTreeViewPresentation::selectAt(*presentation, {8, 0}, selection));
    CHECK(!TerminalTreeViewPresentation::toggleAt(*presentation, {0, 0}, view));
}

TEST_CASE("Terminal TreeView rendering is transactional for malformed rows") {
    TerminalTreeViewPresentationSnapshot snapshot;
    snapshot.bounds = {0, 0, 8, 1};
    snapshot.rows.push_back({TreeNodePath{{0}}, 99, "bad", {0, 0, 8, 1}, {}, false, false, false});
    ScreenBuffer buffer({8, 2});
    buffer.set({0, 0}, Cell{U'X'});
    CHECK(!TerminalTreeViewPresentation::render(buffer, snapshot));
    CHECK(buffer.at({0, 0}).code_point == U'X');
}
