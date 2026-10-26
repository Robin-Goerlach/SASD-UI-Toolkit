#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_snapshot.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/text_field.hpp>

#include <array>
#include <optional>
#include <string_view>

using namespace sasd::ui;

namespace {
constexpr TextStyle styled{Color::bright_cyan, true, false, true, false};
constexpr std::array owners{"sasd.ui.Label", "sasd.ui.Button", "sasd.ui.TextField",
                            "sasd.ui.CheckBox", "sasd.ui.RadioButton", "sasd.ui.ComboBox"};

void checkStyle(const BuiltInComponentPropertySnapshot& snapshot, TextStyle value, bool isDefault) {
    CHECK(snapshot.text_style_properties.size() == 1);
    const auto& property = snapshot.text_style_properties.front();
    CHECK(property.declaring_type_id == snapshot.type_id);
    CHECK(property.property_id == "textStyle");
    CHECK(property.value == value);
    CHECK(property.is_default == isDefault);
}

template<class Control>
void checkAccess(std::string_view type) {
    Control control;
    const auto descriptor = *findBuiltInComponentProperty(type, "textStyle");
    CHECK(readBuiltInTextStyleProperty(control, descriptor) == TextStyle{});
    CHECK(builtInTextStylePropertyDefault(type, descriptor) == TextStyle{});
    CHECK(writeBuiltInTextStyleProperty(control, descriptor, styled));
    CHECK(control.textStyle() == styled);
    const auto copy = readBuiltInTextStyleProperty(control, descriptor);
    CHECK(copy == styled);
    (void)control.measure();
    control.acknowledgeVisualUpdate();
    CHECK(writeBuiltInTextStyleProperty(control, descriptor, styled));
    CHECK(control.isMeasureValid());
    CHECK(!control.isVisualUpdatePending());
    CHECK(writeBuiltInTextStyleProperty(control, descriptor, {}));
    CHECK(control.isMeasureValid());
    CHECK(control.isVisualUpdatePending());
    CHECK(control.textStyle() == TextStyle{});
    CHECK(copy == styled);
    // Runtime setters accept the complete Color domain; tooling must not invent validation.
    auto unknown = styled;
    unknown.foreground = static_cast<Color>(255);
    CHECK(writeBuiltInTextStyleProperty(control, descriptor, unknown));
    CHECK(readBuiltInTextStyleProperty(control, descriptor) == unknown);
    checkStyle(*captureBuiltInComponentPropertySnapshot(control), unknown, false);

    for (const auto other : owners) {
        if (other == type) { continue; }
        const auto sibling = *findBuiltInComponentProperty(other, "textStyle");
        CHECK(!readBuiltInTextStyleProperty(control, sibling));
        CHECK(!writeBuiltInTextStyleProperty(control, sibling, {}));
        CHECK(!builtInTextStylePropertyDefault(type, sibling));
    }
    const std::array invalid{
        ComponentPropertyMetadata{type, "textStyle", ComponentPropertyValueKind::string},
        ComponentPropertyMetadata{"sasd.ui.Widget", "textStyle", ComponentPropertyValueKind::text_style},
        ComponentPropertyMetadata{type, "TextStyle", ComponentPropertyValueKind::text_style},
        ComponentPropertyMetadata{type, "textstyle", ComponentPropertyValueKind::text_style},
        ComponentPropertyMetadata{type, "resolvedStyle", ComponentPropertyValueKind::text_style},
        ComponentPropertyMetadata{},
    };
    for (const auto& property : invalid) {
        CHECK(!readBuiltInTextStyleProperty(control, property));
        CHECK(!writeBuiltInTextStyleProperty(control, property, {}));
        CHECK(!builtInTextStylePropertyDefault(type, property));
        CHECK(control.textStyle() == unknown);
    }
    Component unrelated;
    CHECK(!readBuiltInTextStyleProperty(unrelated, descriptor));
    CHECK(!writeBuiltInTextStyleProperty(unrelated, descriptor, {}));
    CHECK(!builtInTextStylePropertyDefault("sasd.ui.Component", descriptor));
    CHECK(!builtInTextStylePropertyDefault("SASD.UI.Label", descriptor));
    CHECK(!builtInTextStylePropertyDefault("", descriptor));
    CHECK(!readBuiltInStringProperty(control, descriptor));
    CHECK(!writeBuiltInStringProperty(control, descriptor, "ignored"));
    CHECK(!builtInStringPropertyDefault(type, descriptor));
}
}

TEST_CASE("Inline TextStyle schema belongs only to six controls and follows existing properties") {
    static_assert(static_cast<int>(ComponentPropertyValueKind::text_style) == 7);
    CHECK(builtInComponentTypes().size() == 29);
    std::size_t total = 0;
    std::size_t styles = 0;
    for (const auto& type : builtInComponentTypes()) {
        const auto direct = declaredBuiltInComponentProperties(type.type_id);
        total += direct.size();
        bool ownsStyle = false;
        for (const auto owner : owners) { ownsStyle = ownsStyle || owner == type.type_id; }
        const auto* found = findBuiltInComponentProperty(type.type_id, "textStyle");
        CHECK((found != nullptr) == ownsStyle);
        if (ownsStyle) {
            ++styles;
            CHECK(found == &direct.back());
            CHECK(found->declaring_type_id == type.type_id);
            CHECK(found->value_kind == ComponentPropertyValueKind::text_style);
            const bool combo = type.type_id == "sasd.ui.ComboBox";
            const bool stateful = combo || type.type_id == "sasd.ui.CheckBox" ||
                                  type.type_id == "sasd.ui.RadioButton";
            CHECK(direct.size() == (stateful ? 3U : 2U));
            CHECK(direct[0].property_id == (combo ? "items" : "text"));
            if (stateful) {
                CHECK(direct[1].property_id == (combo ? "selectedIndex" :
                      type.type_id == "sasd.ui.CheckBox" ? "checked" : "selected"));
            }
        }
        for (const auto excluded : {"focusedStyle", "disabledStyle", "pressedStyle",
                                   "selectionStyle", "resolvedStyle"}) {
            CHECK(findBuiltInComponentProperty(type.type_id, excluded) == nullptr);
        }
    }
    CHECK(total == 28);
    CHECK(styles == 6);
}

TEST_CASE("All six inline TextStyle adapters preserve defaults values identities and setter semantics") {
    checkAccess<Label>(owners[0]);
    checkAccess<Button>(owners[1]);
    checkAccess<TextField>(owners[2]);
    checkAccess<CheckBox>(owners[3]);
    checkAccess<RadioButton>(owners[4]);
    checkAccess<ComboBox>(owners[5]);
}

TEST_CASE("Inline TextStyle snapshots own authoring values and retain existing collections") {
    Label label;
    const auto initial = captureBuiltInComponentPropertySnapshot(label);
    CHECK(initial.has_value());
    checkStyle(*initial, {}, true);
    CHECK(initial->boolean_properties.size() == 3);
    CHECK(initial->boolean_properties[0].property_id == "visible");
    CHECK(initial->boolean_properties[0].value);
    CHECK(initial->boolean_properties[1].property_id == "enabled");
    CHECK(initial->boolean_properties[1].value);
    CHECK(initial->boolean_properties[2].property_id == "focusable");
    CHECK(!initial->boolean_properties[2].value);
    for (const auto& property : initial->boolean_properties) { CHECK(property.is_default); }
    CHECK(initial->string_properties.size() == 1);
    CHECK(initial->string_properties[0].declaring_type_id == "sasd.ui.Label");
    CHECK(initial->string_properties[0].value.empty());
    CHECK(initial->string_properties[0].is_default);
    CHECK(initial->size_constraints_properties.size() == 1);
    CHECK(initial->size_constraints_properties[0].value == SizeConstraints{});
    CHECK(initial->size_constraints_properties[0].is_default);
    CHECK(initial->signed_integer_properties.empty());
    CHECK(initial->unsigned_integer_properties.empty());
    CHECK(initial->string_collection_properties.empty());
    CHECK(initial->optional_unsigned_integer_properties.empty());
    label.setText("Important");
    label.setTextStyle(styled);
    const auto captured = captureBuiltInComponentPropertySnapshot(label);
    checkStyle(*captured, styled, false);
    CHECK(captured->string_properties[0].value == "Important");
    label.setTextStyle({});
    label.setText("Changed");
    checkStyle(*captured, styled, false);
    CHECK(captured->string_properties[0].value == "Important");
    Button button;
    checkStyle(*captureBuiltInComponentPropertySnapshot(button), {}, true);
    // Disabled/focused appearance is resolved later; capture sees only the semantic getter.
    FocusManager focus;
    CHECK(focus.requestFocus(button));
    checkStyle(*captureBuiltInComponentPropertySnapshot(button), {}, true);
    CHECK(!button.textStyle().inverse);
    button.setEnabled(false);
    const auto disabled = captureBuiltInComponentPropertySnapshot(button);
    checkStyle(*disabled, {}, true);
    CHECK(!button.textStyle().dim);
    CheckBox check{"Choice", true};
    check.setTextStyle(styled);
    const auto checked = captureBuiltInComponentPropertySnapshot(check);
    checkStyle(*checked, styled, false);
    CHECK(checked->boolean_properties.back().property_id == "checked");
    CHECK(checked->boolean_properties.back().value);
    CHECK(checked->string_properties[0].value == "Choice");
    std::optional<BuiltInComponentPropertySnapshot> owned;
    const SizeConstraints hints{{1, 2}, {20, 10}, {100, 50}};
    {
        ComboBox combo{{"One", "Two"}};
        (void)combo.setSelectedIndex(1);
        combo.setSizeConstraints(hints);
        combo.setTextStyle(styled);
        owned = captureBuiltInComponentPropertySnapshot(combo);
        (void)combo.setItems({"Changed"});
        combo.setTextStyle({});
        combo.setSizeConstraints({});
    }
    checkStyle(*owned, styled, false);
    CHECK(owned->type_id == "sasd.ui.ComboBox");
    CHECK(owned->boolean_properties.size() == 3);
    CHECK(owned->string_properties.empty());
    CHECK(owned->signed_integer_properties.empty());
    CHECK(owned->unsigned_integer_properties.empty());
    CHECK(owned->string_collection_properties.size() == 1);
    CHECK(owned->string_collection_properties[0].value == std::vector<std::string>{"One", "Two"});
    CHECK(!owned->string_collection_properties[0].is_default);
    CHECK(owned->optional_unsigned_integer_properties.size() == 1);
    CHECK(owned->optional_unsigned_integer_properties[0].value == 1);
    CHECK(!owned->optional_unsigned_integer_properties[0].is_default);
    CHECK(owned->size_constraints_properties[0].value == hints);
    CHECK(!owned->size_constraints_properties[0].is_default);
}

TEST_CASE("Inline TextStyle property changes leave text selection focus and callbacks untouched") {
    TextField field{"Content"};
    field.setSelection(1, 4);
    FocusManager focus;
    CHECK(focus.requestFocus(field));
    const auto property = *findBuiltInComponentProperty("sasd.ui.TextField", "textStyle");
    const TextStyle allFlags{Color::bright_red, true, true, true, true};
    CHECK(writeBuiltInTextStyleProperty(field, property, allFlags));
    CHECK(readBuiltInTextStyleProperty(field, property) == allFlags);
    CHECK(field.text() == "Content");
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 4);
    CHECK(field.hasFocus());
    CHECK(field.isFocusable());
    CHECK(field.owner() == nullptr);
    CHECK(field.parent() == nullptr);
    CheckBox check{"Choice", true};
    int notifications = 0;
    check.setOnCheckedChanged([&](bool) { ++notifications; });
    CHECK(writeBuiltInTextStyleProperty(check,
        *findBuiltInComponentProperty("sasd.ui.CheckBox", "textStyle"), allFlags));
    CHECK(check.isChecked());
    CHECK(check.text() == "Choice");
    CHECK(notifications == 0);
}
