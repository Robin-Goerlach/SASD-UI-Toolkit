#include <sasd/ui/component_property_authoring_metadata.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_property_resource_metadata.hpp>
#include <sasd/ui/component_property_validation_metadata.hpp>
#include <sasd/ui/component_type_authoring_metadata.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace sasd::ui;

std::vector<ComponentPropertyMetadata> effectiveProperties(const ComponentTypeMetadata& type) {
    std::vector<const ComponentTypeMetadata*> reverse_chain;
    const ComponentTypeMetadata* current = &type;
    for (std::size_t depth = 0; current != nullptr && depth < builtInComponentTypes().size();
         ++depth) {
        reverse_chain.push_back(current);
        current = current->base_type_id.empty()
            ? nullptr
            : findBuiltInComponentType(current->base_type_id);
    }

    std::reverse(reverse_chain.begin(), reverse_chain.end());
    std::vector<ComponentPropertyMetadata> result;
    for (const auto* declaring_type : reverse_chain) {
        const auto declared = declaredBuiltInComponentProperties(declaring_type->type_id);
        result.insert(result.end(), declared.begin(), declared.end());
    }
    return result;
}

const char* valueKindName(ComponentPropertyValueKind kind) noexcept {
    switch (kind) {
    case ComponentPropertyValueKind::boolean: return "boolean";
    case ComponentPropertyValueKind::string: return "string";
    case ComponentPropertyValueKind::signed_integer: return "signed-integer";
    case ComponentPropertyValueKind::unsigned_integer: return "unsigned-integer";
    case ComponentPropertyValueKind::string_collection: return "string-collection";
    case ComponentPropertyValueKind::optional_unsigned_integer: return "optional-unsigned-integer";
    case ComponentPropertyValueKind::size_constraints: return "size-constraints";
    case ComponentPropertyValueKind::text_style: return "text-style";
    }
    return "unknown";
}

const char* categoryName(ComponentPropertyAuthoringCategory category) noexcept {
    switch (category) {
    case ComponentPropertyAuthoringCategory::appearance: return "appearance";
    case ComponentPropertyAuthoringCategory::behavior: return "behavior";
    case ComponentPropertyAuthoringCategory::content: return "content";
    case ComponentPropertyAuthoringCategory::layout: return "layout";
    case ComponentPropertyAuthoringCategory::state: return "state";
    }
    return "unknown";
}

void printDefault(std::string_view target_type_id, const ComponentPropertyMetadata& property) {
    switch (property.value_kind) {
    case ComponentPropertyValueKind::boolean:
        if (const auto value = builtInBooleanPropertyDefault(target_type_id, property)) {
            std::cout << "      default: " << (*value ? "true" : "false") << '\n';
        }
        break;
    case ComponentPropertyValueKind::string:
        if (const auto value = builtInStringPropertyDefault(target_type_id, property)) {
            std::cout << "      default: \"" << *value << "\"\n";
        }
        break;
    case ComponentPropertyValueKind::signed_integer:
        if (const auto value = builtInSignedIntegerPropertyDefault(target_type_id, property)) {
            std::cout << "      default: " << *value << '\n';
        }
        break;
    case ComponentPropertyValueKind::unsigned_integer:
        if (const auto value = builtInUnsignedIntegerPropertyDefault(target_type_id, property)) {
            std::cout << "      default: " << *value << '\n';
        }
        break;
    case ComponentPropertyValueKind::text_style:
        if (builtInTextStylePropertyDefault(target_type_id, property)) {
            std::cout << "      default: TextStyle{}\n";
        }
        break;
    case ComponentPropertyValueKind::string_collection:
    case ComponentPropertyValueKind::optional_unsigned_integer:
    case ComponentPropertyValueKind::size_constraints:
        // The demo keeps output compact. The installed consumer exercises these public default
        // families too; no generic value carrier is needed merely to print heterogeneous values.
        break;
    }
}

void printProperty(std::string_view target_type_id, const ComponentPropertyMetadata& property) {
    std::cout << "    " << property.property_id
              << "\n      declaring type: " << property.declaring_type_id
              << "\n      kind: " << valueKindName(property.value_kind) << '\n';
    if (const auto authoring = builtInComponentPropertyAuthoringMetadata(property)) {
        std::cout << "      category: " << categoryName(authoring->category) << '\n';
    }
    printDefault(target_type_id, property);
    if (const auto resource = builtInComponentPropertyResourceMetadata(property)) {
        if (resource->resource_kind == ResourceKind::text_style) {
            std::cout << "      resource capable: text-style\n";
        }
    }
    if (builtInSizeConstraintsPropertyValidation(property)) {
        std::cout << "      validation: minimum <= maximum\n";
    }
    if (const auto range = builtInUnsignedIntegerPropertyRange(property)) {
        std::cout << "      range: " << range->minimum << " .. " << range->maximum << '\n';
    }
}

} // namespace

int main() {
    std::size_t constructible_count = 0;
    std::size_t abstract_count = 0;
    for (const auto& type : sasd::ui::builtInComponentTypes()) {
        const auto authoring = sasd::ui::builtInComponentTypeAuthoringMetadata(type);
        if (!authoring) {
            std::cerr << "Missing authoring metadata for " << type.type_id << '\n';
            return 1;
        }
        if (authoring->instantiation ==
            sasd::ui::ComponentTypeAuthoringInstantiationKind::constructible) {
            ++constructible_count;
        } else {
            ++abstract_count;
        }
    }

    std::cout << "Built-in types: " << sasd::ui::builtInComponentTypes().size()
              << "\n  constructible: " << constructible_count
              << "\n  abstract contracts: " << abstract_count << "\n\n";

    const auto* button = sasd::ui::findBuiltInComponentType("sasd.ui.Button");
    if (button == nullptr) {
        return 1;
    }
    const auto button_authoring = sasd::ui::builtInComponentTypeAuthoringMetadata(*button);
    std::cout << button->type_id << "\n  base: " << button->base_type_id
              << "\n  constructible: "
              << (button_authoring && button_authoring->instantiation ==
                      sasd::ui::ComponentTypeAuthoringInstantiationKind::constructible
                      ? "yes" : "no")
              << "\n\n  effective properties:\n";
    for (const auto& property : effectiveProperties(*button)) {
        printProperty(button->type_id, property);
    }

    // Also show the unsigned layout default/range family without constructing GridLayout.
    const auto* columns =
        sasd::ui::findBuiltInComponentProperty("sasd.ui.GridLayout", "columnCount");
    if (columns != nullptr) {
        std::cout << "\nsasd.ui.GridLayout metadata sample:\n";
        printProperty("sasd.ui.GridLayout", *columns);
    }
}
