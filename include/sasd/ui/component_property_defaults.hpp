#pragma once

#include <sasd/ui/component_property_metadata.hpp>

#include <optional>
#include <string_view>

namespace sasd::ui {

/**
 * Returns the effective built-in boolean default for a property on a target built-in type.
 *
 * The target type is part of the query because an inherited property can have a different
 * effective default on a derived built-in type. Invalid descriptors are ordinary lookup misses.
 */
[[nodiscard]] std::optional<bool> builtInBooleanPropertyDefault(
    std::string_view target_type_id,
    const ComponentPropertyMetadata& property) noexcept;

/**
 * Returns the effective built-in string default for a property on a target built-in type.
 *
 * The returned view refers to immutable built-in storage and remains valid for the lifetime of the
 * process. It does not refer to a Component instance and does not transfer ownership.
 */
[[nodiscard]] std::optional<std::string_view> builtInStringPropertyDefault(
    std::string_view target_type_id,
    const ComponentPropertyMetadata& property) noexcept;

} // namespace sasd::ui
