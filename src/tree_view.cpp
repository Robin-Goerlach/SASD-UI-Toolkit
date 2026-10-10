#include <sasd/ui/tree_view.hpp>

#include <algorithm>
#include <limits>
#include <variant>

namespace sasd::ui {

TreeView::TreeView() {
    setFocusable(true);
}

TreeView::~TreeView() = default;

void TreeView::setModel(TreeModel* model) {
    if (model_.get() == model) {
        return;
    }
    model_subscription_.reset();
    model_ = model != nullptr ? model->reference() : TreeModel::Reference{};
    expanded_nodes_.clear();
    first_visible_node_.reset();
    if (model != nullptr) {
        model_subscription_ = model->observe([this](const TreeModelChange& change) {
            modelChanged(change);
        });
    }
    normalizeViewport();
    keepSelectionVisible();
    invalidateMeasure();
    invalidateVisual();
}

void TreeView::setSelectionModel(TreeSelectionModel* selection_model) {
    if (selection_model_.get() == selection_model) {
        return;
    }
    selection_subscription_.reset();
    selection_model_ = selection_model != nullptr ? selection_model->reference()
                                                    : TreeSelectionModel::Reference{};
    if (selection_model != nullptr) {
        selection_subscription_ = selection_model->observe([this](const TreeSelectionChange& change) {
            selectionChanged(change);
        });
    }
    keepSelectionVisible();
    invalidateVisual();
}

void TreeView::setViewport(std::optional<TreeNodePath> first_node, std::size_t row_count) {
    first_visible_node_ = std::move(first_node);
    visible_row_count_ = row_count;
    normalizeViewport();
    invalidateVisual();
}

void TreeView::normalizeViewport() noexcept {
    const auto* model = model_.get();
    if (model == nullptr || !first_visible_node_.has_value()) {
        return;
    }
    if (!model->isValidNode(*first_visible_node_) || !isVisibleNode(*first_visible_node_)) {
        first_visible_node_.reset();
    }
}

bool TreeView::isExpanded(const TreeNodePath& node) const noexcept {
    return std::find(expanded_nodes_.begin(), expanded_nodes_.end(), node) != expanded_nodes_.end();
}

bool TreeView::expand(const TreeNodePath& node) {
    const auto* model = model_.get();
    if (model == nullptr || !model->isValidNode(node) || model->childCount(node) == 0 ||
        isExpanded(node)) {
        return false;
    }
    expanded_nodes_.push_back(node);
    invalidateVisual();
    return true;
}

bool TreeView::collapse(const TreeNodePath& node) {
    const auto position = std::find(expanded_nodes_.begin(), expanded_nodes_.end(), node);
    if (position == expanded_nodes_.end()) {
        return false;
    }
    expanded_nodes_.erase(position);
    const auto selected = selection_model_.get();
    const auto selected_node = selected != nullptr ? selected->selectedNode() : std::nullopt;
    if (selected_node.has_value() && isDescendantOf(*selected_node, node) && selected != nullptr &&
        selected->model() == model_.get()) {
        // A hidden descendant must not remain the semantic selection. The collapsed parent is the
        // only visible, deterministic replacement; selection callbacks may be destructive, so no
        // TreeView member access is made after this call in this branch.
        (void)selected->select(node);
    }
    invalidateVisual();
    return true;
}

bool TreeView::toggleExpanded(const TreeNodePath& node) {
    return isExpanded(node) ? collapse(node) : expand(node);
}

std::optional<TreeNodePath> TreeView::firstVisibleCandidate() const {
    const auto* model = model_.get();
    if (model == nullptr || model->childCount(TreeNodePath{}) == 0) {
        return std::nullopt;
    }
    if (first_visible_node_.has_value() && isVisibleNode(*first_visible_node_)) {
        return first_visible_node_;
    }
    return TreeNodePath{}.child(0);
}

bool TreeView::isVisibleNode(const TreeNodePath& node) const {
    const auto* model = model_.get();
    if (model == nullptr || !model->isValidNode(node)) {
        return false;
    }
    auto ancestor = node.parent();
    while (!ancestor.empty()) {
        if (!isExpanded(ancestor)) {
            return false;
        }
        ancestor = ancestor.parent();
    }
    return true;
}

std::optional<TreeNodePath> TreeView::nextVisible(const TreeNodePath& node) const {
    const auto* model = model_.get();
    if (model == nullptr || !model->isValidNode(node)) {
        return std::nullopt;
    }
    if (isExpanded(node) && model->childCount(node) != 0) {
        return node.child(0);
    }

    auto current = node;
    while (!current.empty()) {
        const auto parent = current.parent();
        const auto index = current.indices.back();
        const auto siblings = model->childCount(parent);
        if (index < siblings && index + 1 < siblings) {
            return parent.child(index + 1);
        }
        current = parent;
    }
    return std::nullopt;
}

std::optional<TreeNodePath> TreeView::previousVisible(const TreeNodePath& node) const {
    const auto* model = model_.get();
    if (model == nullptr || !model->isValidNode(node)) {
        return std::nullopt;
    }
    const auto parent = node.parent();
    const auto index = node.indices.back();
    if (index == 0) {
        return parent.empty() ? std::nullopt : std::optional<TreeNodePath>{parent};
    }

    auto candidate = parent.child(index - 1);
    while (isExpanded(candidate)) {
        const auto count = model->childCount(candidate);
        if (count == 0) {
            break;
        }
        candidate = candidate.child(count - 1);
    }
    return candidate;
}

std::vector<TreeViewRow> TreeView::visibleRows() const {
    std::vector<TreeViewRow> result;
    if (visible_row_count_ == 0) {
        return result;
    }
    auto current = firstVisibleCandidate();
    result.reserve(visible_row_count_);
    for (std::size_t row = 0; row < visible_row_count_ && current.has_value(); ++row) {
        const auto* model = model_.get();
        if (model == nullptr || !model->isValidNode(*current)) {
            break;
        }
        const auto child_count = model->childCount(*current);
        const auto selected = selection_model_.get();
        const auto selected_node = selected != nullptr && selected->model() == model
                                       ? selected->selectedNode()
                                       : std::nullopt;
        result.push_back({*current,
                          current->depth(),
                          std::string{model->textAt(*current)},
                          child_count != 0,
                          isExpanded(*current),
                          selected_node.has_value() && *selected_node == *current});
        current = nextVisible(*current);
    }
    return result;
}

bool TreeView::isDescendantOf(const TreeNodePath& node,
                              const TreeNodePath& ancestor) const noexcept {
    return node.indices.size() > ancestor.indices.size() &&
           std::equal(ancestor.indices.begin(), ancestor.indices.end(), node.indices.begin());
}

void TreeView::keepSelectionVisible() {
    const auto* model = model_.get();
    const auto* selection = selection_model_.get();
    const auto selected = selection != nullptr ? selection->selectedNode() : std::nullopt;
    if (!selected.has_value() || visible_row_count_ == 0) {
        return;
    }
    auto current = firstVisibleCandidate();
    if (!current.has_value()) {
        first_visible_node_ = selected;
        return;
    }
    for (std::size_t index = 0; index < visible_row_count_ && current.has_value(); ++index) {
        if (*current == *selected) {
            first_visible_node_ = current;
            return;
        }
        current = nextVisible(*current);
    }
    // The selected node may be outside the current range. Using it as the new anchor is exact and
    // avoids flattening or counting every hidden row just to scroll it into view.
    if (model != nullptr && selection->model() == model && model->isValidNode(*selected)) {
        first_visible_node_ = selected;
    }
}

bool TreeView::moveSelection(Key key) {
    auto* selection = selection_model_.get();
    const auto* model = model_.get();
    if (selection == nullptr || selection->model() != model || model == nullptr) {
        return false;
    }
    const auto selected = selection->selectedNode();
    switch (key) {
    case Key::up:
        return selected.has_value() && previousVisible(*selected).has_value() &&
               selection->select(*previousVisible(*selected));
    case Key::down: {
        const auto next = selected.has_value() ? nextVisible(*selected) : firstVisibleCandidate();
        return next.has_value() && selection->select(*next);
    }
    case Key::right:
        if (!selected.has_value() || model->childCount(*selected) == 0) {
            return false;
        }
        if (!isExpanded(*selected)) {
            return expand(*selected);
        }
        return selection->select(selected->child(0));
    case Key::left: {
        if (!selected.has_value()) {
            return false;
        }
        if (isExpanded(*selected)) {
            return collapse(*selected);
        }
        const auto parent = selected->parent();
        return !parent.empty() && selection->select(parent);
    }
    default:
        return false;
    }
}

EventResult TreeView::onEvent(const Event& event) {
    const auto* key_event = std::get_if<KeyEvent>(&event);
    if (key_event == nullptr || !key_event->pressed ||
        key_event->modifiers != KeyModifier::none || !hasFocus() || !isVisible() || !isEnabled()) {
        return EventResult::ignored;
    }
    return moveSelection(key_event->key) ? EventResult::handled : EventResult::ignored;
}

void TreeView::modelChanged(const TreeModelChange& change) {
    if (change.kind == TreeModelChange::Kind::reset) {
        expanded_nodes_.clear();
        first_visible_node_.reset();
    }
    normalizeViewport();
    keepSelectionVisible();
    invalidateMeasure();
    invalidateVisual();
}

void TreeView::selectionChanged(const TreeSelectionChange&) {
    keepSelectionVisible();
    invalidateVisual();
}

} // namespace sasd::ui
