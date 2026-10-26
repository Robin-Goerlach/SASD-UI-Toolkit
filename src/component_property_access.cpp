#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/widget.hpp>

#include <utility>

namespace sasd::ui {
namespace {

bool isBuiltInSchemaDescriptor(const ComponentPropertyMetadata& candidate) noexcept {
    // A copied descriptor is valid when its semantic fields match a direct catalog entry. Address
    // identity is intentionally not part of the public metadata contract.
    for (const auto& property : declaredBuiltInComponentProperties(candidate.declaring_type_id)) {
        if (property.property_id == candidate.property_id &&
            property.value_kind == candidate.value_kind) {
            return true;
        }
    }
    return false;
}

bool isWidgetProperty(const ComponentPropertyMetadata& property,
                     std::string_view property_id) noexcept {
    return property.declaring_type_id == "sasd.ui.Widget" &&
           property.property_id == property_id &&
           property.value_kind == ComponentPropertyValueKind::boolean;
}

bool isCommandProperty(const ComponentPropertyMetadata& property,
                       std::string_view property_id,
                       ComponentPropertyValueKind value_kind) noexcept {
    return property.declaring_type_id == "sasd.ui.Command" &&
           property.property_id == property_id && property.value_kind == value_kind;
}

} // namespace

std::optional<bool> readBuiltInBooleanProperty(
    const Component& component,
    const ComponentPropertyMetadata& property) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::boolean) {
        return std::nullopt;
    }

    if (isWidgetProperty(property, "visible")) {
        const auto* widget = dynamic_cast<const Widget*>(&component);
        return widget == nullptr ? std::nullopt : std::optional<bool>{widget->isVisible()};
    }
    if (isWidgetProperty(property, "enabled")) {
        const auto* widget = dynamic_cast<const Widget*>(&component);
        return widget == nullptr ? std::nullopt : std::optional<bool>{widget->isEnabled()};
    }
    if (isWidgetProperty(property, "focusable")) {
        const auto* widget = dynamic_cast<const Widget*>(&component);
        return widget == nullptr ? std::nullopt : std::optional<bool>{widget->isFocusable()};
    }
    if (isCommandProperty(property, "enabled", ComponentPropertyValueKind::boolean)) {
        const auto* command = dynamic_cast<const Command*>(&component);
        return command == nullptr ? std::nullopt : std::optional<bool>{command->isEnabled()};
    }
    if (property.declaring_type_id == "sasd.ui.CheckBox" && property.property_id == "checked") {
        const auto* check_box = dynamic_cast<const CheckBox*>(&component);
        return check_box == nullptr ? std::nullopt : std::optional<bool>{check_box->isChecked()};
    }

    if (property.declaring_type_id == "sasd.ui.RadioButton" && property.property_id == "selected") {
        const auto* radioButton = dynamic_cast<const RadioButton*>(&component);
        return radioButton == nullptr ? std::nullopt : std::optional<bool>{radioButton->isSelected()};
    }

    return std::nullopt;
}

std::optional<std::string> readBuiltInStringProperty(
    const Component& component,
    const ComponentPropertyMetadata& property) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::string) {
        return std::nullopt;
    }

    if (isCommandProperty(property, "text", ComponentPropertyValueKind::string)) {
        const auto* command = dynamic_cast<const Command*>(&component);
        return command == nullptr ? std::nullopt : std::optional<std::string>{command->text()};
    }
    if (property.declaring_type_id == "sasd.ui.Label" && property.property_id == "text") {
        const auto* label = dynamic_cast<const Label*>(&component);
        return label == nullptr ? std::nullopt : std::optional<std::string>{label->text()};
    }
    if (property.declaring_type_id == "sasd.ui.Button" && property.property_id == "text") {
        const auto* button = dynamic_cast<const Button*>(&component);
        return button == nullptr ? std::nullopt : std::optional<std::string>{button->text()};
    }
    if (property.declaring_type_id == "sasd.ui.TextField" && property.property_id == "text") {
        const auto* text_field = dynamic_cast<const TextField*>(&component);
        return text_field == nullptr ? std::nullopt
                                     : std::optional<std::string>{text_field->text()};
    }
    if (property.declaring_type_id == "sasd.ui.CheckBox" && property.property_id == "text") {
        const auto* check_box = dynamic_cast<const CheckBox*>(&component);
        return check_box == nullptr ? std::nullopt : std::optional<std::string>{check_box->text()};
    }

    if (property.declaring_type_id == "sasd.ui.RadioButton" && property.property_id == "text") {
        const auto* radioButton = dynamic_cast<const RadioButton*>(&component);
        return radioButton == nullptr ? std::nullopt : std::optional<std::string>{radioButton->text()};
    }

    return std::nullopt;
}

bool writeBuiltInBooleanProperty(
    Component& component,
    const ComponentPropertyMetadata& property,
    bool value) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::boolean) {
        return false;
    }

    if (isWidgetProperty(property, "visible")) {
        auto* widget = dynamic_cast<Widget*>(&component);
        if (widget == nullptr) {
            return false;
        }
        widget->setVisible(value);
        return true;
    }
    if (isWidgetProperty(property, "enabled")) {
        auto* widget = dynamic_cast<Widget*>(&component);
        if (widget == nullptr) {
            return false;
        }
        widget->setEnabled(value);
        return true;
    }
    if (isWidgetProperty(property, "focusable")) {
        auto* widget = dynamic_cast<Widget*>(&component);
        if (widget == nullptr) {
            return false;
        }
        widget->setFocusable(value);
        return true;
    }
    if (isCommandProperty(property, "enabled", ComponentPropertyValueKind::boolean)) {
        auto* command = dynamic_cast<Command*>(&component);
        if (command == nullptr) {
            return false;
        }
        command->setEnabled(value);
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.CheckBox" && property.property_id == "checked") {
        auto* check_box = dynamic_cast<CheckBox*>(&component);
        if (check_box == nullptr) {
            return false;
        }
        // setChecked() reports value change, while this layer reports valid property access.
        (void)check_box->setChecked(value);
        return true;
    }

    if (property.declaring_type_id == "sasd.ui.RadioButton" && property.property_id == "selected") {
        auto* radioButton = dynamic_cast<RadioButton*>(&component);
        if (radioButton == nullptr) {
            return false;
        }
        // Only the semantic setter preserves group exclusivity and callback ordering. Its bool
        // reports state change; property success includes same-value writes. The callback may
        // destroy the button, so do not access it again after entering the setter.
        (void)radioButton->setSelected(value);
        return true;
    }

    return false;
}

bool writeBuiltInStringProperty(
    Component& component,
    const ComponentPropertyMetadata& property,
    std::string value) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::string) {
        return false;
    }

    if (isCommandProperty(property, "text", ComponentPropertyValueKind::string)) {
        auto* command = dynamic_cast<Command*>(&component);
        if (command == nullptr) {
            return false;
        }
        command->setText(std::move(value));
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.Label" && property.property_id == "text") {
        auto* label = dynamic_cast<Label*>(&component);
        if (label == nullptr) {
            return false;
        }
        label->setText(std::move(value));
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.Button" && property.property_id == "text") {
        auto* button = dynamic_cast<Button*>(&component);
        if (button == nullptr) {
            return false;
        }
        button->setText(std::move(value));
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.TextField" && property.property_id == "text") {
        auto* text_field = dynamic_cast<TextField*>(&component);
        if (text_field == nullptr) {
            return false;
        }
        text_field->setText(std::move(value));
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.CheckBox" && property.property_id == "text") {
        auto* check_box = dynamic_cast<CheckBox*>(&component);
        if (check_box == nullptr) {
            return false;
        }
        check_box->setText(std::move(value));
        return true;
    }

    if (property.declaring_type_id == "sasd.ui.RadioButton" && property.property_id == "text") {
        auto* radioButton = dynamic_cast<RadioButton*>(&component);
        if (radioButton == nullptr) {
            return false;
        }
        radioButton->setText(std::move(value));
        return true;
    }

    return false;
}


std::optional<std::int64_t> readBuiltInSignedIntegerProperty(
    const Component& component,
    const ComponentPropertyMetadata& property) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::signed_integer) {
        return std::nullopt;
    }
    if (property.declaring_type_id == "sasd.ui.HBox" &&
        property.property_id == "spacing") {
        const auto* layout = dynamic_cast<const HBox*>(&component);
        if (layout == nullptr) {
            return std::nullopt;
        }
        const auto value = layout->spacing();
        if (!std::in_range<std::int64_t>(value)) {
            return std::nullopt;
        }
        return static_cast<std::int64_t>(value);
    }
    if (property.declaring_type_id == "sasd.ui.VBox" &&
        property.property_id == "spacing") {
        const auto* layout = dynamic_cast<const VBox*>(&component);
        if (layout == nullptr) {
            return std::nullopt;
        }
        const auto value = layout->spacing();
        if (!std::in_range<std::int64_t>(value)) {
            return std::nullopt;
        }
        return static_cast<std::int64_t>(value);
    }
    if (property.declaring_type_id == "sasd.ui.GridLayout" &&
        property.property_id == "columnSpacing") {
        const auto* layout = dynamic_cast<const GridLayout*>(&component);
        if (layout == nullptr) {
            return std::nullopt;
        }
        const auto value = layout->columnSpacing();
        if (!std::in_range<std::int64_t>(value)) {
            return std::nullopt;
        }
        return static_cast<std::int64_t>(value);
    }
    if (property.declaring_type_id == "sasd.ui.GridLayout" &&
        property.property_id == "rowSpacing") {
        const auto* layout = dynamic_cast<const GridLayout*>(&component);
        if (layout == nullptr) {
            return std::nullopt;
        }
        const auto value = layout->rowSpacing();
        if (!std::in_range<std::int64_t>(value)) {
            return std::nullopt;
        }
        return static_cast<std::int64_t>(value);
    }
    if (property.declaring_type_id == "sasd.ui.FormLayout" &&
        property.property_id == "columnSpacing") {
        const auto* layout = dynamic_cast<const FormLayout*>(&component);
        if (layout == nullptr) {
            return std::nullopt;
        }
        const auto value = layout->columnSpacing();
        if (!std::in_range<std::int64_t>(value)) {
            return std::nullopt;
        }
        return static_cast<std::int64_t>(value);
    }
    if (property.declaring_type_id == "sasd.ui.FormLayout" &&
        property.property_id == "rowSpacing") {
        const auto* layout = dynamic_cast<const FormLayout*>(&component);
        if (layout == nullptr) {
            return std::nullopt;
        }
        const auto value = layout->rowSpacing();
        if (!std::in_range<std::int64_t>(value)) {
            return std::nullopt;
        }
        return static_cast<std::int64_t>(value);
    }
    return std::nullopt;
}

bool writeBuiltInSignedIntegerProperty(
    Component& component,
    const ComponentPropertyMetadata& property,
    std::int64_t value) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::signed_integer) {
        return false;
    }
    // Representation belongs to this bridge; semantic domain validation belongs to the setter.
    // Check before casting or calling any setter so narrowing failure cannot mutate the Component.
    if (!std::in_range<Coordinate>(value)) {
        return false;
    }
    const auto concrete_value = static_cast<Coordinate>(value);
    if (property.declaring_type_id == "sasd.ui.HBox" &&
        property.property_id == "spacing") {
        auto* layout = dynamic_cast<HBox*>(&component);
        if (layout == nullptr) {
            return false;
        }
        layout->setSpacing(concrete_value);
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.VBox" &&
        property.property_id == "spacing") {
        auto* layout = dynamic_cast<VBox*>(&component);
        if (layout == nullptr) {
            return false;
        }
        layout->setSpacing(concrete_value);
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.GridLayout" &&
        property.property_id == "columnSpacing") {
        auto* layout = dynamic_cast<GridLayout*>(&component);
        if (layout == nullptr) {
            return false;
        }
        layout->setColumnSpacing(concrete_value);
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.GridLayout" &&
        property.property_id == "rowSpacing") {
        auto* layout = dynamic_cast<GridLayout*>(&component);
        if (layout == nullptr) {
            return false;
        }
        layout->setRowSpacing(concrete_value);
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.FormLayout" &&
        property.property_id == "columnSpacing") {
        auto* layout = dynamic_cast<FormLayout*>(&component);
        if (layout == nullptr) {
            return false;
        }
        layout->setColumnSpacing(concrete_value);
        return true;
    }
    if (property.declaring_type_id == "sasd.ui.FormLayout" &&
        property.property_id == "rowSpacing") {
        auto* layout = dynamic_cast<FormLayout*>(&component);
        if (layout == nullptr) {
            return false;
        }
        layout->setRowSpacing(concrete_value);
        return true;
    }
    return false;
}

std::optional<std::uint64_t> readBuiltInUnsignedIntegerProperty(
    const Component& component,
    const ComponentPropertyMetadata& property) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::unsigned_integer) {
        return std::nullopt;
    }
    if (property.declaring_type_id == "sasd.ui.GridLayout" &&
        property.property_id == "columnCount") {
        const auto* layout = dynamic_cast<const GridLayout*>(&component);
        if (layout == nullptr) {
            return std::nullopt;
        }
        const auto value = layout->columnCount();
        if (!std::in_range<std::uint64_t>(value)) {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(value);
    }
    return std::nullopt;
}

bool writeBuiltInUnsignedIntegerProperty(
    Component& component,
    const ComponentPropertyMetadata& property,
    std::uint64_t value) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::unsigned_integer) {
        return false;
    }
    // Representation belongs to this bridge; semantic domain validation belongs to the setter.
    // Check before casting or calling any setter so narrowing failure cannot mutate the Component.
    if (!std::in_range<std::size_t>(value)) {
        return false;
    }
    const auto concrete_value = static_cast<std::size_t>(value);
    if (property.declaring_type_id == "sasd.ui.GridLayout" &&
        property.property_id == "columnCount") {
        auto* layout = dynamic_cast<GridLayout*>(&component);
        if (layout == nullptr) {
            return false;
        }
        layout->setColumnCount(concrete_value);
        return true;
    }
    return false;
}

std::optional<std::vector<std::string>> readBuiltInStringCollectionProperty(
    const Component& component, const ComponentPropertyMetadata& property) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::string_collection ||
        property.declaring_type_id != "sasd.ui.ComboBox" || property.property_id != "items") {
        return std::nullopt;
    }
    const auto* combo = dynamic_cast<const ComboBox*>(&component);
    if (combo == nullptr) {
        return std::nullopt;
    }
    std::vector<std::string> value;
    value.reserve(combo->itemCount());
    for (std::size_t index = 0; index < combo->itemCount(); ++index) {
        value.emplace_back(combo->itemAt(index));
    }
    return value;
}

bool writeBuiltInStringCollectionProperty(
    Component& component, const ComponentPropertyMetadata& property, std::vector<std::string> value) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::string_collection ||
        property.declaring_type_id != "sasd.ui.ComboBox" || property.property_id != "items") {
        return false;
    }
    auto* combo = dynamic_cast<ComboBox*>(&component);
    if (combo == nullptr) {
        return false;
    }
    // Only the semantic setter owns selection clearing, invalidation and callback delivery.
    // Its changed bool is not our access result. The callback may destroy combo: do not touch it.
    (void)combo->setItems(std::move(value));
    return true;
}

std::optional<std::optional<std::uint64_t>> readBuiltInOptionalUnsignedIntegerProperty(
    const Component& component, const ComponentPropertyMetadata& property) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::optional_unsigned_integer ||
        property.declaring_type_id != "sasd.ui.ComboBox" || property.property_id != "selectedIndex") {
        return std::nullopt;
    }
    const auto* combo = dynamic_cast<const ComboBox*>(&component);
    if (combo == nullptr) {
        return std::nullopt;
    }
    const auto index = combo->selectedIndex();
    if (!index.has_value()) {
        // Explicit in_place engages the access result while preserving semantic absence.
        return std::optional<std::optional<std::uint64_t>>{std::in_place, std::nullopt};
    }
    if (!std::in_range<std::uint64_t>(*index)) {
        return std::nullopt;
    }
    return std::optional<std::optional<std::uint64_t>>{
        std::in_place, static_cast<std::uint64_t>(*index)};
}

bool writeBuiltInOptionalUnsignedIntegerProperty(
    Component& component, const ComponentPropertyMetadata& property, std::optional<std::uint64_t> value) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::optional_unsigned_integer ||
        property.declaring_type_id != "sasd.ui.ComboBox" || property.property_id != "selectedIndex") {
        return false;
    }
    auto* combo = dynamic_cast<ComboBox*>(&component);
    if (combo == nullptr || (value.has_value() && !std::in_range<std::size_t>(*value))) {
        return false;
    }
    std::optional<std::size_t> index;
    if (value.has_value()) {
        index = static_cast<std::size_t>(*value);
    }
    // Representation is checked here; item-count domain validation stays in the setter and its
    // out_of_range propagates. Ignore changed bool and never touch combo after callback delivery.
    (void)combo->setSelectedIndex(index);
    return true;
}

std::optional<SizeConstraints> readBuiltInSizeConstraintsProperty(
    const Component& component, const ComponentPropertyMetadata& property) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::size_constraints ||
        property.declaring_type_id != "sasd.ui.Widget" || property.property_id != "sizeConstraints") {
        return std::nullopt;
    }
    const auto* widget = dynamic_cast<const Widget*>(&component);
    return widget == nullptr ? std::nullopt : std::optional<SizeConstraints>{widget->sizeConstraints()};
}

bool writeBuiltInSizeConstraintsProperty(
    Component& component, const ComponentPropertyMetadata& property, SizeConstraints value) {
    if (!isBuiltInSchemaDescriptor(property) ||
        property.value_kind != ComponentPropertyValueKind::size_constraints ||
        property.declaring_type_id != "sasd.ui.Widget" || property.property_id != "sizeConstraints") {
        return false;
    }
    auto* widget = dynamic_cast<Widget*>(&component);
    if (widget == nullptr) {
        return false;
    }
    // Keep range validation atomic in the semantic setter. In particular, preferred need not lie
    // inside the range, and negative hints are not forbidden. Domain exceptions must propagate;
    // false means an access mismatch, not a rejected domain value. No layout state is restored.
    widget->setSizeConstraints(value);
    return true;
}

} // namespace sasd::ui
