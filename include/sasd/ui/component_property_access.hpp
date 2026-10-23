#pragma once

#include <sasd/ui/component.hpp>
#include <sasd/ui/component_property_metadata.hpp>

#include <cstdint>
#include <optional>
#include <string>

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

} // namespace sasd::ui
