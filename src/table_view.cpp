#include <sasd/ui/table_view.hpp>

#include <algorithm>
#include <variant>

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
    keepSelectionVisible();
    invalidateMeasure();
    invalidateVisual();
}

void TableView::setSelectionModel(TableSelectionModel* selection_model) {
    if (selection_model_.get() == selection_model) {
        return;
    }

    selection_subscription_.reset();
    selection_model_ = selection_model != nullptr ? selection_model->reference()
                                                   : TableSelectionModel::Reference{};
    if (selection_model != nullptr) {
        selection_subscription_ = selection_model->observe([this](const TableSelectionChange& change) {
            selectionChanged(change);
        });
    }
    keepSelectionVisible();
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
    // Selection is only meaningful when it observes this exact model. This identity check keeps a
    // stale or accidentally shared selection object from painting an unrelated table's cell.
    const auto* selection = selection_model_.get();
    const auto selected = selection != nullptr && selection->model() == model
                              ? selection->selectedCell()
                              : std::nullopt;
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
        if (selected.has_value() && selected->row == row &&
            selected->column >= first_column &&
            selected->column - first_column < columns_to_copy) {
            snapshot_row.selected_column = selected->column;
        }
        result.push_back(std::move(snapshot_row));
    }
    return result;
}

bool TableView::moveSelection(Key key) {
    auto* selection = selection_model_.get();
    if (selection == nullptr || selection->model() != model_.get()) {
        return false;
    }

    bool changed = false;
    switch (key) {
    case Key::up:
        changed = selection->selectUp();
        break;
    case Key::down:
        changed = selection->selectDown();
        break;
    case Key::left:
        changed = selection->selectLeft();
        break;
    case Key::right:
        changed = selection->selectRight();
        break;
    default:
        return false;
    }
    if (changed) {
        keepSelectionVisible();
    }
    return changed;
}

EventResult TableView::onEvent(const Event& event) {
    const auto* key_event = std::get_if<KeyEvent>(&event);
    if (key_event == nullptr || !key_event->pressed ||
        key_event->modifiers != KeyModifier::none || !hasFocus() || !isVisible() ||
        !isEnabled()) {
        return EventResult::ignored;
    }
    return moveSelection(key_event->key) ? EventResult::handled : EventResult::ignored;
}

void TableView::keepSelectionVisible() {
    const auto* selection = selection_model_.get();
    const auto selected = selection != nullptr && selection->model() == model_.get()
                              ? selection->selectedCell()
                              : std::nullopt;
    if (!selected.has_value()) {
        return;
    }

    // Keep-visible changes only the semantic viewport. Rendered/Terminal builders later calculate
    // fresh final rectangles from this viewport; no backend geometry is retained in Core.
    if (visible_row_count_ != 0) {
        if (selected->row < first_visible_row_) {
            first_visible_row_ = selected->row;
        } else if (selected->row - first_visible_row_ >= visible_row_count_) {
            first_visible_row_ = selected->row - visible_row_count_ + 1;
        }
    }
    if (visible_column_count_ != 0) {
        if (selected->column < first_visible_column_) {
            first_visible_column_ = selected->column;
        } else if (selected->column - first_visible_column_ >= visible_column_count_) {
            first_visible_column_ = selected->column - visible_column_count_ + 1;
        }
    }
    normalizeViewport();
}

void TableView::modelChanged(const TableModelChange&) {
    normalizeViewport();
    keepSelectionVisible();
    invalidateMeasure();
    invalidateVisual();
}

void TableView::selectionChanged(const TableSelectionChange&) {
    keepSelectionVisible();
    invalidateVisual();
}

} // namespace sasd::ui
