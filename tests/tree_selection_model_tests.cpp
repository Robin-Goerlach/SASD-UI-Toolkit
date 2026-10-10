#include "test_framework.hpp"

#include <sasd/ui/tree_selection_model.hpp>

#include <memory>

using namespace sasd::ui;

TEST_CASE("TreeSelectionModel selects and clears exact node paths") {
    StringTreeModel model({"root"});
    const auto child = model.append({}, "child");
    TreeSelectionModel selection;
    selection.setModel(&model);

    CHECK(!selection.selectedNode().has_value());
    CHECK(selection.select(child));
    CHECK(selection.selectedNode() == std::optional<TreeNodePath>{child});
    CHECK(selection.clear());
    CHECK(!selection.selectedNode().has_value());
    CHECK(!selection.select(TreeNodePath{{9}}));
}

TEST_CASE("TreeSelectionModel clears on reset and survives text-only changes") {
    StringTreeModel model({"root"});
    TreeSelectionModel selection;
    selection.setModel(&model);
    const TreeNodePath root{{0}};
    CHECK(selection.select(root));

    model.setText(root, "renamed");
    CHECK(selection.selectedNode() == std::optional<TreeNodePath>{root});
    model.reset({"replacement"});
    CHECK(!selection.selectedNode().has_value());
}

TEST_CASE("TreeSelectionModel fails closed when its model is destroyed") {
    TreeSelectionModel selection;
    {
        auto model = std::make_unique<StringTreeModel>(std::vector<std::string>{"root"});
        selection.setModel(model.get());
        CHECK(selection.select(TreeNodePath{{0}}));
    }
    CHECK(selection.model() == nullptr);
    CHECK(!selection.selectedNode().has_value());
}

TEST_CASE("TreeSelectionModel destructive callback is safe") {
    auto selection = std::make_unique<TreeSelectionModel>();
    StringTreeModel model({"root"});
    selection->setModel(&model);
    TreeSelectionModel::Subscription subscription;
    subscription = selection->observe([&](const TreeSelectionChange&) { selection.reset(); });

    CHECK(selection->select(TreeNodePath{{0}}));
    CHECK(selection == nullptr);
    CHECK(!subscription.connected());
}
