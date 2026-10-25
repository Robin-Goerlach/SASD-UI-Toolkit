#pragma once

#include <sasd/ui/component_ownership_snapshot.hpp>

namespace sasd::ui {

/** Closed set of persistent built-in Component relations, separate from ownership/properties. */
enum class BuiltInComponentReferenceKind {
    button_command,
    radio_button_group,
    list_view_model,
    table_view_model,
    tree_view_model,
    list_view_selection_model,
    table_view_selection_model,
    tree_view_selection_model,
    list_selection_model_model,
    table_selection_model_model,
    tree_selection_model_model,
};

/** Endpoints are description-local IDs in the containing graph's ownership snapshot. */
struct ComponentReferenceEdgeSnapshot final {
    ComponentDescriptionNodeId source_node_id{};
    ComponentDescriptionNodeId target_node_id{};
    BuiltInComponentReferenceKind kind{BuiltInComponentReferenceKind::button_command};
};

/** Owns all values; ownership has exactly one representation, in the composed snapshot. */
struct BuiltInComponentReferenceGraphSnapshot final {
    BuiltInComponentOwnershipSnapshot ownership;
    std::vector<ComponentReferenceEdgeSnapshot> reference_edges;
};

/**
 * Captures on the UI thread under the ownership capture's complete-built-in-tree contract.
 * Every live supported target must belong to the same ownership snapshot, otherwise the entire
 * capture returns std::nullopt. Null/unbound/expired getters produce no edge: runtime failure
 * history is not a durable relation. No runtime pointers escape. Allocation failures propagate.
 * Edges follow source node ID order, then the fixed reference-kind order declared above.
 * View/model and selection/model bindings are independent; capture imposes no equality invariant.
 * SelectionModel identity/sharing uses ordinary Component nodes; current selection is excluded.
 * No model payloads, selectedButton back-reference, external IDs or reconstruction are provided.
 */
[[nodiscard]] std::optional<BuiltInComponentReferenceGraphSnapshot>
captureBuiltInComponentReferenceGraphSnapshot(const Component& root);

} // namespace sasd::ui
