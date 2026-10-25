#include <sasd/ui/component_property_metadata.hpp>

#include <array>
#include <cstddef>

namespace sasd::ui {
namespace {

// Keep descriptors grouped by declaring type so a direct query can return a subspan without
// allocating or constructing a temporary list. Inherited properties deliberately appear only in
// the catalog of their declaring type; lookup walks ComponentTypeMetadata::base_type_id instead.
constexpr std::array<ComponentPropertyMetadata, 21> built_in_properties{{
    {"sasd.ui.Widget", "visible", ComponentPropertyValueKind::boolean},
    {"sasd.ui.Widget", "enabled", ComponentPropertyValueKind::boolean},
    {"sasd.ui.Widget", "focusable", ComponentPropertyValueKind::boolean},
    {"sasd.ui.Command", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.Command", "enabled", ComponentPropertyValueKind::boolean},
    {"sasd.ui.Label", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.Button", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.TextField", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.CheckBox", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.CheckBox", "checked", ComponentPropertyValueKind::boolean},
    {"sasd.ui.RadioButton", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.RadioButton", "selected", ComponentPropertyValueKind::boolean},
    {"sasd.ui.ComboBox", "items", ComponentPropertyValueKind::string_collection},
    {"sasd.ui.ComboBox", "selectedIndex", ComponentPropertyValueKind::optional_unsigned_integer},
    {"sasd.ui.HBox", "spacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.VBox", "spacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.GridLayout", "columnCount", ComponentPropertyValueKind::unsigned_integer},
    {"sasd.ui.GridLayout", "columnSpacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.GridLayout", "rowSpacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.FormLayout", "columnSpacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.FormLayout", "rowSpacing", ComponentPropertyValueKind::signed_integer},
}};

constexpr std::span<const ComponentPropertyMetadata> empty_properties() noexcept {
    return {};
}

} // namespace

std::span<const ComponentPropertyMetadata> declaredBuiltInComponentProperties(
    std::string_view type_id) noexcept {
    for (std::size_t index = 0; index < built_in_properties.size();) {
        const auto declaring_type_id = built_in_properties[index].declaring_type_id;
        std::size_t end = index + 1;
        while (end < built_in_properties.size() &&
               built_in_properties[end].declaring_type_id == declaring_type_id) {
            ++end;
        }

        if (declaring_type_id == type_id) {
            return std::span{built_in_properties}.subspan(index, end - index);
        }
        index = end;
    }

    return empty_properties();
}

const ComponentPropertyMetadata* findBuiltInComponentProperty(
    std::string_view type_id,
    std::string_view property_id) noexcept {
    // The built-in type catalog is finite and acyclic. The bound also keeps this query fail-closed
    // if a future catalog edit accidentally introduces a cycle in its base-type metadata.
    for (std::size_t depth = 0; depth < builtInComponentTypes().size(); ++depth) {
        for (const auto& property : declaredBuiltInComponentProperties(type_id)) {
            if (property.property_id == property_id) {
                return &property;
            }
        }

        const auto* type = findBuiltInComponentType(type_id);
        if (type == nullptr || type->base_type_id.empty()) {
            return nullptr;
        }
        type_id = type->base_type_id;
    }

    return nullptr;
}

} // namespace sasd::ui
