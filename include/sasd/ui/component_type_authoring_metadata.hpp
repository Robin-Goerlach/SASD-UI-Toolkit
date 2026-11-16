#pragma once

#include <sasd/ui/component_metadata.hpp>

#include <optional>

namespace sasd::ui {

/**
 * Whether an exact built-in type may be authored as a concrete UI-description node.
 *
 * This is a tooling and authoring classification, not a general statement about C++ abstractness.
 * In particular, it does not expose a constructor, factory, reflection facility or persistence
 * opcode. Runtime reconstruction retains its separate, controlled construction policy.
 */
enum class ComponentTypeAuthoringInstantiationKind {
    /** The built-in type is currently accepted as a concrete UI-description node. */
    constructible,

    /** The type remains catalogued as a schema/reference contract but cannot be a concrete node. */
    abstract_contract,
};

/**
 * Machine-readable authoring information for one exact built-in type descriptor.
 *
 * The deliberately small value contains no display name, localized description, icon, palette
 * group, editor choice or other presentation policy. Those decisions belong to a real tooling
 * consumer. Known abstract contracts remain in the normal type catalog because property,
 * reference and schema tools still need their stable IDs and base relationships.
 */
struct ComponentTypeAuthoringMetadata final {
    ComponentTypeAuthoringInstantiationKind instantiation{
        ComponentTypeAuthoringInstantiationKind::constructible};

    friend constexpr bool operator==(const ComponentTypeAuthoringMetadata&,
                                     const ComponentTypeAuthoringMetadata&) = default;
};

/**
 * Returns authoring metadata for an exact built-in type descriptor.
 *
 * Descriptor identity is the complete type-id/base-type-id/kind value, not a catalog address, so
 * a copied valid descriptor succeeds. Unknown, malformed, wrong-case and fabricated descriptors
 * return std::nullopt. The mapping is exhaustive and explicit: a future catalog type must receive
 * a deliberate classification instead of silently becoming constructible.
 *
 * The query allocates nothing, constructs no Component, performs no resource lookup or backend
 * initialization, and has no registration, event, runtime-construction or persistence side effect.
 * It only reports whether authoring tools may use the known type as a concrete description node.
 */
[[nodiscard]] std::optional<ComponentTypeAuthoringMetadata>
builtInComponentTypeAuthoringMetadata(const ComponentTypeMetadata& type) noexcept;

} // namespace sasd::ui
