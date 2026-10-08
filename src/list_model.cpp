#include <sasd/ui/list_model.hpp>

namespace sasd::ui {

ListModel::~ListModel() {
    // An in-flight callback may retain the shared observer state after the model is gone. Clear its
    // back-reference before Component storage disappears so every later Reference resolves null.
    if (observers_) {
        observers_->model = nullptr;
    }
}

ListModel* ListModel::Reference::get() const noexcept {
    const auto state = state_.lock();
    return state ? state->model : nullptr;
}

bool ListModel::Subscription::connected() const noexcept {
    const auto slot = slot_.lock();
    return slot != nullptr && slot->active;
}

void ListModel::Subscription::reset() noexcept {
    if (const auto slot = slot_.lock()) {
        slot->active = false;
    }
    slot_.reset();
}

std::shared_ptr<ListModel::ObserverState> ListModel::ensureObserverState() {
    if (!observers_) {
        observers_ = std::make_shared<ObserverState>();
        observers_->model = this;
    }
    return observers_;
}

ListModel::Reference ListModel::reference() {
    return Reference{ensureObserverState()};
}

ListModel::Subscription ListModel::observe(ChangedHandler handler) {
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

void ListModel::compactInactiveObservers(const std::shared_ptr<ObserverState>& state) {
    if (!state) {
        return;
    }

    state->slots.erase(
        std::remove_if(state->slots.begin(), state->slots.end(), [](const auto& slot) {
            return !slot || !slot->active;
        }),
        state->slots.end());
}

void ListModel::notifyObservers(std::shared_ptr<ObserverState> state,
                                const ListModelChange& change) {
    if (!state) {
        return;
    }

    compactInactiveObservers(state);
    const auto snapshot = state->slots;
    for (const auto& slot : snapshot) {
        if (!slot || !slot->active) {
            continue;
        }

        // Copy the callback before entering application code. It may disconnect or destroy the model,
        // while shared state and this local function object keep the current invocation valid.
        const auto handler = slot->handler;
        if (handler) {
            handler(change);
        }
    }
}

void ListModel::notifyChanged(ListModelChange change) {
    // Derived mutation must finish before this function is called. No ListModel member is touched
    // after notification begins because an observer is allowed to release/destroy its model owner.
    notifyObservers(observers_, change);
}

void StringListModel::append(std::string text) {
    const std::size_t row = rows_.size();
    rows_.push_back(std::move(text));
    notifyChanged({ListModelChange::Kind::rows_inserted, row, 1});
}

void StringListModel::insert(std::size_t row, std::string text) {
    if (row > rows_.size()) {
        throw std::out_of_range{"StringListModel insert row out of range"};
    }

    rows_.insert(rows_.begin() + static_cast<std::ptrdiff_t>(row), std::move(text));
    notifyChanged({ListModelChange::Kind::rows_inserted, row, 1});
}

void StringListModel::erase(std::size_t row) {
    if (row >= rows_.size()) {
        throw std::out_of_range{"StringListModel erase row out of range"};
    }

    rows_.erase(rows_.begin() + static_cast<std::ptrdiff_t>(row));
    notifyChanged({ListModelChange::Kind::rows_removed, row, 1});
}

void StringListModel::setText(std::size_t row, std::string text) {
    if (row >= rows_.size()) {
        throw std::out_of_range{"StringListModel setText row out of range"};
    }
    if (rows_[row] == text) {
        return;
    }

    rows_[row] = std::move(text);
    notifyChanged({ListModelChange::Kind::row_changed, row, 1});
}

void StringListModel::reset(std::vector<std::string> rows) {
    rows_ = std::move(rows);
    notifyChanged({ListModelChange::Kind::reset, 0, 0});
}

} // namespace sasd::ui
