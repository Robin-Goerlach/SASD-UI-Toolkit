#include "test_framework.hpp"

#include <sasd/ui/table_model.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace sasd::ui;

TEST_CASE("StringTableModel preserves a rectangular UTF-8 table") {
    StringTableModel model({"Name", "Wert"}, {{"größer", "eins"}, {"日本語", "二"}});

    CHECK(model.rowCount() == 2);
    CHECK(model.columnCount() == 2);
    CHECK(model.headerAt(0) == "Name");
    CHECK(model.textAt(0, 0) == "größer");
    CHECK(model.textAt(1, 1) == "二");

    model.setCell(0, 1, "geändert");
    CHECK(model.textAt(0, 1) == "geändert");
    model.insertRow(1, {"neu", "drei"});
    CHECK(model.textAt(1, 0) == "neu");
    model.eraseRow(0);
    CHECK(model.textAt(0, 0) == "neu");
}

TEST_CASE("TableModel publishes post-mutation changes with positional ranges") {
    StringTableModel model({"A", "B"}, {{"a", "b"}});
    std::vector<TableModelChange> changes;
    auto subscription = model.observe([&](const TableModelChange& change) {
        changes.push_back(change);
        // The callback sees a complete rectangle. This is the observation boundary a virtualized
        // TableView uses to rebuild only the visible values.
        CHECK(model.rowCount() == 1);
        CHECK(model.columnCount() == 2);
        CHECK(model.textAt(0, 0) == "changed");
    });

    model.setCell(0, 0, "changed");
    CHECK(changes.size() == 1);
    CHECK(changes[0].kind == TableModelChange::Kind::cells_changed);
    CHECK(changes[0].first_row == 0 && changes[0].first_column == 0);
    CHECK(changes[0].row_count == 1 && changes[0].column_count == 1);
    subscription.reset();
}

TEST_CASE("TableModel observers can be disconnected and references expire") {
    StringTableModel model({"A"}, {{"one"}});
    TableModel::Subscription later;
    int first_calls = 0;
    int later_calls = 0;

    auto first = model.observe([&](const TableModelChange&) {
        ++first_calls;
        later.reset();
    });
    later = model.observe([&](const TableModelChange&) { ++later_calls; });
    model.setCell(0, 0, "two");

    CHECK(first_calls == 1);
    CHECK(later_calls == 0);
    CHECK(!later.connected());

    TableModel::Reference reference = model.reference();
    CHECK(reference.get() == &model);
    first.reset();
    later.reset();
}

TEST_CASE("TableModel destructive observer callback leaves no dangling reference") {
    std::unique_ptr<StringTableModel> model =
        std::make_unique<StringTableModel>(std::vector<std::string>{"A"},
                                           std::vector<std::vector<std::string>>{{"one"}});
    TableModel::Reference reference = model->reference();
    TableModel::Subscription subscription;
    int calls = 0;
    subscription = model->observe([&](const TableModelChange&) {
        ++calls;
        model.reset();
    });

    model->setCell(0, 0, "two");
    CHECK(calls == 1);
    CHECK(model == nullptr);
    CHECK(reference.get() == nullptr);
    CHECK(!subscription.connected());
}

TEST_CASE("StringTableModel rejects non-rectangular and out-of-range data") {
    bool threw = false;
    try {
        StringTableModel model({"A", "B"}, {{"only one"}});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);

    StringTableModel model({"A"}, {{"one"}});
    threw = false;
    try {
        model.setCell(1, 0, "bad");
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}
