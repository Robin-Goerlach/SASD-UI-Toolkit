#include <sasd/ui/table_selection_model.hpp>

#include <algorithm>
#include <limits>

namespace sasd::ui {

TableSelectionModel::~TableSelectionModel() {
    if (observers_) {
        observers_->selection = nullptr;
    }
}

TableSelectionModel* TableSelectionModel::Reference::get() const noexcept {
    const auto state = state_.lock();
    return state ? state->selection : nullptr;
}

bool TableSelectionModel::Subscription::connected() const noexcept {
    const auto slot = slot_.lock();
    return slot != nullptr && slot->active;
}

void TableSelectionModel::Subscription::reset() noexcept {
    if (const auto slot = slot_.lock()) {
        slot->active = false;
    }
    slot_.reset();
}

std::shared_ptr<TableSelectionModel::ObserverState>
TableSelectionModel::ensureObserverState() {
    if (!observers_) {
        observers_ = std::make_shared<ObserverState>();
        observers_->selection = this;
    }
    return observers_;
}

TableSelectionModel::Reference TableSelectionModel::reference() {
    return Reference{ensureObserverState()};
}

TableSelectionModel::Subscription TableSelectionModel::observe(ChangedHandler handler) {
    if (!handler) {
        return {};
    }
    const auto state = ensureObserverState();
    compactInactiveObservers(state);
    auto slot = std::make_shared<ObserverSlot>();
    slot->handler = std::move(handler);
    state->slots.push_back(slot);
    return Subscription{slot};
}

void TableSelectionModel::compactInactiveObservers(
    const std::shared_ptr<ObserverState>& state) {
    if (state) {
        state->slots.erase(
            std::remove_if(state->slots.begin(), state->slots.end(), [](const auto& slot) {
                return !slot || !slot->active;
            }),
            state->slots.end());
    }
}

void TableSelectionModel::notifyObservers(std::shared_ptr<ObserverState> state,
                                          const TableSelectionChange& change) {
    if (!state) {
        return;
    }
    compactInactiveObservers(state);
    const auto slots = state->slots;
    for (const auto& slot : slots) {
        if (!slot || !slot->active) {
            continue;
        }
        const auto handler = slot->handler;
        if (handler) {
            handler(change);
        }
    }
}

void TableSelectionModel::setModel(TableModel* model) {
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
    setSelected(std::nullopt);
}

std::optional<TableCell> TableSelectionModel::selectedCell() const noexcept {
    const auto* model = model_.get();
    if (model == nullptr || !selected_cell_.has_value() ||
        selected_cell_->row >= model->rowCount() ||
        selected_cell_->column >= model->columnCount()) {
        return std::nullopt;
    }
    return selected_cell_;
}

bool TableSelectionModel::setSelected(std::optional<TableCell> cell) {
    if (cell.has_value()) {
        const auto* model = model_.get();
        if (model == nullptr || cell->row >= model->rowCount() ||
            cell->column >= model->columnCount()) {
            return false;
        }
    }
    if (selected_cell_ == cell) {
        return false;
    }
    const auto previous = selected_cell_;
    selected_cell_ = cell;
    // The callback may destroy this selection owner. No member access is permitted after this call.
    notifySelection(previous);
    return true;
}

bool TableSelectionModel::clear() {
    return setSelected(std::nullopt);
}

bool TableSelectionModel::select(std::size_t row, std::size_t column) {
    return setSelected(TableCell{row, column});
}

bool TableSelectionModel::selectUp() {
    const auto current = selectedCell();
    return current.has_value() && current->row != 0 &&
           setSelected(TableCell{current->row - 1, current->column});
}

bool TableSelectionModel::selectDown() {
    const auto* model = model_.get();
    if (model == nullptr || model->rowCount() == 0 || model->columnCount() == 0) {
        return false;
    }
    const auto current = selectedCell();
    if (!current.has_value()) {
        return setSelected(TableCell{0, 0});
    }
    if (current->row == model->rowCount() - 1) {
        return false;
    }
    return setSelected(TableCell{current->row + 1, current->column});
}

bool TableSelectionModel::selectLeft() {
    const auto current = selectedCell();
    return current.has_value() && current->column != 0 &&
           setSelected(TableCell{current->row, current->column - 1});
}

bool TableSelectionModel::selectRight() {
    const auto* model = model_.get();
    if (model == nullptr || model->rowCount() == 0 || model->columnCount() == 0) {
        return false;
    }
    const auto current = selectedCell();
    if (!current.has_value()) {
        return setSelected(TableCell{0, 0});
    }
    if (current->column == model->columnCount() - 1) {
        return false;
    }
    return setSelected(TableCell{current->row, current->column + 1});
}

void TableSelectionModel::modelChanged(const TableModelChange& change) {
    const auto previous = selected_cell_;
    if (!selected_cell_.has_value()) {
        return;
    }

    switch (change.kind) {
    case TableModelChange::Kind::reset:
        selected_cell_.reset();
        break;
    case TableModelChange::Kind::cells_changed:
        break;
    case TableModelChange::Kind::rows_inserted:
        if (change.first_row <= selected_cell_->row) {
            if (change.row_count > std::numeric_limits<std::size_t>::max() - selected_cell_->row) {
                selected_cell_.reset();
            } else {
                selected_cell_->row += change.row_count;
            }
        }
        break;
    case TableModelChange::Kind::rows_removed:
        if (change.row_count == 0 || change.first_row > selected_cell_->row) {
            break;
        }
        if (change.row_count > selected_cell_->row - change.first_row) {
            selected_cell_.reset();
        } else {
            selected_cell_->row -= change.row_count;
        }
        break;
    }

    const auto* model = model_.get();
    if (model == nullptr || (selected_cell_.has_value() &&
                             (selected_cell_->row >= model->rowCount() ||
                              selected_cell_->column >= model->columnCount()))) {
        selected_cell_.reset();
    }
    if (previous != selected_cell_) {
        notifySelection(previous);
    }
}

void TableSelectionModel::notifySelection(std::optional<TableCell> previous) {
    notifyObservers(observers_, {previous, selected_cell_});
}

} // namespace sasd::ui
