#include "test_framework.hpp"

#include <sasd/ui/component_property_resource_metadata.hpp>

#include <array>
#include <cstddef>
#include <string_view>

using namespace sasd::ui;

namespace {

struct ExpectedResourceProperty final {
    std::string_view declaring_type_id;
    std::string_view property_id;
};

constexpr std::array<ExpectedResourceProperty, 6> expected_resource_properties{{
    {"sasd.ui.Label", "textStyle"},
    {"sasd.ui.Button", "textStyle"},
    {"sasd.ui.TextField", "textStyle"},
    {"sasd.ui.CheckBox", "textStyle"},
    {"sasd.ui.RadioButton", "textStyle"},
    {"sasd.ui.ComboBox", "textStyle"},
}};

bool isExpectedResourceProperty(const ComponentPropertyMetadata& property) {
    for (const auto& expected : expected_resource_properties) {
        if (expected.declaring_type_id == property.declaring_type_id &&
            expected.property_id == property.property_id) {
            return true;
        }
    }
    return false;
}

const ComponentPropertyMetadata& property(std::string_view type_id, std::string_view property_id) {
    const auto* result = findBuiltInComponentProperty(type_id, property_id);
    CHECK(result != nullptr);
    return *result;
}

} // namespace

TEST_CASE("exactly six built-in textStyle descriptors support TextStyle resource authoring") {
    std::size_t descriptor_count = 0;
    std::size_t resource_capable_count = 0;
    std::size_t non_resource_capable_count = 0;

    for (const auto& type : builtInComponentTypes()) {
        for (const auto& descriptor : declaredBuiltInComponentProperties(type.type_id)) {
            ++descriptor_count;
            const auto metadata = builtInComponentPropertyResourceMetadata(descriptor);
            if (isExpectedResourceProperty(descriptor)) {
                ++resource_capable_count;
                CHECK(metadata ==
                      ComponentPropertyResourceMetadata{ResourceKind::text_style});
            } else {
                ++non_resource_capable_count;
                CHECK(!metadata.has_value());
            }
        }
    }

    CHECK(descriptor_count == 28);
    CHECK(resource_capable_count == expected_resource_properties.size());
    CHECK(non_resource_capable_count == 22);
}

TEST_CASE("resource metadata uses complete descriptor value identity and fails closed") {
    const ComponentPropertyMetadata copied = property("sasd.ui.Label", "textStyle");
    CHECK(builtInComponentPropertyResourceMetadata(copied) ==
          ComponentPropertyResourceMetadata{ResourceKind::text_style});

    auto malformed = copied;
    malformed.value_kind = ComponentPropertyValueKind::string;
    CHECK(!builtInComponentPropertyResourceMetadata(malformed));
    malformed = copied;
    malformed.declaring_type_id = "sasd.ui.Button";
    // These three values now form the independently valid Button.textStyle descriptor. This proves
    // that the policy compares descriptor values rather than requiring the Label catalog address.
    CHECK(builtInComponentPropertyResourceMetadata(malformed));
    malformed = copied;
    malformed.declaring_type_id = "sasd.ui.Widget";
    CHECK(!builtInComponentPropertyResourceMetadata(malformed));
    malformed = copied;
    malformed.property_id = "text";
    CHECK(!builtInComponentPropertyResourceMetadata(malformed));
    malformed = copied;
    malformed.property_id = "TextStyle";
    CHECK(!builtInComponentPropertyResourceMetadata(malformed));
    malformed = copied;
    malformed.property_id = "doesNotExist";
    CHECK(!builtInComponentPropertyResourceMetadata(malformed));
    malformed = copied;
    malformed.declaring_type_id = "sasd.ui.DoesNotExist";
    CHECK(!builtInComponentPropertyResourceMetadata(malformed));

    const ComponentPropertyMetadata fabricated{
        "sasd.ui.Fabricated", "textStyle", ComponentPropertyValueKind::text_style};
    CHECK(!builtInComponentPropertyResourceMetadata(fabricated));
    malformed = copied;
    malformed.value_kind = static_cast<ComponentPropertyValueKind>(255);
    CHECK(!builtInComponentPropertyResourceMetadata(malformed));
}

TEST_CASE("representative non-resource built-in properties remain inline or ordinary values") {
    CHECK(!builtInComponentPropertyResourceMetadata(property("sasd.ui.Label", "text")));
    CHECK(!builtInComponentPropertyResourceMetadata(
        property("sasd.ui.Widget", "sizeConstraints")));
    CHECK(!builtInComponentPropertyResourceMetadata(property("sasd.ui.ComboBox", "items")));
    CHECK(!builtInComponentPropertyResourceMetadata(
        property("sasd.ui.ComboBox", "selectedIndex")));
    CHECK(!builtInComponentPropertyResourceMetadata(property("sasd.ui.CheckBox", "checked")));
}
