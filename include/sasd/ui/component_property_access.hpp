#pragma once

#include <sasd/ui/component.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/geometry.hpp>
#include <sasd/ui/style.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui {

/**
 * Reads a built-in boolean property from a compatible Component.
 *
 * The descriptor is validated by value against the immutable built-in schema. Unknown, malformed or
 * incompatible descriptors/components return std::nullopt; no caller-supplied type assertion is
 * trusted. The returned value is a snapshot of the semantic Component state.
 */
[[nodiscard]] std::optional<bool> readBuiltInBooleanProperty(
    const Component& component,
    const ComponentPropertyMetadata& property);

/** Reads a built-in string property and returns an owned UTF-8 snapshot. */
[[nodiscard]] std::optional<std::string> readBuiltInStringProperty(
    const Component& component,
    const ComponentPropertyMetadata& property);

/**
 * Writes a built-in boolean property through its existing semantic setter.
 *
 * The result reports recognized, compatible access, not whether the value changed. Setter side
 * effects and callback semantics therefore remain owned by the concrete Component API.
 * RadioButton.selected delegates to setSelected(), including group exclusivity and onSelected;
 * the setter's state-change result is intentionally not the property-access result.
 */
[[nodiscard]] bool writeBuiltInBooleanProperty(
    Component& component,
    const ComponentPropertyMetadata& property,
    bool value);

/** Writes a built-in string property through its existing semantic setter. */
[[nodiscard]] bool writeBuiltInStringProperty(
    Component& component,
    const ComponentPropertyMetadata& property,
    std::string value);

/**
 * Reads a built-in signed_integer property into the fixed-width tooling type.
 * Invalid descriptors, incompatible Components or unrepresentable values return std::nullopt.
 */
[[nodiscard]] std::optional<std::int64_t> readBuiltInSignedIntegerProperty(
    const Component& component,
    const ComponentPropertyMetadata& property);

/**
 * Writes through the semantic setter after checking representability in its concrete C++ type.
 * Unrepresentable values return false without mutation. Domain exceptions from the setter propagate;
 * recognized same-value writes return true, just like boolean/string writes.
 */
[[nodiscard]] bool writeBuiltInSignedIntegerProperty(
    Component& component,
    const ComponentPropertyMetadata& property,
    std::int64_t value);

/**
 * Reads a built-in unsigned_integer property into the fixed-width tooling type.
 * Invalid descriptors, incompatible Components or unrepresentable values return std::nullopt.
 */
[[nodiscard]] std::optional<std::uint64_t> readBuiltInUnsignedIntegerProperty(
    const Component& component,
    const ComponentPropertyMetadata& property);

/**
 * Writes through the semantic setter after checking representability in its concrete C++ type.
 * Unrepresentable values return false without mutation. Domain exceptions from the setter propagate;
 * recognized same-value writes return true, just like boolean/string writes.
 */
[[nodiscard]] bool writeBuiltInUnsignedIntegerProperty(
    Component& component,
    const ComponentPropertyMetadata& property,
    std::uint64_t value);

/** Returns an owned collection snapshot; invalid/incompatible access returns nullopt. */
[[nodiscard]] std::optional<std::vector<std::string>> readBuiltInStringCollectionProperty(
    const Component& component, const ComponentPropertyMetadata& property);

/** Delegates to setItems(), including selection clearing and callbacks. True means valid access,
 * not a changed value. No member is accessed after the potentially destructive callback. */
[[nodiscard]] bool writeBuiltInStringCollectionProperty(
    Component& component, const ComponentPropertyMetadata& property, std::vector<std::string> value);

/**
 * Outer nullopt means invalid/incompatible access or an unrepresentable value. Engaged outer with
 * empty inner means valid access with no selection; engaged inner contains the selection index.
 * Two levels keep access failure distinct from semantic absence; no integer sentinel means none.
 */
[[nodiscard]] std::optional<std::optional<std::uint64_t>> readBuiltInOptionalUnsignedIntegerProperty(
    const Component& component, const ComponentPropertyMetadata& property);

/** Checks representation before setSelectedIndex(); domain exceptions propagate from the setter.
 * True means valid access, including same-value writes. Setter callbacks are never suppressed. */
[[nodiscard]] bool writeBuiltInOptionalUnsignedIntegerProperty(
    Component& component, const ComponentPropertyMetadata& property, std::optional<std::uint64_t> value);

/** Reads the existing backend-neutral public value by copy; invalid access returns nullopt.
 * SizeConstraints is one atomic authoring value, not three independently writable size hints. */
[[nodiscard]] std::optional<SizeConstraints> readBuiltInSizeConstraintsProperty(
    const Component& component, const ComponentPropertyMetadata& property);

/** Delegates atomically to setSizeConstraints(). True means compatible access, even for the same
 * value. The setter owns validation and measurement invalidation; std::invalid_argument propagates.
 * Preferred hints outside min/max and negative coordinates retain their existing public semantics. */
[[nodiscard]] bool writeBuiltInSizeConstraintsProperty(
    Component& component, const ComponentPropertyMetadata& property, SizeConstraints value);

/** Reads the inline authoring value by copy, never a backend-resolved focus/disabled overlay.
 * Only the six controls declaring textStyle own this property; it is not a Widget-wide contract. */
[[nodiscard]] std::optional<TextStyle> readBuiltInTextStyleProperty(
    const Component& component, const ComponentPropertyMetadata& property);

/** Delegates exclusively to setTextStyle(): visual invalidation and same-value no-op semantics
 * belong to the setter. True means compatible access, including same-value writes. No additional
 * Color validation is imposed on the existing runtime contract. Measurement remains valid. */
[[nodiscard]] bool writeBuiltInTextStyleProperty(
    Component& component, const ComponentPropertyMetadata& property, TextStyle value);

} // namespace sasd::ui
