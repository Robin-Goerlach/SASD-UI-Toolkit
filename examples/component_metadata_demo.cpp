#include <sasd/ui/component_metadata.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_metadata.hpp>

#include <iostream>
#include <string_view>

namespace {

const char* kindName(sasd::ui::ComponentTypeKind kind) noexcept {
    switch (kind) {
    case sasd::ui::ComponentTypeKind::component:
        return "component";
    case sasd::ui::ComponentTypeKind::widget:
        return "widget";
    case sasd::ui::ComponentTypeKind::container:
        return "container";
    }
    return "unknown";
}

const char* valueKindName(sasd::ui::ComponentPropertyValueKind kind) noexcept {
    switch (kind) {
    case sasd::ui::ComponentPropertyValueKind::boolean:
        return "boolean";
    case sasd::ui::ComponentPropertyValueKind::string:
        return "string";
    }
    return "unknown";
}

void printProperties(std::string_view type_id) {
    std::cout << type_id << " declared properties:\n";
    for (const auto& property : sasd::ui::declaredBuiltInComponentProperties(type_id)) {
        std::cout << "  " << property.property_id << " : "
                  << valueKindName(property.value_kind) << '\n';
    }
}

} // namespace

int main() {
    for (const auto& metadata : sasd::ui::builtInComponentTypes()) {
        std::cout << metadata.type_id << " <- "
                  << (metadata.base_type_id.empty() ? "<none>" : metadata.base_type_id)
                  << " [" << kindName(metadata.kind) << "]\n";
    }

    const auto* button_type = sasd::ui::findBuiltInComponentType("sasd.ui.Button");
    std::cout << "lookup sasd.ui.Button: " << (button_type != nullptr ? "found" : "not found") << '\n';
    const auto* unknown = sasd::ui::findBuiltInComponentType("sasd.ui.DoesNotExist");
    std::cout << "lookup sasd.ui.DoesNotExist: " << (unknown != nullptr ? "found" : "not found")
              << '\n';

    printProperties("sasd.ui.CheckBox");
    for (const auto property_id : {"visible", "enabled", "focusable"}) {
        const auto* property =
            sasd::ui::findBuiltInComponentProperty("sasd.ui.CheckBox", property_id);
        std::cout << "  inherited lookup " << property_id << " -> "
                  << (property != nullptr ? property->declaring_type_id : "<not found>") << '\n';
    }

    sasd::ui::Button button{"Initial"};
    const auto* visible =
        sasd::ui::findBuiltInComponentProperty("sasd.ui.Button", "visible");
    const auto* text = sasd::ui::findBuiltInComponentProperty("sasd.ui.Button", "text");
    if (visible != nullptr && text != nullptr) {
        std::cout << "button.visible before: "
                  << *sasd::ui::readBuiltInBooleanProperty(button, *visible) << '\n';
        (void)sasd::ui::writeBuiltInBooleanProperty(button, *visible, false);
        (void)sasd::ui::writeBuiltInStringProperty(button, *text, "Updated");
        std::cout << "button.visible after: "
                  << *sasd::ui::readBuiltInBooleanProperty(button, *visible) << '\n';
        std::cout << "button.text after: "
                  << *sasd::ui::readBuiltInStringProperty(button, *text) << '\n';
    }

    const auto* focusable =
        sasd::ui::findBuiltInComponentProperty("sasd.ui.Button", "focusable");
    const auto* checked =
        sasd::ui::findBuiltInComponentProperty("sasd.ui.CheckBox", "checked");
    if (focusable != nullptr && checked != nullptr) {
        std::cout << "Widget.focusable default: "
                  << *sasd::ui::builtInBooleanPropertyDefault("sasd.ui.Widget", *focusable) << '\n';
        std::cout << "Label.focusable default: "
                  << *sasd::ui::builtInBooleanPropertyDefault("sasd.ui.Label", *focusable) << '\n';
        std::cout << "Button.focusable default: "
                  << *sasd::ui::builtInBooleanPropertyDefault("sasd.ui.Button", *focusable) << '\n';
        std::cout << "CheckBox.checked default: "
                  << *sasd::ui::builtInBooleanPropertyDefault("sasd.ui.CheckBox", *checked) << '\n';
    }

    const auto* button_text =
        sasd::ui::findBuiltInComponentProperty("sasd.ui.Button", "text");
    if (button_text != nullptr) {
        std::cout << "Button.text default: "
                  << *sasd::ui::builtInStringPropertyDefault("sasd.ui.Button", *button_text)
                  << '\n';
    }
}
