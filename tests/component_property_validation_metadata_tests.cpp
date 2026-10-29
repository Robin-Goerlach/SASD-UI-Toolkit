#include "test_framework.hpp"

#include <sasd/ui/component_metadata.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_property_validation_metadata.hpp>
#include <sasd/ui/geometry.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

using namespace sasd::ui;

namespace {

const ComponentPropertyMetadata& property(std::string_view type, std::string_view id) {
    const auto* result = findBuiltInComponentProperty(type, id);
    CHECK(result != nullptr);
    return *result;
}

struct PropertyIdentity final {
    std::string_view declaring_type_id;
    std::string_view property_id;
};

constexpr std::array signed_properties{
    PropertyIdentity{"sasd.ui.HBox", "spacing"},
    PropertyIdentity{"sasd.ui.VBox", "spacing"},
    PropertyIdentity{"sasd.ui.GridLayout", "columnSpacing"},
    PropertyIdentity{"sasd.ui.GridLayout", "rowSpacing"},
    PropertyIdentity{"sasd.ui.FormLayout", "columnSpacing"},
    PropertyIdentity{"sasd.ui.FormLayout", "rowSpacing"},
};

struct StringPolicyExpectation final {
    PropertyIdentity identity;
    StringPropertyNormalizationKind policy;
};

constexpr std::array string_properties{
    StringPolicyExpectation{{"sasd.ui.Command", "text"},
                            StringPropertyNormalizationKind::identity},
    StringPolicyExpectation{{"sasd.ui.Label", "text"},
                            StringPropertyNormalizationKind::identity},
    StringPolicyExpectation{{"sasd.ui.Button", "text"},
                            StringPropertyNormalizationKind::identity},
    StringPolicyExpectation{{"sasd.ui.TextField", "text"},
                            StringPropertyNormalizationKind::single_line_utf8},
    StringPolicyExpectation{{"sasd.ui.CheckBox", "text"},
                            StringPropertyNormalizationKind::identity},
    StringPolicyExpectation{{"sasd.ui.RadioButton", "text"},
                            StringPropertyNormalizationKind::identity},
};

} // namespace

TEST_CASE("all six signed layout ranges expose the Coordinate domain") {
    const auto maximum = static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max());
    for (const auto& identity : signed_properties) {
        const auto descriptor = property(identity.declaring_type_id, identity.property_id);
        const auto range = builtInSignedIntegerPropertyRange(descriptor);
        CHECK(range.has_value());
        CHECK(range->minimum == 0);
        CHECK(range->maximum == maximum);
        CHECK(range->contains(0));
        CHECK(range->contains(maximum));
        CHECK(!range->contains(-1));
        CHECK(maximum < std::numeric_limits<std::int64_t>::max());
        CHECK(!range->contains(maximum + 1));
        CHECK(!builtInUnsignedIntegerPropertyRange(descriptor));
    }
}

TEST_CASE("GridLayout columnCount exposes a host-independent uint64 domain") {
    const auto descriptor = property("sasd.ui.GridLayout", "columnCount");
    const auto range = builtInUnsignedIntegerPropertyRange(descriptor);
    CHECK(range.has_value());
    CHECK(range->minimum == 1);
    CHECK(range->maximum == std::numeric_limits<std::uint64_t>::max());
    CHECK(range->contains(1));
    CHECK(range->contains(std::numeric_limits<std::uint64_t>::max()));
    CHECK(!range->contains(0));
    CHECK(!builtInSignedIntegerPropertyRange(descriptor));
}

TEST_CASE("integer range lookup validates complete descriptor identity by value") {
    const auto horizontal = property("sasd.ui.HBox", "spacing");
    const auto vertical = property("sasd.ui.VBox", "spacing");
    CHECK(horizontal.declaring_type_id != vertical.declaring_type_id);
    CHECK(builtInSignedIntegerPropertyRange(horizontal) ==
          builtInSignedIntegerPropertyRange(vertical));

    const ComponentPropertyMetadata copied = horizontal;
    CHECK(builtInSignedIntegerPropertyRange(copied).has_value());

    for (const ComponentPropertyMetadata malformed : {
             ComponentPropertyMetadata{"sasd.ui.Container", "spacing",
                                       ComponentPropertyValueKind::signed_integer},
             ComponentPropertyMetadata{"sasd.ui.HBox", "columnSpacing",
                                       ComponentPropertyValueKind::signed_integer},
             ComponentPropertyMetadata{"sasd.ui.HBox", "spacing",
                                       ComponentPropertyValueKind::unsigned_integer},
             ComponentPropertyMetadata{"sasd.ui.HBox", "Spacing",
                                       ComponentPropertyValueKind::signed_integer},
             ComponentPropertyMetadata{"SASD.UI.HBox", "spacing",
                                       ComponentPropertyValueKind::signed_integer},
             ComponentPropertyMetadata{"sasd.ui.HBox", "spacing",
                                       static_cast<ComponentPropertyValueKind>(99)}}) {
        CHECK(!builtInSignedIntegerPropertyRange(malformed));
        CHECK(!builtInUnsignedIntegerPropertyRange(malformed));
    }
}

TEST_CASE("integer validation metadata completely covers the current numeric catalog") {
    std::size_t signed_count = 0;
    std::size_t unsigned_count = 0;

    for (const auto& type : builtInComponentTypes()) {
        for (const auto& descriptor : declaredBuiltInComponentProperties(type.type_id)) {
            const auto signed_range = builtInSignedIntegerPropertyRange(descriptor);
            const auto unsigned_range = builtInUnsignedIntegerPropertyRange(descriptor);
            if (descriptor.value_kind == ComponentPropertyValueKind::signed_integer) {
                ++signed_count;
                CHECK(signed_range.has_value());
                CHECK(!unsigned_range.has_value());
            } else if (descriptor.value_kind == ComponentPropertyValueKind::unsigned_integer) {
                ++unsigned_count;
                CHECK(!signed_range.has_value());
                CHECK(unsigned_range.has_value());
            } else {
                CHECK(!signed_range.has_value());
                CHECK(!unsigned_range.has_value());
            }
        }
    }

    CHECK(signed_count == 6);
    CHECK(unsigned_count == 1);
}

TEST_CASE("all current string descriptors expose explicit normalization policies") {
    for (const auto& expectation : string_properties) {
        const auto descriptor = property(expectation.identity.declaring_type_id,
                                         expectation.identity.property_id);
        CHECK(builtInStringPropertyNormalization(descriptor) == expectation.policy);
        CHECK(!builtInSizeConstraintsPropertyValidation(descriptor));
    }

    const ComponentPropertyMetadata copied = property("sasd.ui.TextField", "text");
    CHECK(builtInStringPropertyNormalization(copied) ==
          StringPropertyNormalizationKind::single_line_utf8);

    for (const ComponentPropertyMetadata malformed : {
             ComponentPropertyMetadata{"sasd.ui.Label", "text",
                                       ComponentPropertyValueKind::boolean},
             ComponentPropertyMetadata{"sasd.ui.Widget", "text",
                                       ComponentPropertyValueKind::string},
             ComponentPropertyMetadata{"sasd.ui.Label", "Text",
                                       ComponentPropertyValueKind::string},
             ComponentPropertyMetadata{"SASD.UI.Label", "text",
                                       ComponentPropertyValueKind::string},
             ComponentPropertyMetadata{"sasd.ui.Label", "text",
                                       static_cast<ComponentPropertyValueKind>(99)}}) {
        CHECK(!builtInStringPropertyNormalization(malformed));
    }
    CHECK(!builtInStringPropertyNormalization(property("sasd.ui.Widget", "visible")));
}

TEST_CASE("Widget sizeConstraints exposes its exact compound validation policy") {
    const auto descriptor = property("sasd.ui.Widget", "sizeConstraints");
    CHECK(builtInSizeConstraintsPropertyValidation(descriptor) ==
          SizeConstraintsPropertyValidationKind::minimum_not_greater_than_maximum);

    const ComponentPropertyMetadata copied = descriptor;
    CHECK(builtInSizeConstraintsPropertyValidation(copied).has_value());
    for (const ComponentPropertyMetadata malformed : {
             ComponentPropertyMetadata{"sasd.ui.Widget", "sizeConstraints",
                                       ComponentPropertyValueKind::boolean},
             ComponentPropertyMetadata{"sasd.ui.Container", "sizeConstraints",
                                       ComponentPropertyValueKind::size_constraints},
             ComponentPropertyMetadata{"sasd.ui.Widget", "SizeConstraints",
                                       ComponentPropertyValueKind::size_constraints},
             ComponentPropertyMetadata{"SASD.UI.Widget", "sizeConstraints",
                                       ComponentPropertyValueKind::size_constraints},
             ComponentPropertyMetadata{"sasd.ui.Widget", "sizeConstraints",
                                       static_cast<ComponentPropertyValueKind>(99)}}) {
        CHECK(!builtInSizeConstraintsPropertyValidation(malformed));
    }
    CHECK(!builtInSizeConstraintsPropertyValidation(property("sasd.ui.Label", "text")));
}

TEST_CASE("ComboBox selectedIndex exposes its exact string collection dependency") {
    const auto descriptor = property("sasd.ui.ComboBox", "selectedIndex");
    const auto validation =
        builtInOptionalUnsignedIntegerCrossPropertyValidation(descriptor);
    CHECK(validation.has_value());
    CHECK(validation->kind == OptionalUnsignedIntegerCrossPropertyValidationKind::
                                  index_within_string_collection);
    CHECK(validation->dependency_property.declaring_type_id == "sasd.ui.ComboBox");
    CHECK(validation->dependency_property.property_id == "items");
    CHECK(validation->dependency_property.value_kind ==
          ComponentPropertyValueKind::string_collection);

    const auto copied = descriptor;
    CHECK(builtInOptionalUnsignedIntegerCrossPropertyValidation(copied) == validation);

    for (const ComponentPropertyMetadata malformed : {
             ComponentPropertyMetadata{"sasd.ui.ComboBox", "selectedIndex",
                                       ComponentPropertyValueKind::unsigned_integer},
             ComponentPropertyMetadata{"sasd.ui.Widget", "selectedIndex",
                                       ComponentPropertyValueKind::optional_unsigned_integer},
             ComponentPropertyMetadata{"sasd.ui.ComboBox", "items",
                                       ComponentPropertyValueKind::optional_unsigned_integer},
             ComponentPropertyMetadata{"sasd.ui.ComboBox", "SelectedIndex",
                                       ComponentPropertyValueKind::optional_unsigned_integer},
             ComponentPropertyMetadata{"SASD.UI.ComboBox", "selectedIndex",
                                       ComponentPropertyValueKind::optional_unsigned_integer},
             ComponentPropertyMetadata{"sasd.ui.ComboBox", "selectedIndex",
                                       static_cast<ComponentPropertyValueKind>(99)}}) {
        CHECK(!builtInOptionalUnsignedIntegerCrossPropertyValidation(malformed));
    }
    CHECK(!builtInOptionalUnsignedIntegerCrossPropertyValidation(
        property("sasd.ui.ComboBox", "items")));
    CHECK(!builtInOptionalUnsignedIntegerCrossPropertyValidation(
        property("sasd.ui.GridLayout", "columnCount")));
}

TEST_CASE("cross property validation metadata covers the optional unsigned catalog") {
    std::size_t optional_unsigned_count = 0;

    for (const auto& type : builtInComponentTypes()) {
        for (const auto& descriptor : declaredBuiltInComponentProperties(type.type_id)) {
            const auto validation =
                builtInOptionalUnsignedIntegerCrossPropertyValidation(descriptor);
            if (descriptor.value_kind ==
                ComponentPropertyValueKind::optional_unsigned_integer) {
                ++optional_unsigned_count;
                CHECK(validation.has_value());
                CHECK(validation->dependency_property.value_kind ==
                      ComponentPropertyValueKind::string_collection);
            } else {
                CHECK(!validation.has_value());
            }
        }
    }

    CHECK(optional_unsigned_count == 1);
}

TEST_CASE("string and size constraints policies completely cover their current catalogs") {
    std::size_t string_count = 0;
    std::size_t identity_count = 0;
    std::size_t single_line_count = 0;
    std::size_t size_constraints_count = 0;

    for (const auto& type : builtInComponentTypes()) {
        for (const auto& descriptor : declaredBuiltInComponentProperties(type.type_id)) {
            const auto string_policy = builtInStringPropertyNormalization(descriptor);
            const auto size_policy = builtInSizeConstraintsPropertyValidation(descriptor);
            if (descriptor.value_kind == ComponentPropertyValueKind::string) {
                ++string_count;
                CHECK(string_policy.has_value());
                CHECK(!size_policy.has_value());
                if (string_policy == StringPropertyNormalizationKind::identity) {
                    ++identity_count;
                } else if (string_policy == StringPropertyNormalizationKind::single_line_utf8) {
                    ++single_line_count;
                }
            } else if (descriptor.value_kind == ComponentPropertyValueKind::size_constraints) {
                ++size_constraints_count;
                CHECK(!string_policy.has_value());
                CHECK(size_policy == SizeConstraintsPropertyValidationKind::
                                         minimum_not_greater_than_maximum);
            } else {
                CHECK(!string_policy.has_value());
                CHECK(!size_policy.has_value());
            }
        }
    }

    CHECK(string_count == string_properties.size());
    CHECK(identity_count == 5);
    CHECK(single_line_count == 1);
    CHECK(size_constraints_count == 1);
}
