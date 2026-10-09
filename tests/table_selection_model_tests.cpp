#include "test_framework.hpp"

#include <sasd/ui/table_selection_model.hpp>

#include <optional>

using namespace sasd::ui;

TEST_CASE("TableSelectionModel selects and navigates a single cell") {
    StringTableModel model({"A", "B", "C"}, {{"a", "b", "c"}, {"d", "e", "f"}});
    TableSelectionModel selection;
    selection.setModel(&model);

    CHECK(!selection.selectedCell().has_value());
    CHECK(selection.select(1, 1));
    CHECK(selection.selectedCell() == std::optional<TableCell>{TableCell{1, 1}});
    CHECK(selection.selectUp());
    CHECK(selection.selectedCell() == std::optional<TableCell>{TableCell{0, 1}});
    CHECK(selection.selectRight());
    CHECK(selection.selectedCell() == std::optional<TableCell>{TableCell{0, 2}});
    CHECK(selection.selectDown());
    CHECK(selection.selectedCell() == std::optional<TableCell>{TableCell{1, 2}});
    CHECK(selection.selectLeft());
    CHECK(selection.selectedCell() == std::optional<TableCell>{TableCell{1, 1}});
    CHECK(selection.clear());
    CHECK(!selection.selectedCell().has_value());
}

TEST_CASE("TableSelectionModel clears on reset, replacement and model destruction") {
    auto model = std::make_unique<StringTableModel>(std::vector<std::string>{"A"},
                                                    std::vector<std::vector<std::string>>{{"x"}});
    TableSelectionModel selection;
    selection.setModel(model.get());
    CHECK(selection.select(0, 0));
    model->reset({"A"}, {{"y"}});
    CHECK(!selection.selectedCell().has_value());

    StringTableModel replacement({"A", "B"}, {{"x", "y"}});
    selection.setModel(&replacement);
    CHECK(!selection.selectedCell().has_value());
    CHECK(selection.select(0, 1));
    model.reset();
    CHECK(selection.selectedCell() == std::optional<TableCell>{TableCell{0, 1}});
    selection.setModel(nullptr);
    CHECK(!selection.selectedCell().has_value());
}

TEST_CASE("TableSelectionModel fails closed after its TableModel is destroyed") {
    TableSelectionModel selection;
    {
        StringTableModel model({"A"}, {{"x"}});
        selection.setModel(&model);
        CHECK(selection.select(0, 0));
        CHECK(selection.selectedCell().has_value());
    }
    CHECK(selection.model() == nullptr);
    CHECK(!selection.selectedCell().has_value());
}

TEST_CASE("TableSelectionModel normalizes row insertion and removal") {
    StringTableModel model({"A"}, {{"a"}, {"b"}, {"c"}});
    TableSelectionModel selection;
    selection.setModel(&model);
    CHECK(selection.select(1, 0));
    model.insertRow(0, {"new"});
    CHECK(selection.selectedCell() == std::optional<TableCell>{TableCell{2, 0}});
    model.eraseRow(2);
    CHECK(!selection.selectedCell().has_value());
}

TEST_CASE("TableSelectionModel does not emit duplicate selection changes") {
    StringTableModel model({"A"}, {{"a"}});
    TableSelectionModel selection;
    selection.setModel(&model);
    int notifications = 0;
    auto subscription = selection.observe([&](const TableSelectionChange&) { ++notifications; });
    CHECK(selection.select(0, 0));
    CHECK(!selection.select(0, 0));
    CHECK(notifications == 1);
    CHECK(subscription.connected());
}
