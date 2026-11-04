#pragma once

#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/resource_reference.hpp>

#include <optional>

namespace sasd::ui {

/**
 * Resource-authoring capability attached to one exact built-in property descriptor.
 *
 * This type intentionally remains separate from ComponentPropertyMetadata. Schema metadata says
 * what value a property stores; resource metadata says whether authoring may identify a resource
 * of one concrete family for that exact slot. Keeping the layers separate avoids implying that
 * every future text_style property must support resources or that runtime Components perform
 * resource lookup.
 */
struct ComponentPropertyResourceMetadata final {
    ResourceKind resource_kind{ResourceKind::text_style};

    friend constexpr bool operator==(const ComponentPropertyResourceMetadata&,
                                     const ComponentPropertyResourceMetadata&) = default;
};

/**
 * Returns resource-authoring metadata for an exact built-in property descriptor.
 *
 * Descriptor identity is the full declaring-type/property/value-kind value, not a catalog pointer,
 * so a copied valid descriptor succeeds. The mapping is explicit rather than inferred from the
 * spelling "textStyle" or from ComponentPropertyValueKind::text_style: a future property can remain
 * inline-only unless its contract deliberately opts in.
 *
 * Unknown, malformed, wrong-kind and fabricated descriptors return std::nullopt. An engaged result
 * is only authoring metadata; it neither attaches a ResourceReference to a Component nor changes
 * the six controls' authoritative inline TextStyle runtime values. Resolution, scopes, persistence
 * and reconstruction remain later layers.
 */
[[nodiscard]] std::optional<ComponentPropertyResourceMetadata>
builtInComponentPropertyResourceMetadata(
    const ComponentPropertyMetadata& property) noexcept;

} // namespace sasd::ui
