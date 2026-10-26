#pragma once

#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/geometry.hpp>
#include <sasd/ui/style.hpp>

#include <cstdint>
#include <optional>
#include <span>
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

/**
 * Returns the effective signed_integer default without constructing a Component.
 * Unknown targets, wrong kinds or incompatible descriptor identities return std::nullopt.
 */
[[nodiscard]] std::optional<std::int64_t> builtInSignedIntegerPropertyDefault(
    std::string_view target_type_id,
    const ComponentPropertyMetadata& property) noexcept;

/**
 * Returns the effective unsigned_integer default without constructing a Component.
 * Unknown targets, wrong kinds or incompatible descriptor identities return std::nullopt.
 */
[[nodiscard]] std::optional<std::uint64_t> builtInUnsignedIntegerPropertyDefault(
    std::string_view target_type_id,
    const ComponentPropertyMetadata& property) noexcept;

/** Allocation-free immutable default collection; outer nullopt means an invalid query. */
[[nodiscard]] std::optional<std::span<const std::string_view>> builtInStringCollectionPropertyDefault(
    std::string_view target_type_id, const ComponentPropertyMetadata& property) noexcept;

/** Outer nullopt means invalid query; engaged outer and empty inner means the valid default none.
 * No sentinel index represents absence. Defaults never construct a Component. */
[[nodiscard]] std::optional<std::optional<std::uint64_t>> builtInOptionalUnsignedIntegerPropertyDefault(
    std::string_view target_type_id, const ComponentPropertyMetadata& property) noexcept;

/** Returns SizeConstraints{} for Widget and its built-in metadata descendants without construction.
 * Unknown targets, incompatible descriptors and wrong kinds return nullopt. */
[[nodiscard]] std::optional<SizeConstraints> builtInSizeConstraintsPropertyDefault(
    std::string_view target_type_id, const ComponentPropertyMetadata& property) noexcept;

/** Returns TextStyle{} for the six direct owning controls, without constructing a Component.
 * Copied complete identities are accepted; sibling, unknown and malformed identities fail closed. */
[[nodiscard]] std::optional<TextStyle> builtInTextStylePropertyDefault(
    std::string_view target_type_id, const ComponentPropertyMetadata& property) noexcept;

} // namespace sasd::ui
