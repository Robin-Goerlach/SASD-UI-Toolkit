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
