#pragma once

#include <sasd/ui/component_metadata.hpp>

#include <span>
#include <string_view>

namespace sasd::ui {

/**
 * The small semantic value-kind vocabulary currently needed by the built-in property schema.
 *
 * This describes a property's schema only. It is not a value container and does not provide
 * access to a value held by a Component instance.
 * Integer tooling values use std::int64_t / std::uint64_t independently of concrete setter types.
 */
enum class ComponentPropertyValueKind {
    boolean,
    string,
    signed_integer,
    unsigned_integer,
    string_collection,
    optional_unsigned_integer,
    size_constraints,
    text_style,
};

/**
 * Immutable schema metadata for one directly declared Component property.
 *
 * `declaring_type_id` and `property_id` are stable, non-localized, case-sensitive identifiers.
 * Their pair identifies the property in this first schema contract. The descriptor owns neither
 * a Component nor a property value; both string views refer to immutable built-in catalog storage.
 */
struct ComponentPropertyMetadata final {
    std::string_view declaring_type_id{};
    std::string_view property_id{};
    ComponentPropertyValueKind value_kind{ComponentPropertyValueKind::boolean};
};

/** Returns the immutable properties declared directly by the exact built-in type ID. */
[[nodiscard]] std::span<const ComponentPropertyMetadata> declaredBuiltInComponentProperties(
    std::string_view type_id) noexcept;

/**
 * Finds a property on a built-in type, walking its direct metadata bases when necessary.
 *
 * Unknown types and properties return nullptr. The result identifies the type that directly
 * declares the property; inherited descriptors are not copied into derived-type catalogs.
 */
[[nodiscard]] const ComponentPropertyMetadata* findBuiltInComponentProperty(
    std::string_view type_id,
    std::string_view property_id) noexcept;

} // namespace sasd::ui
