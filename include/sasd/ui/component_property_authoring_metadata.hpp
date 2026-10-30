#pragma once

#include <sasd/ui/component_property_metadata.hpp>

#include <optional>

namespace sasd::ui {

/**
 * Stable, non-localized semantic groups for property-authoring tools.
 *
 * These values are machine-readable Core identifiers, not display strings, persistence values or
 * a prescribed Property Grid order. A designer is expected to localize and arrange the categories
 * for its own users. In particular, the enum ordinals must never be written to a UI description or
 * treated as a stable file-format/ABI contract.
 */
enum class ComponentPropertyAuthoringCategory {
    /** Pure visual presentation or style, currently including inline textStyle properties. */
    appearance,

    /** Participation in behavior or interaction, such as visible, enabled and focusable. */
    behavior,

    /** Author-provided semantic content, such as text and ComboBox items. */
    content,

    /** Geometry, constraint and spacing configuration. */
    layout,

    /** Explicitly persisted initial control/selection state, such as checked or selected. */
    state,
};

/**
 * Authoring information associated with one exact built-in property descriptor.
 *
 * Schema metadata answers what a property technically is. This separate type answers only how an
 * authoring tool may group it. It intentionally contains no localized display/help text, editor
 * preference, ordering, browsable/read-only/advanced flag or runtime value.
 */
struct ComponentPropertyAuthoringMetadata final {
    ComponentPropertyAuthoringCategory category{
        ComponentPropertyAuthoringCategory::behavior};

    friend constexpr bool operator==(const ComponentPropertyAuthoringMetadata&,
                                     const ComponentPropertyAuthoringMetadata&) = default;
};

/**
 * Returns authoring metadata for an exact built-in property descriptor.
 *
 * Descriptor identity is the complete declaring-type/property/kind value, not a catalog address,
 * so a copied valid descriptor succeeds. Inherited properties keep the metadata of the property
 * that declares them: for example, Button.visible resolves to the Widget.visible descriptor and is
 * categorized as behavior without creating a Button-specific duplicate.
 *
 * Unknown, malformed, wrong-kind and fabricated descriptors return std::nullopt. The mapping is
 * explicit rather than inferred from property names; a future property therefore requires a
 * deliberate authoring decision instead of accidentally matching a spelling heuristic. The query
 * allocates nothing, constructs no Component and has no runtime or persistence effect.
 */
[[nodiscard]] std::optional<ComponentPropertyAuthoringMetadata>
builtInComponentPropertyAuthoringMetadata(
    const ComponentPropertyMetadata& property) noexcept;

} // namespace sasd::ui
