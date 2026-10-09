#include "test_framework.hpp"

#include <sasd/ui/table_view.hpp>

#include <string>
#include <string_view>
#include <vector>

using namespace sasd::ui;

namespace {

class CountingTableModel final : public TableModel {
public:
    CountingTableModel(std::size_t rows, std::size_t columns) : rows_{rows}, columns_{columns} {}

    [[nodiscard]] std::size_t rowCount() const noexcept override { return rows_; }
    [[nodiscard]] std::size_t columnCount() const noexcept override { return columns_; }

    [[nodiscard]] std::string_view headerAt(std::size_t column) const override {
        ++header_queries_;
        header_ = "column-" + std::to_string(column);
        return header_;
    }

    [[nodiscard]] std::string_view textAt(std::size_t row, std::size_t column) const override {
        ++cell_queries_;
        cell_ = "cell-" + std::to_string(row) + "-" + std::to_string(column);
        return cell_;
    }

    [[nodiscard]] std::size_t headerQueries() const noexcept { return header_queries_; }
    [[nodiscard]] std::size_t cellQueries() const noexcept { return cell_queries_; }

private:
    std::size_t rows_;
    std::size_t columns_;
    mutable std::size_t header_queries_{0};
    mutable std::size_t cell_queries_{0};
    mutable std::string header_;
    mutable std::string cell_;
};

} // namespace

TEST_CASE("TableView materializes only its visible rectangular range") {
    CountingTableModel model(1'000'000, 500'000);
    TableView view;
    view.setModel(&model);
    view.setViewport(500'000, 3, 7, 4);

    const auto headers = view.visibleHeaders();
    const auto rows = view.visibleRows();
    CHECK(headers.size() == 4);
    CHECK(headers.front() == "column-7");
    CHECK(rows.size() == 3);
    CHECK(rows.front().row == 500'000);
    CHECK(rows.front().cells.size() == 4);
    CHECK(rows.front().cells.front() == "cell-500000-7");
    CHECK(model.headerQueries() == 4);
    CHECK(model.cellQueries() == 12);
}

TEST_CASE("TableView uses owned values") {
    StringTableModel model({"Name", "Wert"}, {{"eins", "one"}, {"zwei", "two"}});

    TableView view;
    view.setModel(&model);
    view.setViewport(0, 2, 0, 2);
    const auto rows = view.visibleRows();

    CHECK(rows.size() == 2);
    CHECK(rows[1].cells[0] == "zwei");
    model.setCell(1, 0, "changed");
    CHECK(rows[1].cells[0] == "zwei");
}

TEST_CASE("TableView normalizes viewport and fails closed after model destruction") {
    TableView view;
    {
        StringTableModel model({"A"}, {{"one"}, {"two"}});
        view.setModel(&model);
        view.setViewport(99, 20, 99, 20);
        CHECK(view.firstVisibleRow() == 2);
        CHECK(view.firstVisibleColumn() == 1);
        CHECK(view.visibleRows().empty());
        CHECK(view.visibleHeaders().empty());
    }
    CHECK(view.model() == nullptr);
    CHECK(view.visibleRows().empty());
    CHECK(view.visibleHeaders().empty());
}

TEST_CASE("TableView does not manufacture selection state") {
    StringTableModel model({"A"}, {{"one"}, {"two"}, {"three"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 2, 0, 1);

    CHECK(view.visibleRows().front().row == 0);
}
