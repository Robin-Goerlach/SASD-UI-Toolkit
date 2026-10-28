#include "ui_description_document_validation.hpp"

#include <sasd/ui/component_metadata.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_property_validation_metadata.hpp>
#include <sasd/ui/text/utf8.hpp>

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

namespace sasd::ui::detail {
namespace {

using Error = UiDescriptionJsonParseErrorCode;
using Kind = ComponentPropertyValueKind;
using Reference = BuiltInComponentReferenceKind;

struct Relation final {
    Reference kind;
    std::string_view source;
    std::string_view target;
};

constexpr std::array relations{
    Relation{Reference::button_command, "sasd.ui.Button", "sasd.ui.Command"},
    Relation{Reference::radio_button_group, "sasd.ui.RadioButton", "sasd.ui.RadioGroup"},
    Relation{Reference::list_view_model, "sasd.ui.ListView", "sasd.ui.ListModel"},
    Relation{Reference::table_view_model, "sasd.ui.TableView", "sasd.ui.TableModel"},
    Relation{Reference::tree_view_model, "sasd.ui.TreeView", "sasd.ui.TreeModel"},
    Relation{Reference::list_view_selection_model, "sasd.ui.ListView", "sasd.ui.ListSelectionModel"},
    Relation{Reference::table_view_selection_model, "sasd.ui.TableView", "sasd.ui.TableSelectionModel"},
    Relation{Reference::tree_view_selection_model, "sasd.ui.TreeView", "sasd.ui.TreeSelectionModel"},
    Relation{Reference::list_selection_model_model, "sasd.ui.ListSelectionModel", "sasd.ui.ListModel"},
    Relation{Reference::table_selection_model_model, "sasd.ui.TableSelectionModel", "sasd.ui.TableModel"},
    Relation{Reference::tree_selection_model_model, "sasd.ui.TreeSelectionModel", "sasd.ui.TreeModel"},
};

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

        std::vector<std::array<bool, relations.size()>> bound(count);
        std::vector<bool> selected_group(count, false);
        for (std::size_t input_index = 0; input_index < document_.reference_edges.size(); ++input_index) {
            const auto& edge = document_.reference_edges[input_index];
            if (edge.source_node_id >= count || edge.target_node_id >= count ||
                edge.source_node_id == edge.target_node_id) {
                return failReference(Error::invalid_reference, input_index, edge.source_node_id);
            }
            const auto rule = std::find_if(relations.begin(), relations.end(), [&](const auto& relation) {
                return relation.kind == edge.kind;
            });
            if (rule == relations.end()) {
                return failReference(Error::invalid_reference, input_index, edge.source_node_id);
            }
            const auto source = static_cast<std::size_t>(edge.source_node_id);
            const auto target = static_cast<std::size_t>(edge.target_node_id);
            const auto relation_index = static_cast<std::size_t>(rule - relations.begin());
            if (bound[source][relation_index] ||
                !derivesFrom(nodes[source].type_id, rule->source) ||
                !derivesFrom(nodes[target].type_id, rule->target)) {
                return failReference(Error::invalid_reference, input_index, edge.source_node_id);
            }
            bound[source][relation_index] = true;

            if (edge.kind == Reference::radio_button_group) {
                const auto& values = nodes[source].boolean_properties;
                const auto selected = std::find_if(values.begin(), values.end(), [](const auto& property) {
                    return property.declaring_type_id == "sasd.ui.RadioButton" &&
                           property.property_id == "selected";
                });
                if (selected == values.end()) {
                    return failReference(Error::invalid_reference, input_index, edge.source_node_id);
                }
                if (selected->value) {
                    if (selected_group[target]) {
                        return failReference(Error::invalid_reference, input_index, edge.source_node_id);
                    }
                    selected_group[target] = true;
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
        for (const auto& property : node.text_style_properties) {
            if (!validColor(property.value.foreground)) {
                return failNodeId(Error::invalid_property_value, node.node_id);
            }
        }

        if (node.type_id == "sasd.ui.ComboBox") {
            const auto& index = node.optional_unsigned_integer_properties.front().value;
            if (index && *index >= node.string_collection_properties.front().value.size()) {
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
