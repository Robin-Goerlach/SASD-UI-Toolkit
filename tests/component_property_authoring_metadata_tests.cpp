#include "test_framework.hpp"

#include <sasd/ui/component_property_authoring_metadata.hpp>

#include <array>
#include <cstddef>
#include <string_view>

using namespace sasd::ui;

namespace {

struct ExpectedAuthoringCategory final {
    std::string_view declaring_type_id;
    std::string_view property_id;
    ComponentPropertyAuthoringCategory category;
};

constexpr std::array<ExpectedAuthoringCategory, 28> expected_categories{{
    {"sasd.ui.Widget", "visible", ComponentPropertyAuthoringCategory::behavior},
    {"sasd.ui.Widget", "enabled", ComponentPropertyAuthoringCategory::behavior},
    {"sasd.ui.Widget", "focusable", ComponentPropertyAuthoringCategory::behavior},
    {"sasd.ui.Widget", "sizeConstraints", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.Command", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.Command", "enabled", ComponentPropertyAuthoringCategory::behavior},
    {"sasd.ui.Label", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.Label", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.Button", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.Button", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.TextField", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.TextField", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.CheckBox", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.CheckBox", "checked", ComponentPropertyAuthoringCategory::state},
    {"sasd.ui.CheckBox", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.RadioButton", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.RadioButton", "selected", ComponentPropertyAuthoringCategory::state},
    {"sasd.ui.RadioButton", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.ComboBox", "items", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.ComboBox", "selectedIndex", ComponentPropertyAuthoringCategory::state},
    {"sasd.ui.ComboBox", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.HBox", "spacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.VBox", "spacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.GridLayout", "columnCount", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.GridLayout", "columnSpacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.GridLayout", "rowSpacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.FormLayout", "columnSpacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.FormLayout", "rowSpacing", ComponentPropertyAuthoringCategory::layout},
}};

const ExpectedAuthoringCategory* findExpected(const ComponentPropertyMetadata& property) {
    for (const auto& expected : expected_categories) {
        if (expected.declaring_type_id == property.declaring_type_id &&
            expected.property_id == property.property_id) {
            return &expected;
        }
    }
    return nullptr;
}

} // namespace

TEST_CASE("every built-in property has an explicit authoring category") {
    std::size_t descriptor_count = 0;
    std::size_t appearance_count = 0;
    std::size_t behavior_count = 0;
    std::size_t content_count = 0;
    std::size_t layout_count = 0;
    std::size_t state_count = 0;

    for (const auto& type : builtInComponentTypes()) {
        for (const auto& property : declaredBuiltInComponentProperties(type.type_id)) {
            ++descriptor_count;
            const auto* expected = findExpected(property);
            const auto authoring = builtInComponentPropertyAuthoringMetadata(property);
            CHECK(expected != nullptr);
            CHECK(authoring.has_value());
            CHECK(authoring->category == expected->category);

            switch (authoring->category) {
            case ComponentPropertyAuthoringCategory::appearance: ++appearance_count; break;
            case ComponentPropertyAuthoringCategory::behavior: ++behavior_count; break;
            case ComponentPropertyAuthoringCategory::content: ++content_count; break;
            case ComponentPropertyAuthoringCategory::layout: ++layout_count; break;
            case ComponentPropertyAuthoringCategory::state: ++state_count; break;
            }
        }
    }

    CHECK(descriptor_count == expected_categories.size());
    CHECK(appearance_count == 6);
    CHECK(behavior_count == 4);
    CHECK(content_count == 7);
    CHECK(layout_count == 8);
    CHECK(state_count == 3);
}

TEST_CASE("authoring metadata uses complete descriptor value identity and fails closed") {
    const auto* catalog = findBuiltInComponentProperty("sasd.ui.Button", "text");
    CHECK(catalog != nullptr);

    const ComponentPropertyMetadata copied = *catalog;
    CHECK(builtInComponentPropertyAuthoringMetadata(copied) ==
          ComponentPropertyAuthoringMetadata{ComponentPropertyAuthoringCategory::content});

    auto malformed = copied;
    malformed.value_kind = ComponentPropertyValueKind::boolean;
    CHECK(!builtInComponentPropertyAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.declaring_type_id = "sasd.ui.Widget";
    CHECK(!builtInComponentPropertyAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.property_id = "Text";
    CHECK(!builtInComponentPropertyAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.declaring_type_id = "SASD.UI.Button";
    CHECK(!builtInComponentPropertyAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.declaring_type_id = "sasd.ui.DoesNotExist";
    CHECK(!builtInComponentPropertyAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.property_id = "doesNotExist";
    CHECK(!builtInComponentPropertyAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.value_kind = static_cast<ComponentPropertyValueKind>(255);
    CHECK(!builtInComponentPropertyAuthoringMetadata(malformed).has_value());
}

TEST_CASE("inherited properties retain their declaring-property authoring category") {
    const auto* button_visible = findBuiltInComponentProperty("sasd.ui.Button", "visible");
    const auto* check_box_constraints =
        findBuiltInComponentProperty("sasd.ui.CheckBox", "sizeConstraints");
    CHECK(button_visible != nullptr);
    CHECK(button_visible->declaring_type_id == "sasd.ui.Widget");
    CHECK(builtInComponentPropertyAuthoringMetadata(*button_visible) ==
          ComponentPropertyAuthoringMetadata{ComponentPropertyAuthoringCategory::behavior});
    CHECK(check_box_constraints != nullptr);
    CHECK(check_box_constraints->declaring_type_id == "sasd.ui.Widget");
    CHECK(builtInComponentPropertyAuthoringMetadata(*check_box_constraints) ==
          ComponentPropertyAuthoringMetadata{ComponentPropertyAuthoringCategory::layout});

    const ComponentPropertyMetadata fabricated_derived{
        "sasd.ui.Button", "visible", ComponentPropertyValueKind::boolean};
    CHECK(!builtInComponentPropertyAuthoringMetadata(fabricated_derived).has_value());
}
