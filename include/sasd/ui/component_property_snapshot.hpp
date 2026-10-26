#pragma once

#include <sasd/ui/component.hpp>
#include <sasd/ui/geometry.hpp>
#include <sasd/ui/style.hpp>

#include <cstdint>
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

/** Owns one captured signed_integer value and its stable schema identity. */
struct ComponentSignedIntegerPropertySnapshot final {
    std::string declaring_type_id;
    std::string property_id;
    std::int64_t value{0};
    bool is_default{false};
};

/** Owns one captured unsigned_integer value and its stable schema identity. */
struct ComponentUnsignedIntegerPropertySnapshot final {
    std::string declaring_type_id;
    std::string property_id;
    std::uint64_t value{0};
    bool is_default{false};
};

/** Owns every string in a captured collection, independently of the source Component. */
struct ComponentStringCollectionPropertySnapshot final {
    std::string declaring_type_id;
    std::string property_id;
    std::vector<std::string> value;
    bool is_default{false};
};

/** A successful capture needs only one optional: nullopt here means no selection, not failure. */
struct ComponentOptionalUnsignedIntegerPropertySnapshot final {
    std::string declaring_type_id;
    std::string property_id;
    std::optional<std::uint64_t> value;
    bool is_default{false};
};

/** Owns the existing public compound value; no borrowed geometry or measurement cache is retained. */
struct ComponentSizeConstraintsPropertySnapshot final {
    std::string declaring_type_id;
    std::string property_id;
    SizeConstraints value;
    bool is_default{false};
};

/** Owns the small inline authoring value. No resource identity or backend overlay is captured. */
struct ComponentTextStylePropertySnapshot final {
    std::string declaring_type_id;
    std::string property_id;
    TextStyle value;
    bool is_default{false};
};

/**
 * Owns all currently catalogued properties captured from one exact built-in Component type.
 *
 * This is deliberately not a complete Component or UI-tree description. A successful snapshot
 * promises only that every property currently present in the built-in property schema was read;
 * ownership, children, models, bindings and other semantic state remain outside this value.
 * Kind-separated vectors do not encode global reconstruction order. Future reconstruction must
 * restore ComboBox.items before selectedIndex: setItems clears selection and setSelectedIndex
 * validates against current items. This snapshot implements no reconstruction engine.
 */
struct BuiltInComponentPropertySnapshot final {
    std::string type_id;
    std::vector<ComponentBooleanPropertySnapshot> boolean_properties;
    std::vector<ComponentStringPropertySnapshot> string_properties;
    std::vector<ComponentSignedIntegerPropertySnapshot> signed_integer_properties;
    std::vector<ComponentUnsignedIntegerPropertySnapshot> unsigned_integer_properties;
    std::vector<ComponentStringCollectionPropertySnapshot> string_collection_properties;
    std::vector<ComponentOptionalUnsignedIntegerPropertySnapshot> optional_unsigned_integer_properties;
    std::vector<ComponentSizeConstraintsPropertySnapshot> size_constraints_properties;
    std::vector<ComponentTextStylePropertySnapshot> text_style_properties;
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
