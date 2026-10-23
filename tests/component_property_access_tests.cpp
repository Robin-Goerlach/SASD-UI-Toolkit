#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/text_field.hpp>

#include <memory>
#include <string>

using namespace sasd::ui;

namespace {

const ComponentPropertyMetadata& property(std::string_view type_id, std::string_view property_id) {
    const auto* result = findBuiltInComponentProperty(type_id, property_id);
    // Test data is part of the immutable catalog; a missing descriptor is a test failure at the
    // first use, while the access functions themselves remain fail-closed for runtime callers.
    CHECK(result != nullptr);
    return *result;
}

} // namespace

TEST_CASE("built-in boolean property access reads and writes semantic Widget state") {
    Widget widget;
    const auto& visible = property("sasd.ui.Widget", "visible");
    const auto& enabled = property("sasd.ui.Widget", "enabled");
    const auto& focusable = property("sasd.ui.Widget", "focusable");

    CHECK(readBuiltInBooleanProperty(widget, visible) == std::optional<bool>{true});
    CHECK(readBuiltInBooleanProperty(widget, enabled) == std::optional<bool>{true});
    CHECK(readBuiltInBooleanProperty(widget, focusable) == std::optional<bool>{false});

    CHECK(writeBuiltInBooleanProperty(widget, visible, false));
    CHECK(writeBuiltInBooleanProperty(widget, enabled, false));
    CHECK(writeBuiltInBooleanProperty(widget, focusable, true));
    CHECK(!widget.isVisible());
    CHECK(!widget.isEnabled());
    CHECK(widget.isFocusable());
}

TEST_CASE("inherited Widget property access works on a Button instance") {
    Button button{"Before"};
    const auto& visible = property("sasd.ui.Button", "visible");

    CHECK(visible.declaring_type_id == "sasd.ui.Widget");
    CHECK(writeBuiltInBooleanProperty(button, visible, false));
    CHECK(readBuiltInBooleanProperty(button, visible) == std::optional<bool>{false});
    CHECK(!button.isVisible());
}

TEST_CASE("Command boolean access preserves state observation") {
    Command command{"Run"};
    int notifications = 0;
    auto subscription = command.observeState([&](Command::StateChange change) {
        if (change == Command::StateChange::enabled) {
            ++notifications;
        }
    });
    const auto& enabled = property("sasd.ui.Command", "enabled");

    CHECK(writeBuiltInBooleanProperty(command, enabled, false));
    CHECK(!command.isEnabled());
    CHECK(notifications == 1);
    CHECK(readBuiltInBooleanProperty(command, enabled) == std::optional<bool>{false});
}

TEST_CASE("built-in string property access uses semantic setters") {
    Command command{"Command"};
    Label label{"Label"};
    Button button{"Button"};
    TextField field{"Field"};
    CheckBox check_box{"Check"};

    CHECK(writeBuiltInStringProperty(command, property("sasd.ui.Command", "text"), "Run"));
    CHECK(writeBuiltInStringProperty(label, property("sasd.ui.Label", "text"), "Caption"));
    CHECK(writeBuiltInStringProperty(button, property("sasd.ui.Button", "text"), "Press"));
    CHECK(writeBuiltInStringProperty(field, property("sasd.ui.TextField", "text"), "Edit"));
    CHECK(writeBuiltInStringProperty(check_box, property("sasd.ui.CheckBox", "text"), "Option"));

    CHECK(readBuiltInStringProperty(command, property("sasd.ui.Command", "text")) ==
          std::optional<std::string>{"Run"});
    CHECK(readBuiltInStringProperty(label, property("sasd.ui.Label", "text")) ==
          std::optional<std::string>{"Caption"});
    CHECK(readBuiltInStringProperty(button, property("sasd.ui.Button", "text")) ==
          std::optional<std::string>{"Press"});
    CHECK(readBuiltInStringProperty(field, property("sasd.ui.TextField", "text")) ==
          std::optional<std::string>{"Edit"});
    CHECK(readBuiltInStringProperty(check_box, property("sasd.ui.CheckBox", "text")) ==
          std::optional<std::string>{"Option"});
}

TEST_CASE("TextField string access retains sanitization from setText") {
    TextField field;
    const auto& text = property("sasd.ui.TextField", "text");

    CHECK(writeBuiltInStringProperty(field, text, std::string{"A\nB\xC3("}));
    CHECK(readBuiltInStringProperty(field, text) ==
          std::optional<std::string>{std::string{"AB\xEF\xBF\xBD("}});
}

TEST_CASE("CheckBox access preserves callback semantics and accepts same-value writes") {
    CheckBox check_box{"Option", true};
    int notifications = 0;
    check_box.setOnCheckedChanged([&](bool) { ++notifications; });
    const auto& checked = property("sasd.ui.CheckBox", "checked");

    CHECK(writeBuiltInBooleanProperty(check_box, checked, true));
    CHECK(notifications == 0);
    CHECK(writeBuiltInBooleanProperty(check_box, checked, false));
    CHECK(check_box.isChecked() == false);
    CHECK(notifications == 1);
    CHECK(readBuiltInBooleanProperty(check_box, checked) == std::optional<bool>{false});
}

TEST_CASE("property access fails closed for wrong kinds, identities and component types") {
    Button button{"Button"};
    Label label{"Label"};
    const auto& button_text = property("sasd.ui.Button", "text");
    const auto& label_text = property("sasd.ui.Label", "text");
    const auto& check_box_checked = property("sasd.ui.CheckBox", "checked");

    CHECK(!readBuiltInBooleanProperty(button, button_text).has_value());
    CHECK(!readBuiltInStringProperty(button, property("sasd.ui.Widget", "enabled")).has_value());
    CHECK(!readBuiltInStringProperty(button, label_text).has_value());
    CHECK(!writeBuiltInBooleanProperty(button, check_box_checked, true));
    CHECK(!writeBuiltInStringProperty(label, button_text, "wrong"));

    ComponentPropertyMetadata unknown{"sasd.ui.Button", "unknown", ComponentPropertyValueKind::string};
    ComponentPropertyMetadata wrong_declaring{"sasd.ui.NotButton", "text", ComponentPropertyValueKind::string};
    ComponentPropertyMetadata wrong_kind{"sasd.ui.Button", "text", ComponentPropertyValueKind::boolean};
    ComponentPropertyMetadata wrong_case{"sasd.ui.Button", "TEXT", ComponentPropertyValueKind::string};

    CHECK(!readBuiltInStringProperty(button, unknown).has_value());
    CHECK(!readBuiltInStringProperty(button, wrong_declaring).has_value());
    CHECK(!readBuiltInStringProperty(button, wrong_kind).has_value());
    CHECK(!readBuiltInStringProperty(button, wrong_case).has_value());
    CHECK(!writeBuiltInStringProperty(button, unknown, "nope"));
}

TEST_CASE("copied valid metadata works and access does not alter ownership") {
    Container root;
    auto& button = root.emplace<Button>("Owned");
    const auto original_owner = button.owner();
    const auto source = property("sasd.ui.Button", "text");
    const ComponentPropertyMetadata copied = source;

    CHECK(writeBuiltInStringProperty(button, copied, "Changed"));
    CHECK(readBuiltInStringProperty(button, copied) == std::optional<std::string>{"Changed"});
    CHECK(button.owner() == original_owner);
}

TEST_CASE("RadioButton scalar access delegates selection and text semantics") {
    RadioButton radio;
    const auto text = property("sasd.ui.RadioButton", "text");
    const auto selected = property("sasd.ui.RadioButton", "selected");
    CHECK(readBuiltInStringProperty(radio, text) == "");
    RadioButton option{"Option"};
    CHECK(readBuiltInStringProperty(option, text) == "Option");
    CHECK(writeBuiltInStringProperty(radio, text, "Updated"));
    CHECK(writeBuiltInStringProperty(radio, text, "Updated"));
    CHECK(radio.text() == "Updated");
    int selectedCalls = 0;
    radio.setOnSelected([&] { ++selectedCalls; });
    CHECK(readBuiltInBooleanProperty(radio, selected) == false);
    CHECK(writeBuiltInBooleanProperty(radio, selected, true));
    CHECK(radio.isSelected());
    CHECK(readBuiltInBooleanProperty(radio, selected) == true);
    CHECK(selectedCalls == 1);
    CHECK(writeBuiltInBooleanProperty(radio, selected, true));
    CHECK(!radio.setSelected(true));
    CHECK(selectedCalls == 1);
    CHECK(writeBuiltInBooleanProperty(radio, selected, false));
    CHECK(!radio.isSelected());
    CHECK(selectedCalls == 1);

    Button button;
    Label label;
    CheckBox checkBox;
    TextField field;
    Command command;
    for (Component* other : {static_cast<Component*>(&button), static_cast<Component*>(&label),
                            static_cast<Component*>(&checkBox), static_cast<Component*>(&field),
                            static_cast<Component*>(&command)}) {
        CHECK(!readBuiltInStringProperty(*other, text));
        CHECK(!writeBuiltInStringProperty(*other, text, "Rejected"));
        CHECK(!readBuiltInBooleanProperty(*other, selected));
        CHECK(!writeBuiltInBooleanProperty(*other, selected, true));
    }
    for (const auto type : {"sasd.ui.Button", "sasd.ui.Label", "sasd.ui.CheckBox",
                            "sasd.ui.TextField", "sasd.ui.Command"}) {
        const auto otherText = property(type, "text");
        CHECK(!readBuiltInStringProperty(radio, otherText));
        CHECK(!writeBuiltInStringProperty(radio, otherText, "Rejected"));
    }
    for (auto invalid : {
             ComponentPropertyMetadata{"sasd.ui.RadioButton", "text", ComponentPropertyValueKind::boolean},
             ComponentPropertyMetadata{"sasd.ui.Widget", "text", ComponentPropertyValueKind::string},
             ComponentPropertyMetadata{"sasd.ui.radiobutton", "text", ComponentPropertyValueKind::string},
             ComponentPropertyMetadata{"sasd.ui.RadioButton", "Text", ComponentPropertyValueKind::string}}) {
        CHECK(!readBuiltInStringProperty(radio, invalid));
        CHECK(!writeBuiltInStringProperty(radio, invalid, "Rejected"));
    }
    for (auto invalid : {
             ComponentPropertyMetadata{"sasd.ui.RadioButton", "selected", ComponentPropertyValueKind::string},
             ComponentPropertyMetadata{"sasd.ui.CheckBox", "selected", ComponentPropertyValueKind::boolean},
             ComponentPropertyMetadata{"sasd.ui.radiobutton", "selected", ComponentPropertyValueKind::boolean},
             ComponentPropertyMetadata{"sasd.ui.RadioButton", "Selected", ComponentPropertyValueKind::boolean}}) {
        CHECK(!readBuiltInBooleanProperty(radio, invalid));
        CHECK(!writeBuiltInBooleanProperty(radio, invalid, true));
    }
}

TEST_CASE("RadioButton property selection preserves group transaction and permits destruction") {
    const auto selected = property("sasd.ui.RadioButton", "selected");
    RadioGroup group;
    RadioButton first{group, "First"};
    RadioButton second{group, "Second"};
    second.setOnSelected([&] {
        CHECK(!first.isSelected());
        CHECK(second.isSelected());
        CHECK(group.selectedButton() == &second);
    });
    CHECK(writeBuiltInBooleanProperty(first, selected, true));
    CHECK(first.isSelected());
    CHECK(!second.isSelected());
    CHECK(group.selectedButton() == &first);
    CHECK(writeBuiltInBooleanProperty(second, selected, true));
    CHECK(!first.isSelected());
    CHECK(second.isSelected());
    CHECK(group.selectedButton() == &second);
    CHECK(writeBuiltInBooleanProperty(second, selected, false));
    CHECK(!second.isSelected());
    CHECK(group.selectedButton() == nullptr);

    auto owned = std::make_unique<RadioButton>(group, "Released");
    owned->setOnSelected([&] { owned.reset(); });
    CHECK(writeBuiltInBooleanProperty(*owned, selected, true));
    CHECK(!owned);
    CHECK(group.selectedButton() == nullptr);
}
