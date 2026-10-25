#include <sasd/ui/component_property_snapshot.hpp>

#include <sasd/ui/component_metadata.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_runtime_metadata.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace sasd::ui {
namespace {

/**
 * Collects the exact metadata chain from the target toward Component, then returns it in
 * declaration order. The duplicate check makes malformed future catalog edits fail closed instead
 * of looping or producing an ambiguous property order.
 */
[[nodiscard]] std::optional<std::vector<const ComponentTypeMetadata*>>
collectTypeChain(std::string_view target_type_id, std::size_t& chain_size) {
    std::vector<const ComponentTypeMetadata*> reverse_chain;
    chain_size = 0;
    std::string_view current_type_id = target_type_id;

    while (true) {
        const auto* current = findBuiltInComponentType(current_type_id);
        if (current == nullptr) {
            return std::nullopt;
        }
        for (std::size_t index = 0; index < chain_size; ++index) {
            if (reverse_chain[index]->type_id == current->type_id) {
                return std::nullopt;
            }
        }

        reverse_chain.push_back(current);
        ++chain_size;
        if (current->base_type_id.empty()) {
            break;
        }
        current_type_id = current->base_type_id;
    }

    return reverse_chain;
}

} // namespace

std::optional<BuiltInComponentPropertySnapshot>
captureBuiltInComponentPropertySnapshot(const Component& component) {
    const auto* type = builtInComponentTypeOf(component);
    if (type == nullptr) {
        return std::nullopt;
    }

    std::size_t chain_size = 0;
    const auto reverse_chain = collectTypeChain(type->type_id, chain_size);
    if (!reverse_chain.has_value()) {
        return std::nullopt;
    }

    BuiltInComponentPropertySnapshot snapshot;
    snapshot.type_id = std::string{type->type_id};

    for (std::size_t reverse_index = chain_size; reverse_index > 0; --reverse_index) {
        const auto* declaring_type = (*reverse_chain)[reverse_index - 1];
        for (const auto& property : declaredBuiltInComponentProperties(declaring_type->type_id)) {
            if (property.declaring_type_id != declaring_type->type_id) {
                return std::nullopt;
            }

            switch (property.value_kind) {
            case ComponentPropertyValueKind::boolean: {
                const auto value = readBuiltInBooleanProperty(component, property);
                const auto effective_default =
                    builtInBooleanPropertyDefault(type->type_id, property);
                if (!value.has_value() || !effective_default.has_value()) {
                    return std::nullopt;
                }
                snapshot.boolean_properties.push_back({
                    std::string{property.declaring_type_id},
                    std::string{property.property_id},
                    *value,
                    *value == *effective_default,
                });
                break;
            }
            case ComponentPropertyValueKind::string: {
                const auto value = readBuiltInStringProperty(component, property);
                const auto effective_default =
                    builtInStringPropertyDefault(type->type_id, property);
                if (!value.has_value() || !effective_default.has_value()) {
                    return std::nullopt;
                }
                snapshot.string_properties.push_back({
                    std::string{property.declaring_type_id},
                    std::string{property.property_id},
                    *value,
                    *value == *effective_default,
                });
                break;
            }
            case ComponentPropertyValueKind::signed_integer: {
                const auto value = readBuiltInSignedIntegerProperty(component, property);
                const auto effective_default =
                    builtInSignedIntegerPropertyDefault(type->type_id, property);
                if (!value.has_value() || !effective_default.has_value()) {
                    return std::nullopt;
                }
                snapshot.signed_integer_properties.push_back({
                    std::string{property.declaring_type_id},
                    std::string{property.property_id},
                    *value,
                    *value == *effective_default,
                });
                break;
            }
            case ComponentPropertyValueKind::unsigned_integer: {
                const auto value = readBuiltInUnsignedIntegerProperty(component, property);
                const auto effective_default =
                    builtInUnsignedIntegerPropertyDefault(type->type_id, property);
                if (!value.has_value() || !effective_default.has_value()) {
                    return std::nullopt;
                }
                snapshot.unsigned_integer_properties.push_back({
                    std::string{property.declaring_type_id},
                    std::string{property.property_id},
                    *value,
                    *value == *effective_default,
                });
                break;
            }
            case ComponentPropertyValueKind::string_collection: {
                auto value = readBuiltInStringCollectionProperty(component, property);
                const auto effective_default =
                    builtInStringCollectionPropertyDefault(type->type_id, property);
                if (!value.has_value() || !effective_default.has_value()) {
                    return std::nullopt;
                }
                const bool is_default = std::equal(
                    value->begin(), value->end(), effective_default->begin(), effective_default->end());
                snapshot.string_collection_properties.push_back({
                    std::string{property.declaring_type_id}, std::string{property.property_id},
                    std::move(*value), is_default,
                });
                break;
            }
            case ComponentPropertyValueKind::optional_unsigned_integer: {
                auto value = readBuiltInOptionalUnsignedIntegerProperty(component, property);
                const auto effective_default =
                    builtInOptionalUnsignedIntegerPropertyDefault(type->type_id, property);
                if (!value.has_value() || !effective_default.has_value()) {
                    return std::nullopt;
                }
                const bool is_default = *value == *effective_default;
                snapshot.optional_unsigned_integer_properties.push_back({
                    std::string{property.declaring_type_id}, std::string{property.property_id},
                    std::move(*value), is_default,
                });
                break;
            }
            default:
                return std::nullopt;
            }
        }
    }

    return snapshot;
}

} // namespace sasd::ui
