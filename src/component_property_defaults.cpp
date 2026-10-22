#include <sasd/ui/component_property_defaults.hpp>

#include <sasd/ui/component_metadata.hpp>

namespace sasd::ui {
namespace {

bool isExactBuiltInProperty(std::string_view target_type_id,
                            const ComponentPropertyMetadata& property) noexcept {
    // Compare the complete identity: property_id alone would accept Label::text for Button.
    const auto* resolved = findBuiltInComponentProperty(target_type_id, property.property_id);
    return resolved != nullptr && resolved->declaring_type_id == property.declaring_type_id &&
           resolved->property_id == property.property_id &&
           resolved->value_kind == property.value_kind;
}

bool hasFocusableOverride(std::string_view target_type_id) noexcept {
    // These explicit overrides mirror the constructors of the interactive built-ins. Keeping the
    // small policy here avoids deriving metadata by constructing components or introducing a rule
    // engine for a finite set of built-in construction defaults.
    return target_type_id == "sasd.ui.Button" || target_type_id == "sasd.ui.TextField" ||
           target_type_id == "sasd.ui.CheckBox" || target_type_id == "sasd.ui.RadioButton" ||
           target_type_id == "sasd.ui.ComboBox" || target_type_id == "sasd.ui.ListView" ||
           target_type_id == "sasd.ui.TableView" || target_type_id == "sasd.ui.TreeView";
}

} // namespace

std::optional<bool> builtInBooleanPropertyDefault(
    std::string_view target_type_id,
    const ComponentPropertyMetadata& property) noexcept {
    if (findBuiltInComponentType(target_type_id) == nullptr ||
        property.value_kind != ComponentPropertyValueKind::boolean ||
        !isExactBuiltInProperty(target_type_id, property)) {
        return std::nullopt;
    }

    if (property.declaring_type_id == "sasd.ui.Widget") {
        if (property.property_id == "visible" || property.property_id == "enabled") {
            return true;
        }
        if (property.property_id == "focusable") {
            return hasFocusableOverride(target_type_id);
        }
    }
    if (property.declaring_type_id == "sasd.ui.Command" && property.property_id == "enabled") {
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.CheckBox" && property.property_id == "checked") {
        return false;
    }
    return std::nullopt;
}

std::optional<std::string_view> builtInStringPropertyDefault(
    std::string_view target_type_id,
    const ComponentPropertyMetadata& property) noexcept {
    if (findBuiltInComponentType(target_type_id) == nullptr ||
        property.value_kind != ComponentPropertyValueKind::string ||
        !isExactBuiltInProperty(target_type_id, property)) {
        return std::nullopt;
    }

    if ((property.declaring_type_id == "sasd.ui.Command" && property.property_id == "text") ||
        (property.declaring_type_id == "sasd.ui.Label" && property.property_id == "text") ||
        (property.declaring_type_id == "sasd.ui.Button" && property.property_id == "text") ||
        (property.declaring_type_id == "sasd.ui.TextField" && property.property_id == "text") ||
        (property.declaring_type_id == "sasd.ui.CheckBox" && property.property_id == "text")) {
        // Empty built-in defaults use immutable literal storage and require no allocation.
        return std::string_view{};
    }
    return std::nullopt;
}

} // namespace sasd::ui
