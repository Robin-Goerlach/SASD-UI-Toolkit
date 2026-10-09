#include <sasd/ui/table_model.hpp>

#include <algorithm>

namespace sasd::ui {

TableModel::~TableModel() {
    // References may outlive the model. Clearing this back-reference makes every later lookup
    // fail closed before Component storage or derived model data can be touched.
    if (observers_) {
        observers_->model = nullptr;
    }
}

TableModel* TableModel::Reference::get() const noexcept {
    const auto state = state_.lock();
    return state ? state->model : nullptr;
}

bool TableModel::Subscription::connected() const noexcept {
    const auto slot = slot_.lock();
    return slot != nullptr && slot->active;
}

void TableModel::Subscription::reset() noexcept {
    if (const auto slot = slot_.lock()) {
        slot->active = false;
    }
    slot_.reset();
}

std::shared_ptr<TableModel::ObserverState> TableModel::ensureObserverState() {
    if (!observers_) {
        observers_ = std::make_shared<ObserverState>();
        observers_->model = this;
    }
    return observers_;
}

TableModel::Reference TableModel::reference() {
    return Reference{ensureObserverState()};
}

TableModel::Subscription TableModel::observe(ChangedHandler handler) {
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

void TableModel::compactInactiveObservers(const std::shared_ptr<ObserverState>& state) {
    if (!state) {
        return;
    }

    state->slots.erase(
        std::remove_if(state->slots.begin(), state->slots.end(), [](const auto& slot) {
            return !slot || !slot->active;
        }),
        state->slots.end());
}

void TableModel::notifyObservers(std::shared_ptr<ObserverState> state,
                                 const TableModelChange& change) {
    if (!state) {
        return;
    }

    compactInactiveObservers(state);
    const auto snapshot = state->slots;
    for (const auto& slot : snapshot) {
        if (!slot || !slot->active) {
            continue;
        }

        // Copy application code before calling it. A callback may disconnect another observer or
        // destroy the model owner; this invocation then relies only on local shared state.
        const auto handler = slot->handler;
        if (handler) {
            handler(change);
        }
    }
}

void TableModel::notifyChanged(TableModelChange change) {
    // The derived mutation is complete before notification. No TableModel member is accessed after
    // callback entry because application code is allowed to destroy the model owner synchronously.
    ++revision_;
    notifyObservers(observers_, change);
}

StringTableModel::StringTableModel(std::vector<std::string> headers,
                                   std::vector<std::vector<std::string>> rows)
    : headers_{std::move(headers)}, rows_{std::move(rows)} {
    validateTable(headers_, rows_);
}

void StringTableModel::validateRow(const std::vector<std::string>& row) const {
    if (row.size() != headers_.size()) {
        throw std::invalid_argument{"StringTableModel rows must match header count"};
    }
}

void StringTableModel::validateTable(const std::vector<std::string>& headers,
                                     const std::vector<std::vector<std::string>>& rows) const {
    for (const auto& row : rows) {
        if (row.size() != headers.size()) {
            throw std::invalid_argument{"StringTableModel rows must match header count"};
        }
    }
}

void StringTableModel::appendRow(std::vector<std::string> row) {
    validateRow(row);
    const std::size_t first_row = rows_.size();
    rows_.push_back(std::move(row));
    notifyChanged({TableModelChange::Kind::rows_inserted, first_row, 1, 0, headers_.size()});
}

void StringTableModel::insertRow(std::size_t row, std::vector<std::string> values) {
    if (row > rows_.size()) {
        throw std::out_of_range{"StringTableModel insert row out of range"};
    }
    validateRow(values);
    rows_.insert(rows_.begin() + static_cast<std::ptrdiff_t>(row), std::move(values));
    notifyChanged({TableModelChange::Kind::rows_inserted, row, 1, 0, headers_.size()});
}

void StringTableModel::eraseRow(std::size_t row) {
    if (row >= rows_.size()) {
        throw std::out_of_range{"StringTableModel erase row out of range"};
    }
    rows_.erase(rows_.begin() + static_cast<std::ptrdiff_t>(row));
    notifyChanged({TableModelChange::Kind::rows_removed, row, 1, 0, headers_.size()});
}

void StringTableModel::setCell(std::size_t row, std::size_t column, std::string value) {
    if (row >= rows_.size() || column >= headers_.size()) {
        throw std::out_of_range{"StringTableModel cell out of range"};
    }
    if (rows_[row][column] == value) {
        return;
    }

    rows_[row][column] = std::move(value);
    notifyChanged({TableModelChange::Kind::cells_changed, row, 1, column, 1});
}

void StringTableModel::reset(std::vector<std::string> headers,
                             std::vector<std::vector<std::string>> rows) {
    validateTable(headers, rows);
    headers_ = std::move(headers);
    rows_ = std::move(rows);
    notifyChanged({TableModelChange::Kind::reset, 0, 0, 0, 0});
}

} // namespace sasd::ui
