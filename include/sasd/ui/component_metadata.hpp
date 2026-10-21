#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace sasd::ui {

/**
 * The deliberately small classification used by the built-in component-type catalog.
 *
 * This is a tooling-facing architectural classification, not a replacement for the C++ type
 * system and not a taxonomy of every future widget or service category.
 */
enum class ComponentTypeKind {
    component,
    widget,
    container,
};

/**
 * Immutable metadata for one toolkit component type.
 *
 * `type_id` and `base_type_id` are explicit, backend-neutral toolkit identifiers. They are views
 * into static catalog storage and therefore remain valid for the lifetime of the process. An empty
 * `base_type_id` means that this descriptor is the root `Component` descriptor. This value only
 * describes a type; it does not own or identify a Component instance and it does not provide a
 * factory, property list or serializer.
 */
struct ComponentTypeMetadata final {
    std::string_view type_id{};
    std::string_view base_type_id{};
    ComponentTypeKind kind{ComponentTypeKind::component};
};

/**
 * Returns the immutable catalog of component types built into this version of the toolkit.
 *
 * The catalog contains the public Component-derived types implemented by the Core. It has no
 * registration side effects and no ownership relationship with component instances.
 */
[[nodiscard]] std::span<const ComponentTypeMetadata> builtInComponentTypes() noexcept;

/**
 * Finds a built-in descriptor by its exact, case-sensitive toolkit type ID.
 *
 * Unknown IDs are ordinary query misses and return nullptr. This function does not attempt RTTI,
 * compiler-name demangling or fallback classification for external derived C++ types.
 */
[[nodiscard]] const ComponentTypeMetadata* findBuiltInComponentType(
    std::string_view type_id) noexcept;

} // namespace sasd::ui
