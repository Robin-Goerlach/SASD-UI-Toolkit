#pragma once

#include <sasd/ui/component.hpp>
#include <sasd/ui/component_metadata.hpp>

namespace sasd::ui {

/**
 * Returns the exact built-in descriptor for a live Component instance.
 *
 * This is intentionally narrower than base-class compatibility: a Button is identified as a
 * Button, while an application-defined Widget-derived type is unknown. Returning nullptr for an
 * unknown derived type prevents a future serializer from silently treating that object as a
 * complete built-in base and losing derived state.
 *
 * The query is allocation-free and does not change ownership, parenting or component state. RTTI
 * is an implementation detail of the query; the returned stable toolkit type ID is the public
 * identity, and no compiler RTTI name is exposed or persisted.
 */
[[nodiscard]] const ComponentTypeMetadata* builtInComponentTypeOf(
    const Component& component) noexcept;

} // namespace sasd::ui
