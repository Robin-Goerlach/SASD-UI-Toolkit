#include <sasd/ui/component_metadata.hpp>
#include <sasd/ui/component_runtime_metadata.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_property_validation_metadata.hpp>
#include <sasd/ui/component_property_snapshot.hpp>
#include <sasd/ui/component_ownership_snapshot.hpp>
#include <sasd/ui/component_reference_snapshot.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <iomanip>
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
    case Kind::list_view_selection_model: return "list_view_selection_model";
    case Kind::table_view_selection_model: return "table_view_selection_model";
    case Kind::tree_view_selection_model: return "tree_view_selection_model";
    case Kind::list_selection_model_model: return "list_selection_model_model";
    case Kind::table_selection_model_model: return "table_selection_model_model";
    case Kind::tree_selection_model_model: return "tree_selection_model_model";
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
    case sasd::ui::ComponentPropertyValueKind::string_collection:
        return "string_collection";
    case sasd::ui::ComponentPropertyValueKind::text_style:
        return "text_style";
    case sasd::ui::ComponentPropertyValueKind::size_constraints:
        return "size_constraints";
    case sasd::ui::ComponentPropertyValueKind::optional_unsigned_integer:
        return "optional_unsigned_integer";
    }
    return "unknown";
}

const char* normalizationKindName(sasd::ui::StringPropertyNormalizationKind kind) noexcept {
    switch (kind) {
    case sasd::ui::StringPropertyNormalizationKind::identity:
        return "identity";
    case sasd::ui::StringPropertyNormalizationKind::single_line_utf8:
        return "single-line-utf8";
    }
    return "unknown";
}

const char* sizeConstraintsValidationKindName(
    sasd::ui::SizeConstraintsPropertyValidationKind kind) noexcept {
    switch (kind) {
    case sasd::ui::SizeConstraintsPropertyValidationKind::minimum_not_greater_than_maximum:
        return "minimum <= maximum";
    }
    return "unknown";
}

void printProperties(std::string_view type_id) {
    std::cout << type_id << " declared properties:\n";
    for (const auto& property : sasd::ui::declaredBuiltInComponentProperties(type_id)) {
        std::cout << "  " << property.property_id << " : "
                  << valueKindName(property.value_kind);
        if (const auto range = sasd::ui::builtInSignedIntegerPropertyRange(property)) {
            std::cout << " range: " << range->minimum << " .. " << range->maximum;
        }
        if (const auto range = sasd::ui::builtInUnsignedIntegerPropertyRange(property)) {
            std::cout << " range: " << range->minimum << " .. " << range->maximum;
        }
        if (const auto normalization =
                sasd::ui::builtInStringPropertyNormalization(property)) {
            std::cout << " normalization: " << normalizationKindName(*normalization);
        }
        if (const auto validation =
                sasd::ui::builtInSizeConstraintsPropertyValidation(property)) {
            std::cout << " validation: " << sizeConstraintsValidationKindName(*validation);
        }
        std::cout << '\n';
    }
}

} // namespace

int main() {
    sasd::ui::ComboBox combo{{"One", "Two", "Three"}};
    (void)combo.setSelectedIndex(1);
    const auto combo_snapshot = sasd::ui::captureBuiltInComponentPropertySnapshot(combo);
    if (!combo_snapshot.has_value()) {
        return 1;
    }
    std::cout << "ComboBox snapshot:\n  items = [";
    const auto& items = combo_snapshot->string_collection_properties.front();
    for (std::size_t index = 0; index < items.value.size(); ++index) {
        if (index != 0) {
            std::cout << ", ";
        }
        std::cout << std::quoted(items.value[index]);
    }
    std::cout << "] [" << (items.is_default ? "default" : "non-default") << "]\n";
    const auto& combo_selection = combo_snapshot->optional_unsigned_integer_properties.front();
    std::cout << "  selectedIndex = ";
    if (combo_selection.value.has_value()) {
        std::cout << *combo_selection.value;
    } else {
        std::cout << "none";
    }
    std::cout << " [" << (combo_selection.is_default ? "default" : "non-default") << "]\n";

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
    printProperties("sasd.ui.Label");
    printProperties("sasd.ui.TextField");
    printProperties("sasd.ui.Widget");
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

    button.setSizeConstraints({{10, 5}, {40, 20}, {100, 50}});
    const auto button_snapshot =
        sasd::ui::captureBuiltInComponentPropertySnapshot(button);
    if (button_snapshot.has_value()) {
        std::cout << "Button snapshot:\n";
        for (const auto& property : button_snapshot->size_constraints_properties) {
            const auto& value = property.value;
            std::cout << "  " << property.property_id << ": min = "
                      << value.minimum.width << 'x' << value.minimum.height
                      << ", preferred = " << value.preferred.width << 'x' << value.preferred.height
                      << ", max = " << value.maximum.width << 'x' << value.maximum.height
                      << (property.is_default ? " [default]" : " [non-default]") << '\n';
        }
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
    sasd::ui::Label styledLabel{"Important"};
    styledLabel.setTextStyle({sasd::ui::Color::bright_cyan, true, false, true, false});
    const auto styledSnapshot = sasd::ui::captureBuiltInComponentPropertySnapshot(styledLabel);
    if (!styledSnapshot.has_value()) {
        return 1;
    }
    const auto& styleProperty = styledSnapshot->text_style_properties.front();
    std::cout << "Label snapshot:\n  text = \""
              << styledSnapshot->string_properties.front().value << "\"\n  textStyle:\n"
              << "    foreground = "
              << (styleProperty.value.foreground == sasd::ui::Color::bright_cyan
                      ? "bright_cyan" : "unexpected example color")
              << "\n    bold = " << std::boolalpha << styleProperty.value.bold
              << "\n    underline = " << styleProperty.value.underline
              << (styleProperty.is_default ? " [default]" : " [non-default]") << '\n';

    sasd::ui::Window root;
    auto& command = root.emplace<sasd::ui::Command>("Save");
    root.emplace<sasd::ui::Button>().bindCommand(command);
    auto& group = root.emplace<sasd::ui::RadioGroup>();
    root.emplace<sasd::ui::RadioButton>(group, "Choice");
    auto& model = root.emplace<sasd::ui::StringListModel>();
    auto& selection = root.emplace<sasd::ui::ListSelectionModel>();
    selection.setModel(&model);
    auto& view = root.emplace<sasd::ui::ListView>();
    view.setModel(&model);
    view.setSelectionModel(&selection);
    auto& sharedView = root.emplace<sasd::ui::ListView>();
    sharedView.setModel(&model);
    sharedView.setSelectionModel(&selection);
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
