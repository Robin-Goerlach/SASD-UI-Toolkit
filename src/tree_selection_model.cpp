#include <sasd/ui/tree_selection_model.hpp>

#include <algorithm>

namespace sasd::ui {

TreeSelectionModel::~TreeSelectionModel() {
    if (observers_) {
        observers_->selection = nullptr;
    }
}

TreeSelectionModel* TreeSelectionModel::Reference::get() const noexcept {
    const auto state = state_.lock();
    return state ? state->selection : nullptr;
}

bool TreeSelectionModel::Subscription::connected() const noexcept {
    const auto slot = slot_.lock();
    return slot != nullptr && slot->active;
}

void TreeSelectionModel::Subscription::reset() noexcept {
    if (const auto slot = slot_.lock()) {
        slot->active = false;
    }
    slot_.reset();
}

std::shared_ptr<TreeSelectionModel::ObserverState>
TreeSelectionModel::ensureObserverState() {
    if (!observers_) {
        observers_ = std::make_shared<ObserverState>();
        observers_->selection = this;
    }
    return observers_;
}

TreeSelectionModel::Reference TreeSelectionModel::reference() {
    return Reference{ensureObserverState()};
}

TreeSelectionModel::Subscription TreeSelectionModel::observe(ChangedHandler handler) {
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

void TreeSelectionModel::compactInactiveObservers(const std::shared_ptr<ObserverState>& state) {
    if (state) {
        state->slots.erase(
            std::remove_if(state->slots.begin(), state->slots.end(), [](const auto& slot) {
                return !slot || !slot->active;
            }),
            state->slots.end());
    }
}

void TreeSelectionModel::notifyObservers(std::shared_ptr<ObserverState> state,
                                         const TreeSelectionChange& change) {
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
            // The callback may destroy this selection owner. This loop uses only copied observer
            // state after callback entry and never touches a member of the destroyed owner.
            handler(change);
        }
    }
}

void TreeSelectionModel::setModel(TreeModel* model) {
    if (model_.get() == model) {
        return;
    }
    model_subscription_.reset();
    model_ = model != nullptr ? model->reference() : TreeModel::Reference{};
    if (model != nullptr) {
        model_subscription_ = model->observe([this](const TreeModelChange& change) {
            modelChanged(change);
        });
    }
    setSelected(std::nullopt);
}

std::optional<TreeNodePath> TreeSelectionModel::selectedNode() const noexcept {
    const auto* model = model_.get();
    if (model == nullptr || !selected_node_.has_value() ||
        !model->isValidNode(*selected_node_)) {
        return std::nullopt;
    }
    return selected_node_;
}

bool TreeSelectionModel::setSelected(std::optional<TreeNodePath> node) {
    if (node.has_value()) {
        const auto* model = model_.get();
        if (model == nullptr || !model->isValidNode(*node)) {
            return false;
        }
    }
    if (selected_node_ == node) {
        return false;
    }
    const auto previous = selected_node_;
    selected_node_ = std::move(node);
    // The callback may destroy this object. No member access is permitted after notification.
    notifySelection(previous);
    return true;
}

bool TreeSelectionModel::clear() {
    return setSelected(std::nullopt);
}

bool TreeSelectionModel::select(const TreeNodePath& node) {
    return setSelected(node);
}

void TreeSelectionModel::modelChanged(const TreeModelChange& change) {
    if (!selected_node_.has_value() || change.kind == TreeModelChange::Kind::node_changed) {
        return;
    }
    const auto previous = selected_node_;
    selected_node_.reset();
    notifySelection(previous);
}

void TreeSelectionModel::notifySelection(std::optional<TreeNodePath> previous) {
    notifyObservers(observers_, {std::move(previous), selected_node_});
}

} // namespace sasd::ui
