#include <sasd/ui/component_metadata.hpp>
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

    const auto* button = sasd::ui::findBuiltInComponentType("sasd.ui.Button");
    std::cout << "lookup sasd.ui.Button: " << (button != nullptr ? "found" : "not found") << '\n';
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
}
