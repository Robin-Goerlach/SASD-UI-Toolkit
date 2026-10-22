#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/text_field.hpp>

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
