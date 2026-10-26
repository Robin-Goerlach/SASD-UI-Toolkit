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
    if (property.declaring_type_id == "sasd.ui.RadioButton" && property.property_id == "selected") {
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
        (property.declaring_type_id == "sasd.ui.CheckBox" && property.property_id == "text") ||
        (property.declaring_type_id == "sasd.ui.RadioButton" && property.property_id == "text")) {
        // Empty built-in defaults use immutable literal storage and require no allocation.
        return std::string_view{};
    }
    return std::nullopt;
}

std::optional<std::int64_t> builtInSignedIntegerPropertyDefault(
    std::string_view target_type_id,
    const ComponentPropertyMetadata& property) noexcept {
    if (findBuiltInComponentType(target_type_id) == nullptr ||
        property.value_kind != ComponentPropertyValueKind::signed_integer ||
        !isExactBuiltInProperty(target_type_id, property)) {
        return std::nullopt;
    }
    // Mirror the public construction defaults, without constructing a layout or evaluating its
    // children. Descriptor identity keeps equal local names on sibling layouts independent.
    if (property.declaring_type_id == "sasd.ui.HBox" && property.property_id == "spacing") {
        return 0;
    }
    if (property.declaring_type_id == "sasd.ui.VBox" && property.property_id == "spacing") {
        return 0;
    }
    if (property.declaring_type_id == "sasd.ui.GridLayout" && property.property_id == "columnSpacing") {
        return 0;
    }
    if (property.declaring_type_id == "sasd.ui.GridLayout" && property.property_id == "rowSpacing") {
        return 0;
    }
    if (property.declaring_type_id == "sasd.ui.FormLayout" && property.property_id == "columnSpacing") {
        return 1;
    }
    if (property.declaring_type_id == "sasd.ui.FormLayout" && property.property_id == "rowSpacing") {
        return 0;
    }
    return std::nullopt;
}

std::optional<std::uint64_t> builtInUnsignedIntegerPropertyDefault(
    std::string_view target_type_id,
    const ComponentPropertyMetadata& property) noexcept {
    if (findBuiltInComponentType(target_type_id) == nullptr ||
        property.value_kind != ComponentPropertyValueKind::unsigned_integer ||
        !isExactBuiltInProperty(target_type_id, property)) {
        return std::nullopt;
    }
    if (property.declaring_type_id == "sasd.ui.GridLayout" && property.property_id == "columnCount") {
        return 1;
    }
    return std::nullopt;
}

std::optional<std::span<const std::string_view>> builtInStringCollectionPropertyDefault(
    std::string_view target_type_id, const ComponentPropertyMetadata& property) noexcept {
    if (findBuiltInComponentType(target_type_id) == nullptr ||
        property.value_kind != ComponentPropertyValueKind::string_collection ||
        !isExactBuiltInProperty(target_type_id, property)) {
        return std::nullopt;
    }
    if (property.declaring_type_id == "sasd.ui.ComboBox" && property.property_id == "items") {
        return std::span<const std::string_view>{};
    }
    return std::nullopt;
}

std::optional<std::optional<std::uint64_t>> builtInOptionalUnsignedIntegerPropertyDefault(
    std::string_view target_type_id, const ComponentPropertyMetadata& property) noexcept {
    if (findBuiltInComponentType(target_type_id) == nullptr ||
        property.value_kind != ComponentPropertyValueKind::optional_unsigned_integer ||
        !isExactBuiltInProperty(target_type_id, property)) {
        return std::nullopt;
    }
    if (property.declaring_type_id == "sasd.ui.ComboBox" && property.property_id == "selectedIndex") {
        return std::optional<std::optional<std::uint64_t>>{std::in_place, std::nullopt};
    }
    return std::nullopt;
}

std::optional<SizeConstraints> builtInSizeConstraintsPropertyDefault(
    std::string_view target_type_id, const ComponentPropertyMetadata& property) noexcept {
    if (findBuiltInComponentType(target_type_id) == nullptr ||
        property.value_kind != ComponentPropertyValueKind::size_constraints ||
        property.declaring_type_id != "sasd.ui.Widget" || property.property_id != "sizeConstraints" ||
        !isExactBuiltInProperty(target_type_id, property)) {
        return std::nullopt;
    }
    // Inherited lookup establishes Widget compatibility. No built-in constructor overrides these
    // hints, so reuse the public value's default instead of constructing widgets or duplicating it.
    return SizeConstraints{};
}

std::optional<TextStyle> builtInTextStylePropertyDefault(
    std::string_view target_type_id, const ComponentPropertyMetadata& property) noexcept {
    if (property.value_kind != ComponentPropertyValueKind::text_style ||
        !isExactBuiltInProperty(target_type_id, property)) {
        return std::nullopt;
    }
    // All six direct owning controls initialize the public value identically. Schema identity
    // excludes siblings and unrelated targets; there is no style inheritance or constructor probe.
    return TextStyle{};
}

} // namespace sasd::ui
