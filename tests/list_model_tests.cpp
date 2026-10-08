#include "test_framework.hpp"

#include <sasd/ui/list_model.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace sasd::ui;

TEST_CASE("StringListModel supports empty, UTF-8 and positional mutations") {
    StringListModel model;
    CHECK(model.rowCount() == 0);

    model.append("eins");
    model.append("größer");
    model.insert(1, "zwischen");
    CHECK(model.rowCount() == 3);
    CHECK(model.textAt(1) == "zwischen");
    CHECK(model.textAt(2) == "größer");

    model.setText(1, "日本語");
    CHECK(model.textAt(1) == "日本語");
    model.erase(0);
    CHECK(model.textAt(0) == "日本語");
}

TEST_CASE("ListModel reports coherent differentiated changes") {
    StringListModel model;
    std::vector<ListModelChange> changes;
    auto subscription = model.observe([&](const ListModelChange& change) {
        changes.push_back(change);
        // The callback observes the post-mutation model. This is especially important for a View:
        // it may rebuild its visible range immediately without querying half-applied state.
        switch (change.kind) {
        case ListModelChange::Kind::rows_inserted:
        case ListModelChange::Kind::row_changed:
            CHECK(model.rowCount() == 1);
            break;
        case ListModelChange::Kind::rows_removed:
            CHECK(model.rowCount() == 0);
            break;
        case ListModelChange::Kind::reset:
            CHECK(model.rowCount() == 2);
            break;
        }
    });

    model.append("value");
    model.setText(0, "changed");
    model.erase(0);
    model.reset({"a", "b"});
    CHECK(changes.size() == 4);
    CHECK(changes[0].kind == ListModelChange::Kind::rows_inserted);
    CHECK(changes[1].kind == ListModelChange::Kind::row_changed);
    CHECK(changes[2].kind == ListModelChange::Kind::rows_removed);
    CHECK(changes[3].kind == ListModelChange::Kind::reset);
    CHECK(changes[2].first_row == 0 && changes[2].row_count == 1);
    subscription.reset();
}

TEST_CASE("ListModel observer can disconnect a later observer during notification") {
    StringListModel model;
    ListModel::Subscription later;
    int first_calls = 0;
    int later_calls = 0;

    auto first = model.observe([&](const ListModelChange&) {
        ++first_calls;
        later.reset();
    });
    later = model.observe([&](const ListModelChange&) { ++later_calls; });

    model.append("one");
    CHECK(first_calls == 1);
    CHECK(later_calls == 0);
    CHECK(!later.connected());
}

TEST_CASE("ListModel references expire when the model is destroyed") {
    ListModel::Reference reference;
    {
        auto model = std::make_unique<StringListModel>();
        reference = model->reference();
        CHECK(reference.get() == model.get());
    }
    CHECK(reference.get() == nullptr);
}

TEST_CASE("ListModel supports destructive observer callbacks") {
    std::unique_ptr<StringListModel> model = std::make_unique<StringListModel>();
    ListModel::Subscription subscription;
    int calls = 0;
    subscription = model->observe([&](const ListModelChange&) {
        ++calls;
        model.reset();
    });

    model->append("one");
    CHECK(calls == 1);
    CHECK(model == nullptr);
    CHECK(!subscription.connected());
}

TEST_CASE("StringListModel rejects invalid indices") {
    StringListModel model;
    bool threw = false;
    try {
        model.erase(0);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}
