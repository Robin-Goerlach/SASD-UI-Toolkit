#include <sasd/ui/component_metadata.hpp>
#include <sasd/ui/component_runtime_metadata.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_property_snapshot.hpp>
#include <sasd/ui/component_ownership_snapshot.hpp>
#include <sasd/ui/component_reference_snapshot.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

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

const char* referenceKindName(sasd::ui::BuiltInComponentReferenceKind kind) noexcept {
    using Kind = sasd::ui::BuiltInComponentReferenceKind;
    switch (kind) {
    case Kind::button_command: return "button_command";
    case Kind::radio_button_group: return "radio_button_group";
    case Kind::list_view_model: return "list_view_model";
    case Kind::table_view_model: return "table_view_model";
    case Kind::tree_view_model: return "tree_view_model";
    }
    return "unknown";
}

const char* valueKindName(sasd::ui::ComponentPropertyValueKind kind) noexcept {
    switch (kind) {
    case sasd::ui::ComponentPropertyValueKind::boolean:
        return "boolean";
    case sasd::ui::ComponentPropertyValueKind::string:
        return "string";
    case sasd::ui::ComponentPropertyValueKind::signed_integer:
        return "signed_integer";
    case sasd::ui::ComponentPropertyValueKind::unsigned_integer:
        return "unsigned_integer";
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
    printProperties("sasd.ui.HBox");
    printProperties("sasd.ui.VBox");
    printProperties("sasd.ui.GridLayout");
    printProperties("sasd.ui.FormLayout");
    for (const auto property_id : {"visible", "enabled", "focusable"}) {
        const auto* property =
            sasd::ui::findBuiltInComponentProperty("sasd.ui.CheckBox", property_id);
        std::cout << "  inherited lookup " << property_id << " -> "
                  << (property != nullptr ? property->declaring_type_id : "<not found>") << '\n';
    }

    sasd::ui::Button button{"Initial"};
    sasd::ui::Label label{"Name"};
    const auto* button_runtime_type = sasd::ui::builtInComponentTypeOf(button);
    const auto* label_runtime_type = sasd::ui::builtInComponentTypeOf(label);
    std::cout << "Button runtime type: "
              << (button_runtime_type != nullptr ? button_runtime_type->type_id : "unknown") << '\n';
    std::cout << "Label runtime type: "
              << (label_runtime_type != nullptr ? label_runtime_type->type_id : "unknown") << '\n';

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

    sasd::ui::GridLayout grid;
    const auto* columns =
        sasd::ui::findBuiltInComponentProperty("sasd.ui.GridLayout", "columnCount");
    const auto* spacing =
        sasd::ui::findBuiltInComponentProperty("sasd.ui.GridLayout", "columnSpacing");
    if (columns != nullptr && spacing != nullptr) {
        std::cout << "GridLayout.columnCount default: "
                  << *sasd::ui::builtInUnsignedIntegerPropertyDefault("sasd.ui.GridLayout", *columns)
                  << '\n';
        std::cout << "GridLayout.columnSpacing default: "
                  << *sasd::ui::builtInSignedIntegerPropertyDefault("sasd.ui.GridLayout", *spacing)
                  << '\n';
        (void)sasd::ui::writeBuiltInUnsignedIntegerProperty(grid, *columns, 3);
        (void)sasd::ui::writeBuiltInSignedIntegerProperty(grid, *spacing, 2);
        std::cout << "GridLayout.columnCount after: "
                  << *sasd::ui::readBuiltInUnsignedIntegerProperty(grid, *columns) << '\n';
        std::cout << "GridLayout.columnSpacing after: "
                  << *sasd::ui::readBuiltInSignedIntegerProperty(grid, *spacing) << '\n';
    }
    const auto grid_snapshot = sasd::ui::captureBuiltInComponentPropertySnapshot(grid);
    if (grid_snapshot.has_value()) {
        std::cout << "GridLayout numeric snapshot:\n";
        for (const auto& property : grid_snapshot->signed_integer_properties) {
            std::cout << "  " << property.property_id << " = " << property.value
                      << (property.is_default ? " [default]" : " [non-default]") << '\n';
        }
        for (const auto& property : grid_snapshot->unsigned_integer_properties) {
            std::cout << "  " << property.property_id << " = " << property.value
                      << (property.is_default ? " [default]" : " [non-default]") << '\n';
        }
    }

    const auto button_snapshot =
        sasd::ui::captureBuiltInComponentPropertySnapshot(button);
    if (button_snapshot.has_value()) {
        std::cout << "Button snapshot:\n";
        std::cout << "  type: " << button_snapshot->type_id << '\n';
        for (const auto& property : button_snapshot->boolean_properties) {
            std::cout << "  " << property.property_id << " = " << std::boolalpha
                      << property.value << (property.is_default ? " [default]" : " [non-default]")
                      << '\n';
        }
        for (const auto& property : button_snapshot->string_properties) {
            std::cout << "  " << property.property_id << " = \"" << property.value << "\""
                      << (property.is_default ? " [default]" : " [non-default]") << '\n';
        }
    }
    sasd::ui::Window root;
    auto& command = root.emplace<sasd::ui::Command>("Save");
    root.emplace<sasd::ui::Button>().bindCommand(command);
    auto& group = root.emplace<sasd::ui::RadioGroup>();
    root.emplace<sasd::ui::RadioButton>(group, "Choice");
    auto& model = root.emplace<sasd::ui::StringListModel>();
    root.emplace<sasd::ui::ListView>().setModel(&model);
    auto& box = root.emplace<sasd::ui::VBox>();
    box.emplace<sasd::ui::Label>("Name");
    box.emplace<sasd::ui::TextField>();
    const auto graph = sasd::ui::captureBuiltInComponentReferenceGraphSnapshot(root);
    if (!graph.has_value()) {
        std::cerr << "Reference graph snapshot failed\n";
        return 1;
    }
    for (const auto& node : graph->ownership.nodes) {
        std::cout << "node " << node.node_id << ' ' << node.properties.type_id << "\n  owns:";
        if (node.owned_component_ids.empty()) {
            std::cout << " -";
        }
        for (const auto ownedId : node.owned_component_ids) {
            std::cout << ' ' << ownedId;
        }
        std::cout << '\n';
    }
    for (const auto& edge : graph->reference_edges) {
        std::cout << "reference: node " << edge.source_node_id << " --"
                  << referenceKindName(edge.kind) << "--> node " << edge.target_node_id << '\n';
    }

    sasd::ui::RadioButton radio{"Choice"};
    (void)radio.setSelected(true);
    const auto radioSnapshot = sasd::ui::captureBuiltInComponentPropertySnapshot(radio);
    if (radioSnapshot.has_value()) {
        std::cout << "RadioButton snapshot:\n";
        for (const auto& property : radioSnapshot->boolean_properties) {
            std::cout << "  " << property.property_id << " = " << std::boolalpha << property.value
                      << (property.is_default ? " [default]" : " [non-default]") << '\n';
        }
        for (const auto& property : radioSnapshot->string_properties) {
            std::cout << "  " << property.property_id << " = \"" << property.value << "\""
                      << (property.is_default ? " [default]" : " [non-default]") << '\n';
        }
    }
}
