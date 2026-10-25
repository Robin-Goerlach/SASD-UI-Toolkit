#include "test_framework.hpp"

#include <sasd/ui/component_property_metadata.hpp>

#include <array>
#include <cstddef>
#include <string_view>

using namespace sasd::ui;

namespace {

struct ExpectedProperty final {
    std::string_view declaring_type_id;
    std::string_view property_id;
    ComponentPropertyValueKind value_kind;
};

constexpr std::array<ExpectedProperty, 21> expected_properties{{
    {"sasd.ui.Widget", "visible", ComponentPropertyValueKind::boolean},
    {"sasd.ui.Widget", "enabled", ComponentPropertyValueKind::boolean},
    {"sasd.ui.Widget", "focusable", ComponentPropertyValueKind::boolean},
    {"sasd.ui.Command", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.Command", "enabled", ComponentPropertyValueKind::boolean},
    {"sasd.ui.Label", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.Button", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.TextField", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.CheckBox", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.CheckBox", "checked", ComponentPropertyValueKind::boolean},
    {"sasd.ui.RadioButton", "text", ComponentPropertyValueKind::string},
    {"sasd.ui.RadioButton", "selected", ComponentPropertyValueKind::boolean},
    {"sasd.ui.ComboBox", "items", ComponentPropertyValueKind::string_collection},
    {"sasd.ui.ComboBox", "selectedIndex", ComponentPropertyValueKind::optional_unsigned_integer},
    {"sasd.ui.HBox", "spacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.VBox", "spacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.GridLayout", "columnCount", ComponentPropertyValueKind::unsigned_integer},
    {"sasd.ui.GridLayout", "columnSpacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.GridLayout", "rowSpacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.FormLayout", "columnSpacing", ComponentPropertyValueKind::signed_integer},
    {"sasd.ui.FormLayout", "rowSpacing", ComponentPropertyValueKind::signed_integer},
}};

const ComponentPropertyMetadata* findDirect(std::string_view type_id,
                                            std::string_view property_id) {
    for (const auto& property : declaredBuiltInComponentProperties(type_id)) {
        if (property.property_id == property_id) {
            return &property;
        }
    }
    return nullptr;
}

} // namespace

TEST_CASE("built-in component property schema is explicit and complete") {
    std::size_t descriptor_count = 0;

    for (const auto& type : builtInComponentTypes()) {
        const auto properties = declaredBuiltInComponentProperties(type.type_id);
        descriptor_count += properties.size();

        for (std::size_t index = 0; index < properties.size(); ++index) {
            const auto& property = properties[index];
            CHECK(!property.declaring_type_id.empty());
            CHECK(!property.property_id.empty());
            CHECK(property.declaring_type_id == type.type_id);
            CHECK(findBuiltInComponentType(property.declaring_type_id) != nullptr);
            for (std::size_t other = index + 1; other < properties.size(); ++other) {
                CHECK(property.property_id != properties[other].property_id);
            }
        }
    }

    CHECK(descriptor_count == expected_properties.size());
    for (const auto& expected : expected_properties) {
        const auto* property = findDirect(expected.declaring_type_id, expected.property_id);
        CHECK(property != nullptr);
        CHECK(property->declaring_type_id == expected.declaring_type_id);
        CHECK(property->property_id == expected.property_id);
        CHECK(property->value_kind == expected.value_kind);
    }
}

TEST_CASE("direct property queries return only properties declared by that type") {
    CHECK(declaredBuiltInComponentProperties("sasd.ui.Widget").size() == 3);
    CHECK(declaredBuiltInComponentProperties("sasd.ui.Command").size() == 2);
    CHECK(declaredBuiltInComponentProperties("sasd.ui.Label").size() == 1);
    CHECK(declaredBuiltInComponentProperties("sasd.ui.Button").size() == 1);
    CHECK(declaredBuiltInComponentProperties("sasd.ui.TextField").size() == 1);
    CHECK(declaredBuiltInComponentProperties("sasd.ui.CheckBox").size() == 2);

    CHECK(declaredBuiltInComponentProperties("sasd.ui.Window").empty());
    CHECK(declaredBuiltInComponentProperties("sasd.ui.DoesNotExist").empty());
}

TEST_CASE("property lookup follows component type inheritance without duplication") {
    const auto* button_visible =
        findBuiltInComponentProperty("sasd.ui.Button", "visible");
    CHECK(button_visible != nullptr);
    CHECK(button_visible->declaring_type_id == "sasd.ui.Widget");
    CHECK(button_visible->property_id == "visible");

    const auto* label_enabled = findBuiltInComponentProperty("sasd.ui.Label", "enabled");
    CHECK(label_enabled != nullptr);
    CHECK(label_enabled->declaring_type_id == "sasd.ui.Widget");

    const auto* text_field_focusable =
        findBuiltInComponentProperty("sasd.ui.TextField", "focusable");
    CHECK(text_field_focusable != nullptr);
    CHECK(text_field_focusable->declaring_type_id == "sasd.ui.Widget");

    const auto* check_box_text = findBuiltInComponentProperty("sasd.ui.CheckBox", "text");
    CHECK(check_box_text != nullptr);
    CHECK(check_box_text->declaring_type_id == "sasd.ui.CheckBox");

    const auto* check_box_checked =
        findBuiltInComponentProperty("sasd.ui.CheckBox", "checked");
    CHECK(check_box_checked != nullptr);
    CHECK(check_box_checked->value_kind == ComponentPropertyValueKind::boolean);

    const auto* window_visible = findBuiltInComponentProperty("sasd.ui.Window", "visible");
    CHECK(window_visible != nullptr);
    CHECK(window_visible->declaring_type_id == "sasd.ui.Widget");

    const auto* command_enabled = findBuiltInComponentProperty("sasd.ui.Command", "enabled");
    CHECK(command_enabled != nullptr);
    CHECK(command_enabled->declaring_type_id == "sasd.ui.Command");
}

TEST_CASE("built-in property inheritance has no shadowing along ancestor chains") {
    for (const auto& type : builtInComponentTypes()) {
        for (const auto& property : declaredBuiltInComponentProperties(type.type_id)) {
            auto base_type_id = type.base_type_id;
            for (std::size_t depth = 0; depth < builtInComponentTypes().size(); ++depth) {
                CHECK(findDirect(base_type_id, property.property_id) == nullptr);
                const auto* base = findBuiltInComponentType(base_type_id);
                if (base == nullptr || base->base_type_id.empty()) {
                    break;
                }
                base_type_id = base->base_type_id;
            }
        }
    }
}

TEST_CASE("property metadata lookup is exact and does not access instances") {
    CHECK(findBuiltInComponentProperty("sasd.ui.Button", "TEXT") == nullptr);
    CHECK(findBuiltInComponentProperty("SASD.UI.Button", "text") == nullptr);
    CHECK(findBuiltInComponentProperty("sasd.ui.ButtonExtra", "text") == nullptr);
    CHECK(findBuiltInComponentProperty("sasd.ui.Button", "doesNotExist") == nullptr);
    CHECK(findBuiltInComponentProperty("sasd.ui.Window", "checked") == nullptr);
    CHECK(findBuiltInComponentProperty("", "visible") == nullptr);
}

TEST_CASE("RadioButton declares ordered scalars and inherits Widget properties") {
    const auto direct = declaredBuiltInComponentProperties("sasd.ui.RadioButton");
    CHECK(direct.size() == 2);
    CHECK(direct[0].property_id == "text");
    CHECK(direct[0].value_kind == ComponentPropertyValueKind::string);
    CHECK(direct[1].property_id == "selected");
    CHECK(direct[1].value_kind == ComponentPropertyValueKind::boolean);
    CHECK(findBuiltInComponentProperty("sasd.ui.RadioButton", "text") == &direct[0]);
    CHECK(findBuiltInComponentProperty("sasd.ui.RadioButton", "selected") == &direct[1]);
    for (const auto name : {"visible", "enabled", "focusable"}) {
        const auto* inherited = findBuiltInComponentProperty("sasd.ui.RadioButton", name);
        CHECK(inherited != nullptr);
        CHECK(inherited->declaring_type_id == "sasd.ui.Widget");
    }
}
