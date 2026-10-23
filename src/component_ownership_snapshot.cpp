#include <sasd/ui/component_ownership_snapshot.hpp>

#include <sasd/ui/container.hpp>

#include <unordered_set>
#include <utility>

namespace sasd::ui {

std::optional<BuiltInComponentOwnershipSnapshot>
captureBuiltInComponentOwnershipSnapshot(const Component& root) {
    // Subtree capture would silently omit the root's persistent owner relationship.
    if (root.owner() != nullptr) {
        return std::nullopt;
    }

    struct PendingNode final {
        const Component* component;
        const Container* owner;
        std::size_t owner_node_index;
    };

    BuiltInComponentOwnershipSnapshot snapshot;
    std::vector<PendingNode> pending{{&root, nullptr, 0}};
    std::unordered_set<const Component*> visited;
    while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        // Pointer identity is a temporary integrity check, never description identity. A duplicate
        // also catches cycles, even if future ownership internals permit malformed structures.
        if (!visited.insert(current.component).second ||
            current.component->owner() != current.owner) {
            return std::nullopt;
        }
        const auto* widget = dynamic_cast<const Widget*>(current.component);
        if (current.owner != nullptr && widget != nullptr && widget->parent() != current.owner) {
            return std::nullopt;
        }

        auto properties = captureBuiltInComponentPropertySnapshot(*current.component);
        if (!properties.has_value() ||
            !std::in_range<ComponentDescriptionNodeId>(snapshot.nodes.size())) {
            return std::nullopt;
        }
        const auto node_index = snapshot.nodes.size();
        const auto node_id = static_cast<ComponentDescriptionNodeId>(node_index);
        snapshot.nodes.push_back({node_id, std::move(*properties), {}});
        if (current.owner != nullptr) {
            snapshot.nodes[current.owner_node_index].owned_component_ids.push_back(node_id);
        }

        const auto* container = dynamic_cast<const Container*>(current.component);
        if (container != nullptr) {
            // Ownership order is persistent, including non-visual Commands, groups and models.
            // childAt() would lose these nodes. Reverse pushing preserves pre-order on this stack;
            // consumers still use the explicit edge lists, not global node-vector adjacency.
            for (std::size_t index = container->componentCount(); index > 0; --index) {
                pending.push_back({&container->componentAt(index - 1), container, node_index});
            }
        }
    }
    // All intermediate values stay local: a failed descendant can never expose a partial snapshot.
    return snapshot;
}

} // namespace sasd::ui
