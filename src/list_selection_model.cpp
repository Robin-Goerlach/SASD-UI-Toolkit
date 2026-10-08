#include <sasd/ui/list_selection_model.hpp>

#include <algorithm>
#include <limits>

namespace sasd::ui {

ListSelectionModel::~ListSelectionModel() {
    if (observers_) {
        observers_->selection = nullptr;
    }
}

ListSelectionModel* ListSelectionModel::Reference::get() const noexcept {
    const auto state = state_.lock();
    return state ? state->selection : nullptr;
}

bool ListSelectionModel::Subscription::connected() const noexcept {
    const auto slot = slot_.lock();
    return slot != nullptr && slot->active;
}

void ListSelectionModel::Subscription::reset() noexcept {
    if (const auto slot = slot_.lock()) {
        slot->active = false;
    }
    slot_.reset();
}

std::shared_ptr<ListSelectionModel::ObserverState> ListSelectionModel::ensureObserverState() {
    if (!observers_) {
        observers_ = std::make_shared<ObserverState>();
        observers_->selection = this;
    }
    return observers_;
}

ListSelectionModel::Reference ListSelectionModel::reference() {
    return Reference{ensureObserverState()};
}

ListSelectionModel::Subscription ListSelectionModel::observe(ChangedHandler handler) {
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

void ListSelectionModel::compactInactiveObservers(const std::shared_ptr<ObserverState>& state) {
    if (state) {
        state->slots.erase(
            std::remove_if(state->slots.begin(), state->slots.end(), [](const auto& slot) {
                return !slot || !slot->active;
            }),
            state->slots.end());
    }
}

void ListSelectionModel::notifyObservers(std::shared_ptr<ObserverState> state,
                                         const ListSelectionChange& change) {
    if (!state) {
        return;
    }
    compactInactiveObservers(state);
    const auto snapshot = state->slots;
    for (const auto& slot : snapshot) {
        if (!slot || !slot->active) {
            continue;
        }
        const auto handler = slot->handler;
        if (handler) {
            handler(change);
        }
    }
}

void ListSelectionModel::setModel(ListModel* model) {
    if (model_.get() == model) {
        return;
    }

    model_subscription_.reset();
    model_ = model != nullptr ? model->reference() : ListModel::Reference{};
    if (model != nullptr) {
        model_subscription_ = model->observe([this](const ListModelChange& change) {
            modelChanged(change);
        });
    }

    setSelected(std::nullopt);
}

std::optional<std::size_t> ListSelectionModel::selectedRow() const noexcept {
    const auto* model = model_.get();
    if (model == nullptr || !selected_row_.has_value() || *selected_row_ >= model->rowCount()) {
        return std::nullopt;
    }
    return selected_row_;
}

bool ListSelectionModel::setSelected(std::optional<std::size_t> row) {
    if (row.has_value()) {
        const auto* model = model_.get();
        if (model == nullptr || *row >= model->rowCount()) {
            return false;
        }
    }

    if (selected_row_ == row) {
        return false;
    }
    const auto previous = selected_row_;
    selected_row_ = row;
    notifySelection(previous);
    return true;
}

bool ListSelectionModel::clear() {
    return setSelected(std::nullopt);
}

bool ListSelectionModel::select(std::size_t row) {
    return setSelected(row);
}

bool ListSelectionModel::selectNext() {
    const auto* model = model_.get();
    if (model == nullptr || model->rowCount() == 0) {
        return false;
    }
    const auto current = selectedRow();
    if (!current.has_value()) {
        return setSelected(std::size_t{0});
    }
    if (*current == model->rowCount() - 1) {
        return false;
    }
    return setSelected(*current + 1);
}

bool ListSelectionModel::selectPrevious() {
    const auto current = selectedRow();
    if (!current.has_value() || *current == 0) {
        return false;
    }
    return setSelected(*current - 1);
}

bool ListSelectionModel::selectFirst() {
    const auto* model = model_.get();
    return model != nullptr && model->rowCount() != 0 && setSelected(std::size_t{0});
}

bool ListSelectionModel::selectLast() {
    const auto* model = model_.get();
    return model != nullptr && model->rowCount() != 0 && setSelected(model->rowCount() - 1);
}

void ListSelectionModel::modelChanged(const ListModelChange& change) {
    const auto previous = selected_row_;
    if (!selected_row_.has_value()) {
        return;
    }

    switch (change.kind) {
    case ListModelChange::Kind::reset:
        selected_row_.reset();
        break;
    case ListModelChange::Kind::row_changed:
        if (*selected_row_ >= model_.get()->rowCount()) {
            selected_row_.reset();
        }
        break;
    case ListModelChange::Kind::rows_inserted:
        if (change.first_row <= *selected_row_) {
            if (change.row_count > std::numeric_limits<std::size_t>::max() - *selected_row_) {
                // A representable row index cannot be produced. Clearing is safer than silently
                // retaining an index that could now identify unrelated data.
                selected_row_.reset();
            } else {
                *selected_row_ += change.row_count;
            }
        }
        break;
    case ListModelChange::Kind::rows_removed:
        if (change.first_row > *selected_row_) {
            break;
        }
        if (change.row_count > *selected_row_ - change.first_row) {
            selected_row_.reset();
        } else {
            *selected_row_ -= change.row_count;
        }
        break;
    }

    if (previous != selected_row_) {
        notifySelection(previous);
    }
}

void ListSelectionModel::notifySelection(std::optional<std::size_t> previous) {
    notifyObservers(observers_, {previous, selected_row_});
}

} // namespace sasd::ui
