#include <sasd/ui/tree_model.hpp>

#include <algorithm>
#include <limits>

namespace sasd::ui {

TreeNodePath TreeNodePath::parent() const {
    TreeNodePath result = *this;
    if (!result.indices.empty()) {
        result.indices.pop_back();
    }
    return result;
}

TreeNodePath TreeNodePath::child(std::size_t index) const {
    TreeNodePath result = *this;
    result.indices.push_back(index);
    return result;
}

TreeModel::~TreeModel() {
    // References may outlive this object. Clearing the back-reference before Component destruction
    // makes every later lookup fail closed, without touching derived storage after its lifetime.
    if (observers_) {
        observers_->model = nullptr;
    }
}

TreeModel* TreeModel::Reference::get() const noexcept {
    const auto state = state_.lock();
    return state ? state->model : nullptr;
}

bool TreeModel::Subscription::connected() const noexcept {
    const auto slot = slot_.lock();
    return slot != nullptr && slot->active;
}

void TreeModel::Subscription::reset() noexcept {
    if (const auto slot = slot_.lock()) {
        slot->active = false;
    }
    slot_.reset();
}

std::shared_ptr<TreeModel::ObserverState> TreeModel::ensureObserverState() {
    if (!observers_) {
        observers_ = std::make_shared<ObserverState>();
        observers_->model = this;
    }
    return observers_;
}

TreeModel::Reference TreeModel::reference() {
    return Reference{ensureObserverState()};
}

TreeModel::Subscription TreeModel::observe(ChangedHandler handler) {
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

void TreeModel::compactInactiveObservers(const std::shared_ptr<ObserverState>& state) {
    if (!state) {
        return;
    }
    state->slots.erase(
        std::remove_if(state->slots.begin(), state->slots.end(), [](const auto& slot) {
            return !slot || !slot->active;
        }),
        state->slots.end());
}

void TreeModel::notifyObservers(std::shared_ptr<ObserverState> state,
                                const TreeModelChange& change) {
    if (!state) {
        return;
    }
    compactInactiveObservers(state);
    const auto snapshot = state->slots;
    for (const auto& slot : snapshot) {
        if (!slot || !slot->active) {
            continue;
        }
        // Copy the callback before invocation. It may disconnect peers or destroy the model owner;
        // no model member is accessed by this loop after application code starts running.
        const auto handler = slot->handler;
        if (handler) {
            handler(change);
        }
    }
}

bool TreeModel::isValidNode(const TreeNodePath& node) const noexcept {
    if (node.empty()) {
        return false;
    }
    TreeNodePath parent;
    for (const auto index : node.indices) {
        if (index >= childCount(parent)) {
            return false;
        }
        parent.indices.push_back(index);
    }
    return true;
}

void TreeModel::notifyChanged(TreeModelChange change) {
    // Mutation is complete before revision and notification. An observer may destroy this owner,
    // therefore no member access is permitted after notifyObservers begins.
    if (revision_ != std::numeric_limits<std::uint64_t>::max()) {
        ++revision_;
    }
    notifyObservers(observers_, change);
}

StringTreeModel::StringTreeModel(std::vector<std::string> roots) {
    reset(std::move(roots));
}

const std::vector<StringTreeModel::Node>*
StringTreeModel::childrenAt(const TreeNodePath& parent) const noexcept {
    if (parent.empty()) {
        return &roots_;
    }
    const auto* node = nodeAt(parent);
    return node == nullptr ? nullptr : &node->children;
}

std::vector<StringTreeModel::Node>* StringTreeModel::childrenAt(const TreeNodePath& parent) noexcept {
    if (parent.empty()) {
        return &roots_;
    }
    auto* node = nodeAt(parent);
    return node == nullptr ? nullptr : &node->children;
}

const StringTreeModel::Node* StringTreeModel::nodeAt(const TreeNodePath& path) const noexcept {
    const std::vector<Node>* children = &roots_;
    const Node* result = nullptr;
    for (const auto index : path.indices) {
        if (index >= children->size()) {
            return nullptr;
        }
        result = &(*children)[index];
        children = &result->children;
    }
    return result;
}

StringTreeModel::Node* StringTreeModel::nodeAt(const TreeNodePath& path) noexcept {
    return const_cast<Node*>(std::as_const(*this).nodeAt(path));
}

std::size_t StringTreeModel::childCount(const TreeNodePath& parent) const noexcept {
    const auto* children = childrenAt(parent);
    return children == nullptr ? 0 : children->size();
}

std::string_view StringTreeModel::textAt(const TreeNodePath& node) const {
    const auto* value = nodeAt(node);
    if (value == nullptr) {
        throw std::out_of_range{"StringTreeModel node out of range"};
    }
    return value->text;
}

TreeNodePath StringTreeModel::append(const TreeNodePath& parent, std::string text) {
    auto* children = childrenAt(parent);
    if (children == nullptr) {
        throw std::out_of_range{"StringTreeModel parent out of range"};
    }
    const auto index = children->size();
    children->push_back({std::move(text), {}});
    notifyChanged({TreeModelChange::Kind::reset, {}});
    return parent.child(index);
}

void StringTreeModel::setText(const TreeNodePath& node, std::string text) {
    auto* value = nodeAt(node);
    if (value == nullptr) {
        throw std::out_of_range{"StringTreeModel node out of range"};
    }
    if (value->text == text) {
        return;
    }
    value->text = std::move(text);
    notifyChanged({TreeModelChange::Kind::node_changed, node});
}

void StringTreeModel::reset(std::vector<std::string> roots) {
    std::vector<Node> replacement;
    replacement.reserve(roots.size());
    for (auto& text : roots) {
        replacement.push_back({std::move(text), {}});
    }
    roots_ = std::move(replacement);
    notifyChanged({TreeModelChange::Kind::reset, {}});
}

} // namespace sasd::ui
