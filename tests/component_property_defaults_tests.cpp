#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/tree_view.hpp>
#include <sasd/ui/window.hpp>

#include "test_framework.hpp"

#include <optional>
#include <string_view>

namespace sasd::ui {
namespace {

const ComponentPropertyMetadata& property(std::string_view target_type_id,
                                          std::string_view property_id) {
    const auto* result = findBuiltInComponentProperty(target_type_id, property_id);
    CHECK(result != nullptr);
    return *result;
}

} // namespace

TEST_CASE("built-in property defaults use effective target type semantics") {
    const auto& visible = property("sasd.ui.Widget", "visible");
    const auto& enabled = property("sasd.ui.Widget", "enabled");
    const auto& focusable = property("sasd.ui.Widget", "focusable");

    CHECK(builtInBooleanPropertyDefault("sasd.ui.Widget", visible) == std::optional<bool>{true});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.Widget", enabled) == std::optional<bool>{true});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.Widget", focusable) == std::optional<bool>{false});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.Label", focusable) == std::optional<bool>{false});

    CHECK(builtInBooleanPropertyDefault("sasd.ui.Button", focusable) == std::optional<bool>{true});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.TextField", focusable) == std::optional<bool>{true});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.CheckBox", focusable) == std::optional<bool>{true});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.RadioButton", focusable) == std::optional<bool>{true});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.ComboBox", focusable) == std::optional<bool>{true});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.ListView", focusable) == std::optional<bool>{true});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.TableView", focusable) == std::optional<bool>{true});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.TreeView", focusable) == std::optional<bool>{true});

    for (const auto target_type_id : {"sasd.ui.Container", "sasd.ui.Window", "sasd.ui.HBox",
                                      "sasd.ui.VBox", "sasd.ui.GridLayout", "sasd.ui.FormLayout",
                                      "sasd.ui.StackLayout"}) {
        CHECK(builtInBooleanPropertyDefault(target_type_id, focusable) == std::optional<bool>{false});
    }
}

TEST_CASE("built-in direct property defaults are empty strings and explicit booleans") {
    CHECK(builtInStringPropertyDefault("sasd.ui.Command", property("sasd.ui.Command", "text")) ==
          std::optional<std::string_view>{std::string_view{}});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.Command", property("sasd.ui.Command", "enabled")) ==
          std::optional<bool>{true});
    CHECK(builtInStringPropertyDefault("sasd.ui.Label", property("sasd.ui.Label", "text")) ==
          std::optional<std::string_view>{std::string_view{}});
    CHECK(builtInStringPropertyDefault("sasd.ui.Button", property("sasd.ui.Button", "text")) ==
          std::optional<std::string_view>{std::string_view{}});
    CHECK(builtInStringPropertyDefault("sasd.ui.TextField", property("sasd.ui.TextField", "text")) ==
          std::optional<std::string_view>{std::string_view{}});
    CHECK(builtInStringPropertyDefault("sasd.ui.CheckBox", property("sasd.ui.CheckBox", "text")) ==
          std::optional<std::string_view>{std::string_view{}});
    CHECK(builtInBooleanPropertyDefault("sasd.ui.CheckBox", property("sasd.ui.CheckBox", "checked")) ==
          std::optional<bool>{false});
}

TEST_CASE("default queries validate complete descriptor identity and fail closed") {
    const auto& focusable = property("sasd.ui.Button", "focusable");
    const auto& label_text = property("sasd.ui.Label", "text");
    const auto& button_text = property("sasd.ui.Button", "text");

    const ComponentPropertyMetadata copied = focusable;
    CHECK(builtInBooleanPropertyDefault("sasd.ui.Button", copied) == std::optional<bool>{true});
    CHECK(!builtInBooleanPropertyDefault("sasd.ui.DoesNotExist", focusable).has_value());

    const ComponentPropertyMetadata fabricated{"sasd.ui.Widget", "focusable",
                                               ComponentPropertyValueKind::string};
    const ComponentPropertyMetadata wrong_declaring{"sasd.ui.Label", "focusable",
                                                    ComponentPropertyValueKind::boolean};
    const ComponentPropertyMetadata wrong_case{"sasd.ui.Widget", "Focusable",
                                               ComponentPropertyValueKind::boolean};
    CHECK(!builtInStringPropertyDefault("sasd.ui.Button", focusable).has_value());
    CHECK(!builtInBooleanPropertyDefault("sasd.ui.Button", fabricated).has_value());
    CHECK(!builtInBooleanPropertyDefault("sasd.ui.Button", wrong_declaring).has_value());
    CHECK(!builtInBooleanPropertyDefault("sasd.ui.Button", wrong_case).has_value());
    CHECK(!builtInStringPropertyDefault("sasd.ui.Button", label_text).has_value());
    CHECK(!builtInStringPropertyDefault("sasd.ui.Label", button_text).has_value());
}

TEST_CASE("default metadata agrees with simple fresh built-in construction") {
    Widget widget;
    Label label;
    Button button;
    TextField text_field;
    CheckBox check_box;
    RadioButton radio_button;
    ComboBox combo_box;
    ListView list_view;
    TableView table_view;
    TreeView tree_view;
    Command command;

    CHECK(widget.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.Widget", property("sasd.ui.Widget", "focusable")));
    CHECK(label.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.Label", property("sasd.ui.Label", "focusable")));
    CHECK(button.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.Button", property("sasd.ui.Button", "focusable")));
    CHECK(text_field.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.TextField", property("sasd.ui.TextField", "focusable")));
    CHECK(check_box.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.CheckBox", property("sasd.ui.CheckBox", "focusable")));
    CHECK(radio_button.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.RadioButton", property("sasd.ui.RadioButton", "focusable")));
    CHECK(combo_box.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.ComboBox", property("sasd.ui.ComboBox", "focusable")));
    CHECK(list_view.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.ListView", property("sasd.ui.ListView", "focusable")));
    CHECK(table_view.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.TableView", property("sasd.ui.TableView", "focusable")));
    CHECK(tree_view.isFocusable() == *builtInBooleanPropertyDefault(
        "sasd.ui.TreeView", property("sasd.ui.TreeView", "focusable")));
    CHECK(command.isEnabled() == *builtInBooleanPropertyDefault(
        "sasd.ui.Command", property("sasd.ui.Command", "enabled")));
    CHECK(check_box.isChecked() == *builtInBooleanPropertyDefault(
        "sasd.ui.CheckBox", property("sasd.ui.CheckBox", "checked")));
}

} // namespace sasd::ui
