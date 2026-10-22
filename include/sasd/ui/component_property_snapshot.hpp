#pragma once

#include <sasd/ui/component.hpp>

#include <optional>
#include <string>
#include <vector>

namespace sasd::ui {

/**
 * Owns one captured boolean property value and its stable schema identity.
 *
 * The snapshot intentionally owns all strings. It therefore remains valid after the source
 * Component and the immutable metadata catalogs have gone out of scope. `is_default` compares
 * the captured value with the effective default for the snapshot's exact target type.
 */
struct ComponentBooleanPropertySnapshot final {
    std::string declaring_type_id;
    std::string property_id;
    bool value{false};
    bool is_default{false};
};

/** Owns one captured string property value and its stable schema identity. */
struct ComponentStringPropertySnapshot final {
    std::string declaring_type_id;
    std::string property_id;
    std::string value;
    bool is_default{false};
};

/**
 * Owns all currently catalogued properties captured from one exact built-in Component type.
 *
 * This is deliberately not a complete Component or UI-tree description. A successful snapshot
 * promises only that every property currently present in the built-in property schema was read;
 * ownership, children, models, bindings and other semantic state remain outside this slice.
 */
struct BuiltInComponentPropertySnapshot final {
    std::string type_id;
    std::vector<ComponentBooleanPropertySnapshot> boolean_properties;
    std::vector<ComponentStringPropertySnapshot> string_properties;
};

/**
 * Captures all currently catalogued properties of an exact built-in Component.
 *
 * Unknown derived Components return std::nullopt. If any catalogued property cannot be read or
 * cannot resolve its effective default, the whole operation fails rather than returning a partial
 * snapshot that could silently omit state from a future tooling or serialization consumer.
 */
[[nodiscard]] std::optional<BuiltInComponentPropertySnapshot>
captureBuiltInComponentPropertySnapshot(const Component& component);

} // namespace sasd::ui
