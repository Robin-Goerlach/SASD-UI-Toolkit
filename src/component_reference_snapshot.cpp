#include <sasd/ui/component_reference_snapshot.hpp>

#include <sasd/ui/button.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/component_runtime_metadata.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/tree_view.hpp>

#include <unordered_map>
#include <utility>

namespace sasd::ui {

std::optional<BuiltInComponentReferenceGraphSnapshot>
captureBuiltInComponentReferenceGraphSnapshot(const Component& root) {
    auto ownership = captureBuiltInComponentOwnershipSnapshot(root);
    if (!ownership) {
        return std::nullopt;
    }

    // Repeat the ownership pre-order without retaining pointers in any public value. Validate this
    // traversal against the captured IDs, types and explicit ownership lists before resolving refs.
    struct PendingNode final {
        const Component* component;
        const Container* owner;
        ComponentDescriptionNodeId node_id;
    };
    std::vector<PendingNode> pending{{&root, nullptr, ownership->root_node_id}};
    std::vector<const Component*> components;
    std::unordered_map<const Component*, ComponentDescriptionNodeId> nodeIds;
    while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        const auto index = components.size();
        if (index >= ownership->nodes.size()) {
            return std::nullopt;
        }
        const auto& node = ownership->nodes[index];
        const auto* type = builtInComponentTypeOf(*current.component);
        const auto* widget = dynamic_cast<const Widget*>(current.component);
        if (node.node_id != current.node_id || type == nullptr ||
            type->type_id != node.properties.type_id || current.component->owner() != current.owner ||
            (current.owner != nullptr && widget != nullptr && widget->parent() != current.owner) ||
            !nodeIds.emplace(current.component, node.node_id).second) {
            return std::nullopt;
        }
        components.push_back(current.component);
        const auto* container = dynamic_cast<const Container*>(current.component);
        const auto count = container != nullptr ? container->componentCount() : 0;
        if (count != node.owned_component_ids.size()) {
            return std::nullopt;
        }
        for (std::size_t child = count; child > 0; --child) {
            pending.push_back({&container->componentAt(child - 1), container,
                               node.owned_component_ids[child - 1]});
        }
    }
    if (components.size() != ownership->nodes.size()) {
        return std::nullopt;
    }

    BuiltInComponentReferenceGraphSnapshot result{std::move(*ownership), {}};
    for (std::size_t index = 0; index < components.size(); ++index) {
        const auto* component = components[index];
        const auto sourceId = result.ownership.nodes[index].node_id;
        const auto append = [&](const Component* target, BuiltInComponentReferenceKind kind) {
            // Safe getters also return null for expired bindings. Neither an absent relation nor
            // a lifetime tombstone has a reconstructible live target; neither produces an edge.
            if (target == nullptr) {
                return true;
            }
            const auto found = nodeIds.find(target);
            if (found == nodeIds.end()) {
                // Omitting an external live relation would silently change the described UI.
                // This first graph contract is self-contained, with no external resource IDs.
                return false;
            }
            result.reference_edges.push_back({sourceId, found->second, kind});
            return true;
        };
        // Typed getters enforce target compatibility. Explicit checks in enum order keep output
        // deterministic, independent of hash iteration and of today's single-relation source types.
        if (const auto* button = dynamic_cast<const Button*>(component)) {
            if (!append(button->boundCommand(), BuiltInComponentReferenceKind::button_command)) {
                return std::nullopt;
            }
        }
        if (const auto* radio = dynamic_cast<const RadioButton*>(component)) {
            if (!append(radio->group(), BuiltInComponentReferenceKind::radio_button_group)) {
                return std::nullopt;
            }
        }
        if (const auto* view = dynamic_cast<const ListView*>(component)) {
            if (!append(view->model(), BuiltInComponentReferenceKind::list_view_model)) {
                return std::nullopt;
            }
        }
        if (const auto* view = dynamic_cast<const TableView*>(component)) {
            if (!append(view->model(), BuiltInComponentReferenceKind::table_view_model)) {
                return std::nullopt;
            }
        }
        if (const auto* view = dynamic_cast<const TreeView*>(component)) {
            if (!append(view->model(), BuiltInComponentReferenceKind::tree_view_model)) {
                return std::nullopt;
            }
        }
        // Ownership is already composed, not repeated as reference edges. RadioGroup selection
        // derives from membership + selected properties; model data and SelectionModels stay out.
    }
    return result;
}

} // namespace sasd::ui
