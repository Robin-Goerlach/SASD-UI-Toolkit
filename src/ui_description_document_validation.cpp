#include "ui_description_document_validation.hpp"

#include <sasd/ui/component_metadata.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_property_validation_metadata.hpp>
#include <sasd/ui/component_reference_metadata.hpp>
#include <sasd/ui/text/utf8.hpp>

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

namespace sasd::ui::detail {
namespace {

using Error = UiDescriptionJsonParseErrorCode;
using Kind = ComponentPropertyValueKind;

template<class Property>
bool hasValidStaticIntegerRange(const Property&, const ComponentPropertyMetadata&) noexcept {
    return true;
}

bool hasValidStaticIntegerRange(const UiDescriptionSignedIntegerProperty& property,
                                const ComponentPropertyMetadata& descriptor) noexcept {
    const auto range = builtInSignedIntegerPropertyRange(descriptor);
    return range.has_value() && range->contains(property.value);
}

bool hasValidStaticIntegerRange(const UiDescriptionUnsignedIntegerProperty& property,
                                const ComponentPropertyMetadata& descriptor) noexcept {
    const auto range = builtInUnsignedIntegerPropertyRange(descriptor);
    return range.has_value() && range->contains(property.value);
}

bool hasCanonicalStringValue(const UiDescriptionStringProperty& property,
                             const ComponentPropertyMetadata& descriptor) {
    const auto policy = builtInStringPropertyNormalization(descriptor);
    if (!policy) {
        return false;
    }
    switch (*policy) {
    case StringPropertyNormalizationKind::identity:
        return true;
    case StringPropertyNormalizationKind::single_line_utf8:
        return utf8::sanitizeSingleLine(property.value) == property.value;
    }
    return false;
}

bool hasValidSizeConstraintsValue(const UiDescriptionSizeConstraintsProperty& property,
                                  const ComponentPropertyMetadata& descriptor) noexcept {
    const auto policy = builtInSizeConstraintsPropertyValidation(descriptor);
    if (!policy) {
        return false;
    }
    switch (*policy) {
    case SizeConstraintsPropertyValidationKind::minimum_not_greater_than_maximum:
        return property.value.hasValidRange();
    }
    return false;
}

template<class Properties>
const typename Properties::value_type* findProperty(
    const Properties& properties, const ComponentPropertyMetadata& descriptor) noexcept {
    const auto found = std::find_if(properties.begin(), properties.end(), [&](const auto& property) {
        return property.declaring_type_id == descriptor.declaring_type_id &&
               property.property_id == descriptor.property_id;
    });
    return found == properties.end() ? nullptr : &*found;
}

bool hasValidOptionalUnsignedIntegerCrossPropertyValue(
    const BuiltInUiDescriptionNode& node,
    const UiDescriptionOptionalUnsignedIntegerProperty& property,
    const ComponentPropertyMetadata& descriptor) noexcept {
    const auto validation =
        builtInOptionalUnsignedIntegerCrossPropertyValidation(descriptor);
    if (!validation) {
        // Every current optional-unsigned descriptor requires an explicit contextual decision.
        // Missing metadata is a catalog coverage defect, not permission to accept any value.
        return false;
    }

    switch (validation->kind) {
    case OptionalUnsignedIntegerCrossPropertyValidationKind::
        index_within_string_collection: {
        if (validation->dependency_property.value_kind != Kind::string_collection) {
            return false;
        }
        const auto* dependency =
            findProperty(node.string_collection_properties, validation->dependency_property);
        if (dependency == nullptr) {
            return false;
        }

        // Absence is the semantic "no selection" value and is valid for every collection size.
        // Compare uint64_t directly and safely with the host collection size: metadata describes an
        // existing element, not whether narrowing the persistent value to size_t would succeed.
        return !property.value || std::cmp_less(*property.value, dependency->value.size());
    }
    }
    return false;
}

bool derivesFrom(std::string_view type, std::string_view base) {
    for (std::size_t depth = 0; depth < builtInComponentTypes().size(); ++depth) {
        if (type == base) {
            return true;
        }
        const auto* metadata = findBuiltInComponentType(type);
        if (metadata == nullptr || metadata->base_type_id.empty()) {
            return false;
        }
        type = metadata->base_type_id;
    }
    return false;
}

bool validUtf8(std::string_view text) {
    for (std::size_t offset = 0; offset < text.size();) {
        const auto scalar = utf8::decodeOne(text, offset);
        if (!scalar.valid || scalar.consumed == 0) {
            return false;
        }
        offset += scalar.consumed;
    }
    return true;
}

bool validColor(Color color) {
    switch (color) {
    case Color::default_color:
    case Color::black:
    case Color::red:
    case Color::green:
    case Color::yellow:
    case Color::blue:
    case Color::magenta:
    case Color::cyan:
    case Color::white:
    case Color::bright_black:
    case Color::bright_red:
    case Color::bright_green:
    case Color::bright_yellow:
    case Color::bright_blue:
    case Color::bright_magenta:
    case Color::bright_cyan:
    case Color::bright_white:
        return true;
    }
    return false;
}

class Validator final {
public:
    explicit Validator(BuiltInUiDescriptionDocument& document) : document_{document} {}

    UiDescriptionValidationResult validate() {
        auto& nodes = document_.nodes;
        const auto count = nodes.size();
        if (count == 0 || document_.root_node_id != 0) {
            return fail(Error::invalid_ownership);
        }

        input_index_by_id_.resize(count);
        std::vector<bool> ids(count, false);
        for (std::size_t input_index = 0; input_index < count; ++input_index) {
            const auto id = nodes[input_index].node_id;
            if (id >= count || ids[static_cast<std::size_t>(id)]) {
                return failNode(Error::invalid_ownership, input_index, id);
            }
            ids[static_cast<std::size_t>(id)] = true;
            input_index_by_id_[static_cast<std::size_t>(id)] = input_index;
        }

        std::sort(nodes.begin(), nodes.end(), [](const auto& left, const auto& right) {
            return left.node_id < right.node_id;
        });

        std::vector<bool> owned(count, false);
        for (auto& node : nodes) {
            if (const auto result = validateNode(node); !result) {
                return result;
            }
            for (const auto child : node.owned_component_ids) {
                if (child == 0 || child >= count || child == node.node_id ||
                    owned[static_cast<std::size_t>(child)]) {
                    return failNodeId(Error::invalid_ownership, node.node_id);
                }
                owned[static_cast<std::size_t>(child)] = true;
            }
        }

        // Dense pre-order is the v1 identity contract. Iteration avoids making document depth a
        // call-stack limit and simultaneously proves reachability and cycle freedom.
        std::vector<ComponentDescriptionNodeId> pending{0};
        std::size_t next = 0;
        while (!pending.empty()) {
            const auto id = pending.back();
            pending.pop_back();
            if (id != next) {
                return failNodeId(Error::invalid_ownership, id);
            }
            ++next;
            const auto& children = nodes[static_cast<std::size_t>(id)].owned_component_ids;
            pending.insert(pending.end(), children.rbegin(), children.rend());
        }
        if (next != count) {
            return fail(Error::invalid_ownership);
        }

        const auto reference_metadata = builtInComponentReferenceMetadata();
        std::vector<std::vector<bool>> bound(
            count, std::vector<bool>(reference_metadata.size(), false));

        // Phase A validates the fundamental optional-singular relation contract. Metadata supplies
        // format-neutral endpoint types; JSON symbols and typed runtime binding operations remain
        // concerns of their respective encoding and reconstruction layers.
        for (std::size_t input_index = 0; input_index < document_.reference_edges.size(); ++input_index) {
            const auto& edge = document_.reference_edges[input_index];
            if (edge.source_node_id >= count || edge.target_node_id >= count ||
                edge.source_node_id == edge.target_node_id) {
                return failReference(Error::invalid_reference, input_index, edge.source_node_id);
            }
            const auto* metadata = findBuiltInComponentReferenceMetadata(edge.kind);
            if (metadata == nullptr) {
                return failReference(Error::invalid_reference, input_index, edge.source_node_id);
            }
            const auto source = static_cast<std::size_t>(edge.source_node_id);
            const auto target = static_cast<std::size_t>(edge.target_node_id);
            const auto relation_index =
                static_cast<std::size_t>(metadata - reference_metadata.data());
            if (bound[source][relation_index] ||
                !derivesFrom(nodes[source].type_id, metadata->source_type_id) ||
                !derivesFrom(nodes[target].type_id, metadata->target_type_id)) {
                return failReference(Error::invalid_reference, input_index, edge.source_node_id);
            }
            bound[source][relation_index] = true;
        }

        // Phase B applies zero-or-more additional graph rules only after every edge is structurally
        // and type valid. RadioGroup exclusivity is graph context: standalone RadioButtons never
        // participate, and distinct target groups have independent true-value counts.
        for (const auto& metadata : reference_metadata) {
            for (const auto& validation :
                 builtInComponentReferenceGraphValidations(metadata.kind)) {
                const auto& source_property = validation.source_property;
                const auto* catalog_property = findBuiltInComponentProperty(
                    source_property.declaring_type_id, source_property.property_id);
                if (validation.reference_kind != metadata.kind ||
                    source_property.value_kind != Kind::boolean || catalog_property == nullptr ||
                    catalog_property->declaring_type_id != source_property.declaring_type_id ||
                    catalog_property->property_id != source_property.property_id ||
                    catalog_property->value_kind != source_property.value_kind ||
                    !derivesFrom(metadata.source_type_id,
                                 source_property.declaring_type_id)) {
                    return fail(Error::invalid_reference);
                }

                switch (validation.kind) {
                case BuiltInComponentReferenceGraphValidationKind::
                    at_most_one_source_boolean_true_per_target: {
                    std::vector<bool> target_has_true_source(count, false);
                    for (std::size_t input_index = 0;
                         input_index < document_.reference_edges.size(); ++input_index) {
                        const auto& edge = document_.reference_edges[input_index];
                        if (edge.kind != validation.reference_kind) {
                            continue;
                        }
                        const auto source = static_cast<std::size_t>(edge.source_node_id);
                        const auto target = static_cast<std::size_t>(edge.target_node_id);
                        const auto* value = findProperty(
                            nodes[source].boolean_properties, source_property);
                        if (value == nullptr) {
                            return failReference(
                                Error::invalid_reference, input_index, edge.source_node_id);
                        }
                        if (value->value && target_has_true_source[target]) {
                            return failReference(
                                Error::invalid_reference, input_index, edge.source_node_id);
                        }
                        if (value->value) {
                            target_has_true_source[target] = true;
                        }
                    }
                    break;
                }
                }
            }
        }

        std::sort(document_.reference_edges.begin(), document_.reference_edges.end(),
                  [](const auto& left, const auto& right) {
            if (left.source_node_id != right.source_node_id) {
                return left.source_node_id < right.source_node_id;
            }
            return left.kind < right.kind;
        });
        return {};
    }

private:
    BuiltInUiDescriptionDocument& document_;
    std::vector<std::size_t> input_index_by_id_;

    static UiDescriptionValidationResult fail(Error error) {
        return {error, UiDescriptionValidationLocation::document, 0, 0};
    }

    static UiDescriptionValidationResult failNode(
        Error error, std::size_t input_index, ComponentDescriptionNodeId node_id) {
        return {error, UiDescriptionValidationLocation::node, input_index, node_id};
    }

    UiDescriptionValidationResult failNodeId(Error error, ComponentDescriptionNodeId node_id) const {
        return failNode(error, input_index_by_id_[static_cast<std::size_t>(node_id)], node_id);
    }

    static UiDescriptionValidationResult failReference(
        Error error, std::size_t input_index, ComponentDescriptionNodeId source_node_id) {
        return {error, UiDescriptionValidationLocation::reference, input_index, source_node_id};
    }

    template<class Properties>
    UiDescriptionValidationResult validateProperties(
        const BuiltInUiDescriptionNode& node,
        Properties& values,
        Kind kind,
        std::vector<const ComponentPropertyMetadata*>& seen) {
        for (const auto& value : values) {
            if (!validUtf8(value.declaring_type_id) || !validUtf8(value.property_id)) {
                return failNodeId(Error::invalid_utf8, node.node_id);
            }
            const auto* descriptor = findBuiltInComponentProperty(node.type_id, value.property_id);
            if (descriptor == nullptr || descriptor->declaring_type_id != value.declaring_type_id) {
                return failNodeId(Error::unknown_property, node.node_id);
            }
            if (descriptor->value_kind != kind) {
                return failNodeId(Error::property_kind_mismatch, node.node_id);
            }
            if (!hasValidStaticIntegerRange(value, *descriptor)) {
                return failNodeId(Error::invalid_property_value, node.node_id);
            }
            if (std::find(seen.begin(), seen.end(), descriptor) != seen.end()) {
                return failNodeId(Error::unknown_property, node.node_id);
            }
            seen.push_back(descriptor);
        }
        std::sort(values.begin(), values.end(), [](const auto& left, const auto& right) {
            if (left.declaring_type_id != right.declaring_type_id) {
                return left.declaring_type_id < right.declaring_type_id;
            }
            return left.property_id < right.property_id;
        });
        return {};
    }

    UiDescriptionValidationResult validateNode(BuiltInUiDescriptionNode& node) {
        if (!validUtf8(node.type_id)) {
            return failNodeId(Error::invalid_utf8, node.node_id);
        }
        const auto* type = findBuiltInComponentType(node.type_id);
        if (type == nullptr) {
            return failNodeId(Error::unknown_component_type, node.node_id);
        }
        if (!node.owned_component_ids.empty() && type->kind != ComponentTypeKind::container) {
            return failNodeId(Error::invalid_ownership, node.node_id);
        }

        std::vector<const ComponentPropertyMetadata*> seen;
        if (auto result = validateProperties(node, node.boolean_properties, Kind::boolean, seen); !result) return result;
        if (auto result = validateProperties(node, node.string_properties, Kind::string, seen); !result) return result;
        if (auto result = validateProperties(node, node.signed_integer_properties, Kind::signed_integer, seen); !result) return result;
        if (auto result = validateProperties(node, node.unsigned_integer_properties, Kind::unsigned_integer, seen); !result) return result;
        if (auto result = validateProperties(node, node.string_collection_properties, Kind::string_collection, seen); !result) return result;
        if (auto result = validateProperties(node, node.optional_unsigned_integer_properties, Kind::optional_unsigned_integer, seen); !result) return result;
        if (auto result = validateProperties(node, node.size_constraints_properties, Kind::size_constraints, seen); !result) return result;
        if (auto result = validateProperties(node, node.text_style_properties, Kind::text_style, seen); !result) return result;

        std::size_t expected = 0;
        for (const auto* current = type; current != nullptr;
             current = findBuiltInComponentType(current->base_type_id)) {
            expected += declaredBuiltInComponentProperties(current->type_id).size();
        }
        if (seen.size() != expected) {
            return failNodeId(Error::missing_member, node.node_id);
        }

        for (const auto& property : node.string_properties) {
            if (!validUtf8(property.value)) {
                return failNodeId(Error::invalid_utf8, node.node_id);
            }
            const auto* descriptor =
                findBuiltInComponentProperty(node.type_id, property.property_id);
            if (descriptor == nullptr || !hasCanonicalStringValue(property, *descriptor)) {
                return failNodeId(Error::invalid_property_value, node.node_id);
            }
        }
        for (const auto& property : node.string_collection_properties) {
            if (std::any_of(property.value.begin(), property.value.end(), [](const auto& value) {
                    return !validUtf8(value);
                })) {
                return failNodeId(Error::invalid_utf8, node.node_id);
            }
        }
        for (const auto& property : node.size_constraints_properties) {
            const auto* descriptor =
                findBuiltInComponentProperty(node.type_id, property.property_id);
            if (descriptor == nullptr || !hasValidSizeConstraintsValue(property, *descriptor)) {
                return failNodeId(Error::invalid_property_value, node.node_id);
            }
        }
        for (const auto& property : node.optional_unsigned_integer_properties) {
            const auto* descriptor =
                findBuiltInComponentProperty(node.type_id, property.property_id);
            if (descriptor == nullptr ||
                !hasValidOptionalUnsignedIntegerCrossPropertyValue(
                    node, property, *descriptor)) {
                return failNodeId(Error::invalid_property_value, node.node_id);
            }
        }
        for (const auto& property : node.text_style_properties) {
            if (!validColor(property.value.foreground)) {
                return failNodeId(Error::invalid_property_value, node.node_id);
            }
        }

        return {};
    }
};

} // namespace

UiDescriptionValidationResult
validateAndNormalizeBuiltInUiDescriptionDocument(BuiltInUiDescriptionDocument& document) {
    return Validator{document}.validate();
}

} // namespace sasd::ui::detail
