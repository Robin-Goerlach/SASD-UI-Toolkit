#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/component_property_snapshot.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/widget.hpp>

#include <optional>
#include <string>

using namespace sasd::ui;

namespace {

const ComponentBooleanPropertySnapshot& booleanProperty(
    const BuiltInComponentPropertySnapshot& snapshot,
    std::string_view declaring_type_id,
    std::string_view property_id) {
    for (const auto& property : snapshot.boolean_properties) {
        if (property.declaring_type_id == declaring_type_id &&
            property.property_id == property_id) {
            return property;
        }
    }
    CHECK(false);
    throw std::logic_error{"unreachable"};
}

const ComponentStringPropertySnapshot& stringProperty(
    const BuiltInComponentPropertySnapshot& snapshot,
    std::string_view declaring_type_id,
    std::string_view property_id) {
    for (const auto& property : snapshot.string_properties) {
        if (property.declaring_type_id == declaring_type_id &&
            property.property_id == property_id) {
            return property;
        }
    }
    CHECK(false);
    throw std::logic_error{"unreachable"};
}

class UnknownWidget final : public Widget {};
class UnknownComponent final : public Component {};

} // namespace

TEST_CASE("Button snapshot captures inherited and direct properties with defaults") {
    Button button;
    const auto snapshot = captureBuiltInComponentPropertySnapshot(button);

    CHECK(snapshot.has_value());
    CHECK(snapshot->type_id == "sasd.ui.Button");
    CHECK(snapshot->boolean_properties.size() == 3);
    CHECK(snapshot->string_properties.size() == 1);

    CHECK(booleanProperty(*snapshot, "sasd.ui.Widget", "visible").value);
    CHECK(booleanProperty(*snapshot, "sasd.ui.Widget", "visible").is_default);
    CHECK(booleanProperty(*snapshot, "sasd.ui.Widget", "enabled").value);
    CHECK(booleanProperty(*snapshot, "sasd.ui.Widget", "enabled").is_default);
    CHECK(booleanProperty(*snapshot, "sasd.ui.Widget", "focusable").value);
    CHECK(booleanProperty(*snapshot, "sasd.ui.Widget", "focusable").is_default);
    CHECK(stringProperty(*snapshot, "sasd.ui.Button", "text").value.empty());
    CHECK(stringProperty(*snapshot, "sasd.ui.Button", "text").is_default);

    CHECK(snapshot->boolean_properties[0].property_id == "visible");
    CHECK(snapshot->boolean_properties[1].property_id == "enabled");
    CHECK(snapshot->boolean_properties[2].property_id == "focusable");
}

TEST_CASE("modified Button snapshot preserves current values and default comparison") {
    Button button{"Save"};
    button.setVisible(false);

    const auto snapshot = captureBuiltInComponentPropertySnapshot(button);
    CHECK(snapshot.has_value());

    CHECK(!booleanProperty(*snapshot, "sasd.ui.Widget", "visible").value);
    CHECK(!booleanProperty(*snapshot, "sasd.ui.Widget", "visible").is_default);
    CHECK(booleanProperty(*snapshot, "sasd.ui.Widget", "enabled").is_default);
    CHECK(booleanProperty(*snapshot, "sasd.ui.Widget", "focusable").is_default);
    CHECK(stringProperty(*snapshot, "sasd.ui.Button", "text").value == "Save");
    CHECK(!stringProperty(*snapshot, "sasd.ui.Button", "text").is_default);
}

TEST_CASE("Label and CheckBox snapshots use exact target-type defaults") {
    Label label{"Name"};
    CheckBox check_box{"Option", true};

    const auto label_snapshot = captureBuiltInComponentPropertySnapshot(label);
    const auto check_box_snapshot = captureBuiltInComponentPropertySnapshot(check_box);
    CHECK(label_snapshot.has_value());
    CHECK(check_box_snapshot.has_value());

    CHECK(!booleanProperty(*label_snapshot, "sasd.ui.Widget", "focusable").value);
    CHECK(booleanProperty(*label_snapshot, "sasd.ui.Widget", "focusable").is_default);
    CHECK(stringProperty(*label_snapshot, "sasd.ui.Label", "text").value == "Name");
    CHECK(!stringProperty(*label_snapshot, "sasd.ui.Label", "text").is_default);

    CHECK(booleanProperty(*check_box_snapshot, "sasd.ui.CheckBox", "checked").value);
    CHECK(!booleanProperty(*check_box_snapshot, "sasd.ui.CheckBox", "checked").is_default);
    CHECK(check_box_snapshot->boolean_properties[3].property_id == "checked");
    CHECK(stringProperty(*check_box_snapshot, "sasd.ui.CheckBox", "text").value == "Option");
}

TEST_CASE("Command and TextField snapshots observe semantic runtime access") {
    Command command{"Save"};
    command.setEnabled(false);
    TextField field;
    field.setText(std::string{"A\nB\xC3("});

    const auto command_snapshot = captureBuiltInComponentPropertySnapshot(command);
    const auto field_snapshot = captureBuiltInComponentPropertySnapshot(field);
    CHECK(command_snapshot.has_value());
    CHECK(field_snapshot.has_value());

    CHECK(command_snapshot->type_id == "sasd.ui.Command");
    CHECK(stringProperty(*command_snapshot, "sasd.ui.Command", "text").value == "Save");
    CHECK(!stringProperty(*command_snapshot, "sasd.ui.Command", "text").is_default);
    CHECK(!booleanProperty(*command_snapshot, "sasd.ui.Command", "enabled").value);
    CHECK(!booleanProperty(*command_snapshot, "sasd.ui.Command", "enabled").is_default);
    CHECK(stringProperty(*field_snapshot, "sasd.ui.TextField", "text").value ==
          std::string{"AB\xEF\xBF\xBD("});
}

TEST_CASE("known built-in Component without catalogued properties has an empty snapshot") {
    Component component;
    const auto snapshot = captureBuiltInComponentPropertySnapshot(component);

    CHECK(snapshot.has_value());
    CHECK(snapshot->type_id == "sasd.ui.Component");
    CHECK(snapshot->boolean_properties.empty());
    CHECK(snapshot->string_properties.empty());
}

TEST_CASE("unknown derived components fail closed without base fallback") {
    UnknownWidget widget;
    UnknownComponent component;

    CHECK(!captureBuiltInComponentPropertySnapshot(widget).has_value());
    CHECK(!captureBuiltInComponentPropertySnapshot(component).has_value());
}

TEST_CASE("snapshot owns its strings after the source Component is destroyed") {
    std::optional<BuiltInComponentPropertySnapshot> snapshot;
    {
        Button button{"Owned"};
        snapshot = captureBuiltInComponentPropertySnapshot(button);
    }

    CHECK(snapshot.has_value());
    CHECK(snapshot->type_id == "sasd.ui.Button");
    CHECK(stringProperty(*snapshot, "sasd.ui.Button", "text").value == "Owned");
    CHECK(stringProperty(*snapshot, "sasd.ui.Button", "text").declaring_type_id ==
          "sasd.ui.Button");
}

TEST_CASE("snapshot does not mutate Component state or ownership") {
    Button button{"Stable"};
    const auto* owner_before = button.owner();
    const auto* parent_before = button.parent();
    const bool visible_before = button.isVisible();
    const bool enabled_before = button.isEnabled();
    const bool focusable_before = button.isFocusable();

    CHECK(captureBuiltInComponentPropertySnapshot(button).has_value());
    CHECK(button.owner() == owner_before);
    CHECK(button.parent() == parent_before);
    CHECK(button.isVisible() == visible_before);
    CHECK(button.isEnabled() == enabled_before);
    CHECK(button.isFocusable() == focusable_before);
}
