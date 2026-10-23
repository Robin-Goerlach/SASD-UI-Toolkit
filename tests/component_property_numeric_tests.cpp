#include "test_framework.hpp"

#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_snapshot.hpp>
#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/vbox.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace sasd::ui;

namespace {

const ComponentPropertyMetadata& property(std::string_view type, std::string_view id) {
    const auto* result = findBuiltInComponentProperty(type, id);
    CHECK(result != nullptr);
    return *result;
}

template <typename Callback>
void rejectsDomainValue(Callback callback) {
    bool threw = false;
    try {
        callback();
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

struct SignedCase {
    Component& component;
    std::string_view type;
    std::string_view id;
    std::int64_t initial;
};

} // namespace

TEST_CASE("all six signed layout properties bridge semantic access and construction defaults") {
    HBox hbox;
    VBox vbox;
    GridLayout grid;
    FormLayout form;
    const std::array<SignedCase, 6> cases{{
        {hbox, "sasd.ui.HBox", "spacing", 0},
        {vbox, "sasd.ui.VBox", "spacing", 0},
        {grid, "sasd.ui.GridLayout", "columnSpacing", 0},
        {grid, "sasd.ui.GridLayout", "rowSpacing", 0},
        {form, "sasd.ui.FormLayout", "columnSpacing", 1},
        {form, "sasd.ui.FormLayout", "rowSpacing", 0},
    }};
    Component incompatible;
    for (const auto& entry : cases) {
        // Copying a descriptor must preserve access; catalog addresses are not identity.
        const auto descriptor = property(entry.type, entry.id);
        CHECK(descriptor.declaring_type_id == entry.type);
        CHECK(descriptor.value_kind == ComponentPropertyValueKind::signed_integer);
        CHECK(readBuiltInSignedIntegerProperty(entry.component, descriptor) == entry.initial);
        CHECK(builtInSignedIntegerPropertyDefault(entry.type, descriptor) == entry.initial);
        CHECK(writeBuiltInSignedIntegerProperty(entry.component, descriptor, 7));
        CHECK(writeBuiltInSignedIntegerProperty(entry.component, descriptor, 7));
        CHECK(readBuiltInSignedIntegerProperty(entry.component, descriptor) == 7);
        CHECK(!readBuiltInSignedIntegerProperty(incompatible, descriptor));
        CHECK(!writeBuiltInSignedIntegerProperty(incompatible, descriptor, 8));
        CHECK(!readBuiltInUnsignedIntegerProperty(entry.component, descriptor));
        CHECK(!writeBuiltInUnsignedIntegerProperty(entry.component, descriptor, 8));
        CHECK(!readBuiltInBooleanProperty(entry.component, descriptor));
        CHECK(!writeBuiltInBooleanProperty(entry.component, descriptor, true));
        CHECK(!readBuiltInStringProperty(entry.component, descriptor));
        CHECK(!writeBuiltInStringProperty(entry.component, descriptor, "8"));
        CHECK(!builtInUnsignedIntegerPropertyDefault(entry.type, descriptor));
        CHECK(!builtInSignedIntegerPropertyDefault("sasd.ui.Unknown", descriptor));

        for (const ComponentPropertyMetadata malformed : {
                 ComponentPropertyMetadata{entry.type, "unknown", descriptor.value_kind},
                 ComponentPropertyMetadata{"sasd.ui.Container", entry.id, descriptor.value_kind},
                 ComponentPropertyMetadata{entry.type, entry.id, ComponentPropertyValueKind::string},
                 ComponentPropertyMetadata{entry.type, entry.id, static_cast<ComponentPropertyValueKind>(99)}}) {
            CHECK(!readBuiltInSignedIntegerProperty(entry.component, malformed));
            CHECK(!writeBuiltInSignedIntegerProperty(entry.component, malformed, 8));
            CHECK(!builtInSignedIntegerPropertyDefault(entry.type, malformed));
        }
        // Both out-of-range directions must fail before reaching the semantic setter.
        const auto maximum = static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max());
        const auto minimum = static_cast<std::int64_t>(std::numeric_limits<Coordinate>::min());
        for (const auto outside : {minimum - 1, maximum + 1,
                                   std::numeric_limits<std::int64_t>::min(),
                                   std::numeric_limits<std::int64_t>::max()}) {
            CHECK(!writeBuiltInSignedIntegerProperty(entry.component, descriptor, outside));
            CHECK(readBuiltInSignedIntegerProperty(entry.component, descriptor) == 7);
        }
        rejectsDomainValue([&] {
            (void)writeBuiltInSignedIntegerProperty(entry.component, descriptor, -1);
        });
        rejectsDomainValue([&] {
            (void)writeBuiltInSignedIntegerProperty(entry.component, descriptor, minimum);
        });
        CHECK(readBuiltInSignedIntegerProperty(entry.component, descriptor) == 7);
        CHECK(writeBuiltInSignedIntegerProperty(entry.component, descriptor, maximum));
        CHECK(readBuiltInSignedIntegerProperty(entry.component, descriptor) == maximum);
        CHECK(writeBuiltInSignedIntegerProperty(entry.component, descriptor, 0));
    }
    // Independently check concrete getters, rather than only round-tripping through the bridge.
    CHECK(hbox.spacing() == 0);
    CHECK(vbox.spacing() == 0);
    CHECK(grid.columnSpacing() == 0);
    CHECK(grid.rowSpacing() == 0);
    CHECK(form.columnSpacing() == 0);
    CHECK(form.rowSpacing() == 0);
    hbox.setSpacing(9);
    CHECK(readBuiltInSignedIntegerProperty(hbox, property("sasd.ui.HBox", "spacing")) == 9);
}

TEST_CASE("numeric descriptors preserve sibling identity for access and defaults") {
    HBox hbox;
    VBox vbox;
    GridLayout grid;
    FormLayout form;
    const auto& horizontal = property("sasd.ui.HBox", "spacing");
    const auto& vertical = property("sasd.ui.VBox", "spacing");
    CHECK(&horizontal != &vertical);
    CHECK(!readBuiltInSignedIntegerProperty(vbox, horizontal));
    CHECK(!writeBuiltInSignedIntegerProperty(vbox, horizontal, 3));
    CHECK(!readBuiltInSignedIntegerProperty(hbox, vertical));
    CHECK(!writeBuiltInSignedIntegerProperty(hbox, vertical, 3));
    CHECK(!builtInSignedIntegerPropertyDefault("sasd.ui.VBox", horizontal));
    CHECK(!builtInSignedIntegerPropertyDefault("sasd.ui.HBox", vertical));
    for (const auto id : {"columnSpacing", "rowSpacing"}) {
        const auto& grid_property = property("sasd.ui.GridLayout", id);
        const auto& form_property = property("sasd.ui.FormLayout", id);
        CHECK(&grid_property != &form_property);
        CHECK(!readBuiltInSignedIntegerProperty(form, grid_property));
        CHECK(!writeBuiltInSignedIntegerProperty(form, grid_property, 3));
        CHECK(!readBuiltInSignedIntegerProperty(grid, form_property));
        CHECK(!writeBuiltInSignedIntegerProperty(grid, form_property, 3));
        CHECK(!builtInSignedIntegerPropertyDefault("sasd.ui.FormLayout", grid_property));
        CHECK(!builtInSignedIntegerPropertyDefault("sasd.ui.GridLayout", form_property));
    }
    CHECK(hbox.spacing() == 0);
    CHECK(vbox.spacing() == 0);
    CHECK(grid.columnSpacing() == 0);
    CHECK(form.columnSpacing() == 1);
}

TEST_CASE("unsigned column count access checks platform range and delegates domain validation") {
    GridLayout grid;
    Component incompatible;
    const auto descriptor = property("sasd.ui.GridLayout", "columnCount");
    CHECK(descriptor.declaring_type_id == "sasd.ui.GridLayout");
    CHECK(descriptor.value_kind == ComponentPropertyValueKind::unsigned_integer);
    CHECK(readBuiltInUnsignedIntegerProperty(grid, descriptor) == 1);
    CHECK(builtInUnsignedIntegerPropertyDefault("sasd.ui.GridLayout", descriptor) == 1);
    CHECK(writeBuiltInUnsignedIntegerProperty(grid, descriptor, 4));
    CHECK(writeBuiltInUnsignedIntegerProperty(grid, descriptor, 4));
    CHECK(grid.columnCount() == 4);
    CHECK(readBuiltInUnsignedIntegerProperty(grid, descriptor) == 4);
    rejectsDomainValue([&] { (void)writeBuiltInUnsignedIntegerProperty(grid, descriptor, 0); });
    CHECK(grid.columnCount() == 4);
    CHECK(!readBuiltInUnsignedIntegerProperty(incompatible, descriptor));
    CHECK(!writeBuiltInUnsignedIntegerProperty(incompatible, descriptor, 2));
    CHECK(!readBuiltInSignedIntegerProperty(grid, descriptor));
    CHECK(!writeBuiltInSignedIntegerProperty(grid, descriptor, 2));
    CHECK(!readBuiltInBooleanProperty(grid, descriptor));
    CHECK(!writeBuiltInBooleanProperty(grid, descriptor, true));
    CHECK(!readBuiltInStringProperty(grid, descriptor));
    CHECK(!writeBuiltInStringProperty(grid, descriptor, "2"));
    CHECK(!builtInSignedIntegerPropertyDefault("sasd.ui.GridLayout", descriptor));
    CHECK(!builtInUnsignedIntegerPropertyDefault("sasd.ui.FormLayout", descriptor));
    CHECK(!builtInUnsignedIntegerPropertyDefault("sasd.ui.Unknown", descriptor));
    for (const ComponentPropertyMetadata malformed : {
             ComponentPropertyMetadata{"sasd.ui.FormLayout", "columnCount", descriptor.value_kind},
             ComponentPropertyMetadata{"sasd.ui.GridLayout", "unknown", descriptor.value_kind},
             ComponentPropertyMetadata{"sasd.ui.GridLayout", "columnCount", ComponentPropertyValueKind::signed_integer},
             ComponentPropertyMetadata{"sasd.ui.GridLayout", "columnCount", static_cast<ComponentPropertyValueKind>(99)}}) {
        CHECK(!readBuiltInUnsignedIntegerProperty(grid, malformed));
        CHECK(!writeBuiltInUnsignedIntegerProperty(grid, malformed, 2));
        CHECK(!builtInUnsignedIntegerPropertyDefault("sasd.ui.GridLayout", malformed));
        CHECK(grid.columnCount() == 4);
    }
    // No assumption that size_t is 64 bits: on narrower platforms the large writes must fail.
    // On 64-bit platforms UINT64_MAX is representable and must reach the existing setter.
    for (const auto candidate : {std::uint64_t{1} << 32,
                                  std::numeric_limits<std::uint64_t>::max()}) {
        const auto before = grid.columnCount();
        const bool representable = std::in_range<std::size_t>(candidate);
        CHECK(writeBuiltInUnsignedIntegerProperty(grid, descriptor, candidate) == representable);
        if (representable) {
            CHECK(readBuiltInUnsignedIntegerProperty(grid, descriptor) == candidate);
        } else {
            CHECK(grid.columnCount() == before);
        }
    }
}

TEST_CASE("layout snapshots own separate numeric collections and compare all seven defaults") {
    HBox hbox;
    VBox vbox;
    GridLayout grid;
    FormLayout form;
    const std::array<Component*, 4> layouts{{&hbox, &vbox, &grid, &form}};
    for (const auto* layout : layouts) {
        const auto snapshot = captureBuiltInComponentPropertySnapshot(*layout);
        CHECK(snapshot.has_value());
        CHECK(snapshot->boolean_properties.size() == 3);
        CHECK(snapshot->string_properties.empty());
        const auto declared = declaredBuiltInComponentProperties(snapshot->type_id);
        CHECK(snapshot->signed_integer_properties.size() + snapshot->unsigned_integer_properties.size() ==
              declared.size());
        for (const auto& entry : snapshot->signed_integer_properties) {
            CHECK(entry.declaring_type_id == snapshot->type_id);
            CHECK(entry.is_default);
            CHECK(entry.value == builtInSignedIntegerPropertyDefault(
                snapshot->type_id, property(entry.declaring_type_id, entry.property_id)));
        }
        for (const auto& entry : snapshot->unsigned_integer_properties) {
            CHECK(entry.declaring_type_id == "sasd.ui.GridLayout");
            CHECK(entry.property_id == "columnCount");
            CHECK(entry.value == 1);
            CHECK(entry.is_default);
        }
    }
    std::optional<BuiltInComponentPropertySnapshot> owned;
    {
        GridLayout source;
        source.setColumnCount(5);
        source.setColumnSpacing(3);
        source.setRowSpacing(2);
        owned = captureBuiltInComponentPropertySnapshot(source);
        source.setColumnSpacing(0);
    }
    CHECK(owned.has_value());
    CHECK(owned->type_id == "sasd.ui.GridLayout");
    CHECK(owned->signed_integer_properties.size() == 2);
    CHECK(owned->unsigned_integer_properties.size() == 1);
    CHECK(owned->signed_integer_properties[0].declaring_type_id == "sasd.ui.GridLayout");
    CHECK(owned->signed_integer_properties[0].property_id == "columnSpacing");
    CHECK(owned->signed_integer_properties[0].value == 3);
    CHECK(!owned->signed_integer_properties[0].is_default);
    CHECK(owned->signed_integer_properties[1].property_id == "rowSpacing");
    CHECK(owned->signed_integer_properties[1].value == 2);
    CHECK(!owned->signed_integer_properties[1].is_default);
    CHECK(owned->unsigned_integer_properties[0].value == 5);
    CHECK(!owned->unsigned_integer_properties[0].is_default);

    hbox.setSpacing(4);
    form.setColumnSpacing(0);
    form.setRowSpacing(6);
    const auto box_snapshot = captureBuiltInComponentPropertySnapshot(hbox);
    const auto form_snapshot = captureBuiltInComponentPropertySnapshot(form);
    CHECK(box_snapshot.has_value());
    CHECK(box_snapshot->signed_integer_properties[0].property_id == "spacing");
    CHECK(box_snapshot->signed_integer_properties[0].value == 4);
    CHECK(!box_snapshot->signed_integer_properties[0].is_default);
    CHECK(form_snapshot.has_value());
    CHECK(form_snapshot->signed_integer_properties[0].property_id == "columnSpacing");
    CHECK(form_snapshot->signed_integer_properties[0].value == 0);
    CHECK(!form_snapshot->signed_integer_properties[0].is_default);
    CHECK(form_snapshot->signed_integer_properties[1].property_id == "rowSpacing");
    CHECK(form_snapshot->signed_integer_properties[1].value == 6);
    CHECK(!form_snapshot->signed_integer_properties[1].is_default);
}
