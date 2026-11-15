#pragma once

#include <sasd/ui/component_reference_snapshot.hpp>
#include <sasd/ui/geometry.hpp>
#include <sasd/ui/resource_dictionary.hpp>
#include <sasd/ui/resource_reference.hpp>
#include <sasd/ui/style.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui {

/**
 * Persistent, owning values, independent of the encoding and of runtime Components.
 * Capture snapshots deliberately remain separate: is_default is capture/tooling information
 * absent from persisted data. Loading must neither invent it nor consult current defaults.
 * Every string owns its storage, so the input buffer may be released immediately after parsing.
 */
struct UiDescriptionBooleanProperty final {
    std::string declaring_type_id;
    std::string property_id;
    bool value{false};

    friend bool operator==(const UiDescriptionBooleanProperty&,
                           const UiDescriptionBooleanProperty&) = default;
};

struct UiDescriptionStringProperty final {
    std::string declaring_type_id;
    std::string property_id;
    std::string value;

    friend bool operator==(const UiDescriptionStringProperty&,
                           const UiDescriptionStringProperty&) = default;
};

struct UiDescriptionSignedIntegerProperty final {
    std::string declaring_type_id;
    std::string property_id;
    std::int64_t value{0};

    friend bool operator==(const UiDescriptionSignedIntegerProperty&,
                           const UiDescriptionSignedIntegerProperty&) = default;
};

struct UiDescriptionUnsignedIntegerProperty final {
    std::string declaring_type_id;
    std::string property_id;
    std::uint64_t value{0};

    friend bool operator==(const UiDescriptionUnsignedIntegerProperty&,
                           const UiDescriptionUnsignedIntegerProperty&) = default;
};

struct UiDescriptionStringCollectionProperty final {
    std::string declaring_type_id;
    std::string property_id;
    std::vector<std::string> value;

    friend bool operator==(const UiDescriptionStringCollectionProperty&,
                           const UiDescriptionStringCollectionProperty&) = default;
};

struct UiDescriptionOptionalUnsignedIntegerProperty final {
    std::string declaring_type_id;
    std::string property_id;
    std::optional<std::uint64_t> value;

    friend bool operator==(const UiDescriptionOptionalUnsignedIntegerProperty&,
                           const UiDescriptionOptionalUnsignedIntegerProperty&) = default;
};

struct UiDescriptionSizeConstraintsProperty final {
    std::string declaring_type_id;
    std::string property_id;
    SizeConstraints value;

    friend bool operator==(const UiDescriptionSizeConstraintsProperty&,
                           const UiDescriptionSizeConstraintsProperty&) = default;
};

struct UiDescriptionTextStyleProperty final {
    std::string declaring_type_id;
    std::string property_id;
    TextStyle value;

    friend bool operator==(const UiDescriptionTextStyleProperty&,
                           const UiDescriptionTextStyleProperty&) = default;
};

/**
 * Durable authoring value for one resource-capable property.
 *
 * This value deliberately sits beside, rather than inside, the concrete inline property
 * collections. The document validator enforces that an exact descriptor occurs either inline or
 * here, never in both places. ResourceReference remains a typed identity rather than becoming a
 * generic payload variant.
 */
struct UiDescriptionResourceReferenceProperty final {
    std::string declaring_type_id;
    std::string property_id;
    ResourceReference reference;

    friend bool operator==(const UiDescriptionResourceReferenceProperty&,
                           const UiDescriptionResourceReferenceProperty&) = default;
};

/** Ordered ownership, eight inline collections and durable references; no reconstruction order implied. */
struct BuiltInUiDescriptionNode final {
    ComponentDescriptionNodeId node_id{};
    std::string type_id;
    std::vector<ComponentDescriptionNodeId> owned_component_ids;
    std::vector<UiDescriptionBooleanProperty> boolean_properties;
    std::vector<UiDescriptionStringProperty> string_properties;
    std::vector<UiDescriptionSignedIntegerProperty> signed_integer_properties;
    std::vector<UiDescriptionUnsignedIntegerProperty> unsigned_integer_properties;
    std::vector<UiDescriptionStringCollectionProperty> string_collection_properties;
    std::vector<UiDescriptionOptionalUnsignedIntegerProperty> optional_unsigned_integer_properties;
    std::vector<UiDescriptionSizeConstraintsProperty> size_constraints_properties;
    std::vector<UiDescriptionTextStyleProperty> text_style_properties;
    std::vector<UiDescriptionResourceReferenceProperty> resource_reference_properties;

    friend bool operator==(const BuiltInUiDescriptionNode&, const BuiltInUiDescriptionNode&) = default;
};

struct UiDescriptionReferenceEdge final {
    ComponentDescriptionNodeId source_node_id{};
    ComponentDescriptionNodeId target_node_id{};
    BuiltInComponentReferenceKind kind{BuiltInComponentReferenceKind::button_command};

    friend bool operator==(const UiDescriptionReferenceEdge&, const UiDescriptionReferenceEdge&) = default;
};

/**
 * Format-neutral document value. Successful JSON parsing validates this value, but public
 * aggregate mutation does not preserve that guarantee automatically. Encoding/version markers
 * belong to adapters, not this semantic model. No Components or backend state are retained.
 * Reconstruction treats document-local resources as input and materializes references into
 * concrete property values; resource authoring identity remains in this document rather than in
 * runtime Components.
 */
struct BuiltInUiDescriptionDocument final {
    ComponentDescriptionNodeId root_node_id{};
    // v2 intentionally persists one closed document-local scope. Reusing the existing concrete
    // TextStyleResource keeps runtime dictionaries and durable descriptions on the same value
    // semantics without introducing a generic ResourceValue framework before a second family exists.
    std::vector<TextStyleResource> text_style_resources;
    std::vector<BuiltInUiDescriptionNode> nodes;
    std::vector<UiDescriptionReferenceEdge> reference_edges;

    friend bool operator==(const BuiltInUiDescriptionDocument&,
                           const BuiltInUiDescriptionDocument&) = default;
};

} // namespace sasd::ui
