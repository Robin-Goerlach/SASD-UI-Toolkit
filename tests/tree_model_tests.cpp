#include "test_framework.hpp"

#include <sasd/ui/tree_model.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace sasd::ui;

TEST_CASE("StringTreeModel exposes positional paths and UTF-8 text") {
    StringTreeModel model({"Root", "zweite"});
    const auto root = TreeNodePath{{0}};
    const auto child = model.append(root, "日本語");

    CHECK(root == TreeNodePath{{0}});
    CHECK(child == TreeNodePath{{0, 0}});
    CHECK(model.isValidNode(child));
    CHECK(model.textAt(child) == "日本語");
    CHECK(model.childCount(TreeNodePath{{1}}) == 0);
    CHECK(!model.isValidNode(TreeNodePath{{4}}));
}

TEST_CASE("TreeModel reports completed changes and invalidates paths on reset") {
    StringTreeModel model({"old"});
    std::vector<TreeModelChange> changes;
    auto subscription = model.observe([&](const TreeModelChange& change) {
        changes.push_back(change);
        CHECK(model.childCount({}) == 2);
    });

    model.reset({"new", "second"});
    CHECK(changes.size() == 1);
    CHECK(changes.front().kind == TreeModelChange::Kind::reset);
    CHECK(model.isValidNode(TreeNodePath{{1}}));
    CHECK(subscription.connected());
}

TEST_CASE("TreeModel references and observers fail closed across destruction") {
    TreeModel::Reference reference;
    TreeModel::Subscription subscription;
    {
        auto model = std::make_unique<StringTreeModel>(std::vector<std::string>{"one"});
        reference = model->reference();
        subscription = model->observe([&](const TreeModelChange&) { model.reset(); });
        model->reset({"replacement"});
    }
    CHECK(reference.get() == nullptr);
    CHECK(!subscription.connected());
}

TEST_CASE("TreeModel observer can disconnect a later observer") {
    StringTreeModel model({"one"});
    TreeModel::Subscription later;
    int first_calls = 0;
    int later_calls = 0;
    auto first = model.observe([&](const TreeModelChange&) {
        ++first_calls;
        later.reset();
    });
    later = model.observe([&](const TreeModelChange&) { ++later_calls; });

    model.setText(TreeNodePath{{0}}, "changed");
    CHECK(first_calls == 1);
    CHECK(later_calls == 0);
    CHECK(!later.connected());
    CHECK(first.connected());
}
