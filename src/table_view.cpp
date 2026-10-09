#include <sasd/ui/table_view.hpp>

#include <algorithm>

namespace sasd::ui {

TableView::TableView() {
    setFocusable(true);
}

TableView::~TableView() = default;

void TableView::setModel(TableModel* model) {
    if (model_.get() == model) {
        return;
    }

    model_subscription_.reset();
    model_ = model != nullptr ? model->reference() : TableModel::Reference{};
    if (model != nullptr) {
        model_subscription_ = model->observe([this](const TableModelChange& change) {
            modelChanged(change);
        });
    }
    normalizeViewport();
    invalidateMeasure();
    invalidateVisual();
}

void TableView::setViewport(std::size_t first_row,
                            std::size_t row_count,
                            std::size_t first_column,
                            std::size_t column_count) {
    first_visible_row_ = first_row;
    visible_row_count_ = row_count;
    first_visible_column_ = first_column;
    visible_column_count_ = column_count;
    normalizeViewport();
    invalidateVisual();
}

void TableView::normalizeViewport() noexcept {
    const auto* model = model_.get();
    if (model == nullptr) {
        first_visible_row_ = 0;
        first_visible_column_ = 0;
        return;
    }

    if (first_visible_row_ > model->rowCount()) {
        first_visible_row_ = model->rowCount();
    }
    if (first_visible_column_ > model->columnCount()) {
        first_visible_column_ = model->columnCount();
    }
}

std::vector<std::string> TableView::visibleHeaders() const {
    std::vector<std::string> result;
    const auto* model = model_.get();
    if (model == nullptr || visible_column_count_ == 0) {
        return result;
    }

    const auto column_count = model->columnCount();
    const auto start = std::min(first_visible_column_, column_count);
    const auto columns_to_copy = std::min(visible_column_count_, column_count - start);
    result.reserve(columns_to_copy);
    for (std::size_t offset = 0; offset < columns_to_copy; ++offset) {
        const auto column = start + offset;
        // The model owns this view; the snapshot owns the copy. No string_view crosses this API.
        result.emplace_back(model->headerAt(column));
    }
    return result;
}

std::vector<TableViewRow> TableView::visibleRows() const {
    std::vector<TableViewRow> result;
    const auto* model = model_.get();
    if (model == nullptr || visible_row_count_ == 0 || visible_column_count_ == 0) {
        return result;
    }

    const auto row_count = model->rowCount();
    const auto column_count = model->columnCount();
    const auto first_row = std::min(first_visible_row_, row_count);
    const auto first_column = std::min(first_visible_column_, column_count);
    const auto rows_to_copy = std::min(visible_row_count_, row_count - first_row);
    const auto columns_to_copy = std::min(visible_column_count_, column_count - first_column);
    result.reserve(rows_to_copy);
    for (std::size_t row_offset = 0; row_offset < rows_to_copy; ++row_offset) {
        const auto row = first_row + row_offset;
        TableViewRow snapshot_row;
        snapshot_row.row = row;
        snapshot_row.cells.reserve(columns_to_copy);
        for (std::size_t column_offset = 0; column_offset < columns_to_copy; ++column_offset) {
            const auto column = first_column + column_offset;
            // Cell text is borrowed only for this call. Copying it before the next model query
            // keeps the owned snapshot independent from model storage and later invalidation.
            snapshot_row.cells.emplace_back(model->textAt(row, column));
        }
        result.push_back(std::move(snapshot_row));
    }
    return result;
}

void TableView::modelChanged(const TableModelChange&) {
    normalizeViewport();
    invalidateMeasure();
    invalidateVisual();
}

} // namespace sasd::ui
