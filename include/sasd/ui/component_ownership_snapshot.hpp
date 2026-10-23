#pragma once

#include <sasd/ui/component_property_snapshot.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace sasd::ui {

/**
 * Identity within one ownership snapshot only; never a runtime, application or resource identity.
 * Captures assign dense IDs 0..N-1 in pre-order ownership traversal, with root ID 0. No identity
 * stability is promised across captures, even when capturing the same live Component again.
 */
using ComponentDescriptionNodeId = std::uint64_t;

/** Owns one node's catalogued properties and ordered ownership edges, with no runtime references. */
struct ComponentOwnershipNodeSnapshot final {
    ComponentDescriptionNodeId node_id{};
    // Reuse the property layer's exact type and values rather than maintaining duplicate state.
    BuiltInComponentPropertySnapshot properties;
    std::vector<ComponentDescriptionNodeId> owned_component_ids;
};

/**
 * Owned description of a complete built-in ownership tree, not a complete UI description.
 * The ordered edge lists define ownership; no redundant owner or visual-parent edges are stored.
 * Command/group/model/selection-model references and model payloads are deliberately absent.
 */
struct BuiltInComponentOwnershipSnapshot final {
    ComponentDescriptionNodeId root_node_id{};
    std::vector<ComponentOwnershipNodeSnapshot> nodes;
};

/**
 * Captures a complete, unowned root on the UI thread, without mutating the runtime tree.
 *
 * An owned root, unknown exact type, failed property capture, repeated node, inconsistent owner,
 * or owned Widget whose parent differs from its owning Container rejects the entire capture with
 * std::nullopt. Allocation failures propagate normally. Standalone non-Container roots are valid.
 * Runtime pointers are used only during capture for traversal/duplicate detection, never as IDs.
 * Visual parenting currently follows Container adoption and is checked rather than duplicated.
 */
[[nodiscard]] std::optional<BuiltInComponentOwnershipSnapshot>
captureBuiltInComponentOwnershipSnapshot(const Component& root);

} // namespace sasd::ui
