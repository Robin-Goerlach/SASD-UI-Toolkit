#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_snapshot.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/widget.hpp>

#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>

using namespace sasd::ui;

namespace {
ComponentPropertyMetadata sizeProperty() {
    return *findBuiltInComponentProperty("sasd.ui.Widget", "sizeConstraints");
}
constexpr SizeConstraints custom{{10, 5}, {40, 20}, {100, 50}};

void checkSizeSnapshot(const BuiltInComponentPropertySnapshot& snapshot,
                       SizeConstraints value, bool is_default) {
    CHECK(snapshot.size_constraints_properties.size() == 1);
    const auto& captured = snapshot.size_constraints_properties.front();
    CHECK(captured.declaring_type_id == "sasd.ui.Widget");
    CHECK(captured.property_id == "sizeConstraints");
    CHECK(captured.value == value);
    CHECK(captured.is_default == is_default);
}
}

TEST_CASE("Widget size constraints schema is ordered and inherited without duplication") {
    const auto direct = declaredBuiltInComponentProperties("sasd.ui.Widget");
    CHECK(direct.size() == 4);
    const std::array names{"visible", "enabled", "focusable", "sizeConstraints"};
    for (std::size_t index = 0; index < names.size(); ++index) {
        CHECK(direct[index].property_id == names[index]);
        CHECK(direct[index].value_kind == (index == 3 ? ComponentPropertyValueKind::size_constraints
                                                     : ComponentPropertyValueKind::boolean));
    }
    CHECK(builtInComponentTypes().size() == 29);
    std::size_t count = 0;
    for (const auto& type : builtInComponentTypes()) {
        count += declaredBuiltInComponentProperties(type.type_id).size();
        const bool is_widget = findBuiltInComponentProperty(type.type_id, "visible") != nullptr;
        const auto* inherited = findBuiltInComponentProperty(type.type_id, "sizeConstraints");
        CHECK((inherited != nullptr) == is_widget);
        if (is_widget) {
            CHECK(inherited == &direct[3]);
            CHECK(builtInSizeConstraintsPropertyDefault(type.type_id, *inherited) == SizeConstraints{});
            if (type.type_id != "sasd.ui.Widget") {
                for (const auto& property : declaredBuiltInComponentProperties(type.type_id)) {
                    CHECK(property.property_id != "sizeConstraints");
                }
            }
        } else {
            CHECK(!builtInSizeConstraintsPropertyDefault(type.type_id, direct[3]));
        }
        for (const auto excluded : {"bounds", "desiredSize", "measureValid", "isMeasureValid",
                                   "MeasureConstraints", "MeasurementContext", "textStyle"}) {
            CHECK(findBuiltInComponentProperty(type.type_id, excluded) == nullptr);
        }
    }
    CHECK(count == 22);
    static_assert(static_cast<int>(ComponentPropertyValueKind::boolean) == 0);
    static_assert(static_cast<int>(ComponentPropertyValueKind::string) == 1);
    static_assert(static_cast<int>(ComponentPropertyValueKind::signed_integer) == 2);
    static_assert(static_cast<int>(ComponentPropertyValueKind::unsigned_integer) == 3);
    static_assert(static_cast<int>(ComponentPropertyValueKind::string_collection) == 4);
    static_assert(static_cast<int>(ComponentPropertyValueKind::optional_unsigned_integer) == 5);
    static_assert(static_cast<int>(ComponentPropertyValueKind::size_constraints) == 6);
}

TEST_CASE("Size constraints access and defaults validate complete copied descriptor identity") {
    Widget widget;
    const auto valid = sizeProperty();
    CHECK(readBuiltInSizeConstraintsProperty(widget, valid) == SizeConstraints{});
    CHECK(writeBuiltInSizeConstraintsProperty(widget, valid, custom));
    CHECK(builtInSizeConstraintsPropertyDefault("sasd.ui.Button", valid) == SizeConstraints{});
    const std::array invalid{
        ComponentPropertyMetadata{"sasd.ui.Widget", "sizeConstraints", ComponentPropertyValueKind::boolean},
        ComponentPropertyMetadata{"sasd.ui.Button", "sizeConstraints", ComponentPropertyValueKind::size_constraints},
        ComponentPropertyMetadata{"sasd.ui.Widget", "SizeConstraints", ComponentPropertyValueKind::size_constraints},
        ComponentPropertyMetadata{"sasd.ui.Widget", "sizeconstraints", ComponentPropertyValueKind::size_constraints},
        ComponentPropertyMetadata{"sasd.ui.Widget", "SIZECONSTRAINTS", ComponentPropertyValueKind::size_constraints},
        ComponentPropertyMetadata{"sasd.ui.Widget", "fabricated", ComponentPropertyValueKind::size_constraints},
        ComponentPropertyMetadata{},
    };
    for (const auto& property : invalid) {
        CHECK(!readBuiltInSizeConstraintsProperty(widget, property));
        CHECK(!writeBuiltInSizeConstraintsProperty(widget, property, {}));
        CHECK(!builtInSizeConstraintsPropertyDefault("sasd.ui.Widget", property));
        CHECK(widget.sizeConstraints() == custom);
    }
    CHECK(!readBuiltInBooleanProperty(widget, valid));
    CHECK(!writeBuiltInBooleanProperty(widget, valid, false));
    CHECK(!builtInBooleanPropertyDefault("sasd.ui.Widget", valid));
    for (const auto target : {"", "unknown", "SASD.UI.Widget", "sasd.ui.Component", "sasd.ui.Command",
                              "sasd.ui.RadioGroup", "sasd.ui.StringListModel", "sasd.ui.ListSelectionModel"}) {
        CHECK(!builtInSizeConstraintsPropertyDefault(target, valid));
    }
    Component component;
    Command command;
    RadioGroup group;
    ListSelectionModel selection;
    const std::array<Component*, 4> unrelated{&component, &command, &group, &selection};
    for (auto* other : unrelated) {
        CHECK(!readBuiltInSizeConstraintsProperty(*other, valid));
        CHECK(!writeBuiltInSizeConstraintsProperty(*other, valid, custom));
    }
}

TEST_CASE("Size constraints reads own public values across source mutation and destruction") {
    std::optional<SizeConstraints> captured;
    {
        Widget widget;
        Button button;
        CHECK(readBuiltInSizeConstraintsProperty(widget, sizeProperty()) == SizeConstraints{});
        CHECK(readBuiltInSizeConstraintsProperty(button, sizeProperty()) == SizeConstraints{});
        widget.setSizeConstraints(custom);
        captured = readBuiltInSizeConstraintsProperty(widget, sizeProperty());
        widget.setSizeConstraints({});
        CHECK(captured == custom);
    }
    CHECK(captured == custom);
}

TEST_CASE("Size constraints property writes preserve atomic domain validation and permissive hints") {
    Widget widget;
    const SizeConstraints value{{1, 2}, {10, 20}, {100, 200}};
    CHECK(writeBuiltInSizeConstraintsProperty(widget, sizeProperty(), value));
    CHECK(widget.sizeConstraints() == value);
    CHECK(writeBuiltInSizeConstraintsProperty(widget, sizeProperty(), value));
    for (const auto invalid : {SizeConstraints{{100, 10}, {}, {50, 5}},
                               SizeConstraints{{100, 0}, {}, {50, 5}},
                               SizeConstraints{{0, 10}, {}, {50, 5}}}) {
        bool threw = false;
        try {
            (void)writeBuiltInSizeConstraintsProperty(widget, sizeProperty(), invalid);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        CHECK(threw);
        CHECK(widget.sizeConstraints() == value);
    }
    for (const auto valid : {SizeConstraints{{4, 2}, {0, 0}, {20, 10}},
                             SizeConstraints{{4, 2}, {30, 20}, {20, 10}},
                             SizeConstraints{{-20, -10}, {-30, 20}, {-1, -1}},
                             SizeConstraints{{std::numeric_limits<Coordinate>::min(), 0}, {},
                                             {std::numeric_limits<Coordinate>::max(), 0}}}) {
        CHECK(writeBuiltInSizeConstraintsProperty(widget, sizeProperty(), valid));
        CHECK(widget.sizeConstraints() == valid);
        CHECK(readBuiltInSizeConstraintsProperty(widget, sizeProperty()) == valid);
        checkSizeSnapshot(*captureBuiltInComponentPropertySnapshot(widget), valid, false);
    }
}

TEST_CASE("Size constraints property writes retain same-value measure cache and parent propagation") {
    Container parent;
    auto& widget = parent.emplace<Widget>();
    (void)parent.measure();
    (void)widget.measure();
    CHECK(parent.isMeasureValid());
    CHECK(widget.isMeasureValid());
    CHECK(writeBuiltInSizeConstraintsProperty(widget, sizeProperty(), {}));
    CHECK(parent.isMeasureValid());
    CHECK(widget.isMeasureValid());
    CHECK(writeBuiltInSizeConstraintsProperty(widget, sizeProperty(), custom));
    CHECK(!widget.isMeasureValid());
    CHECK(!parent.isMeasureValid());
    (void)parent.measure();
    (void)widget.measure();
    CHECK(writeBuiltInSizeConstraintsProperty(widget, sizeProperty(), custom));
    CHECK(parent.isMeasureValid());
    CHECK(widget.isMeasureValid());
    const SizeConstraints outside{{4, 2}, {}, {20, 10}};
    CHECK(writeBuiltInSizeConstraintsProperty(widget, sizeProperty(), outside));
    CHECK(widget.measure() == Size{4, 2});
    CHECK(widget.sizeConstraints() == outside);
}

TEST_CASE("Size constraints snapshots own default and modified inherited authoring configuration") {
    Widget widget;
    const auto initial = captureBuiltInComponentPropertySnapshot(widget);
    CHECK(initial.has_value());
    CHECK(initial->type_id == "sasd.ui.Widget");
    checkSizeSnapshot(*initial, {}, true);
    CHECK(initial->boolean_properties.size() == 3);
    CHECK(initial->boolean_properties[0].property_id == "visible");
    CHECK(initial->boolean_properties[0].value);
    CHECK(initial->boolean_properties[1].property_id == "enabled");
    CHECK(initial->boolean_properties[1].value);
    CHECK(initial->boolean_properties[2].property_id == "focusable");
    CHECK(!initial->boolean_properties[2].value);
    for (const auto& property : initial->boolean_properties) { CHECK(property.is_default); }
    CHECK(initial->string_properties.empty());
    CHECK(initial->signed_integer_properties.empty());
    CHECK(initial->unsigned_integer_properties.empty());
    CHECK(initial->string_collection_properties.empty());
    CHECK(initial->optional_unsigned_integer_properties.empty());
    CHECK(initial->size_constraints_properties[0].value.maximum ==
          Size{std::numeric_limits<Coordinate>::max(), std::numeric_limits<Coordinate>::max()});
    widget.setSizeConstraints(custom);
    (void)widget.measure();
    widget.arrange({7, 8, 9, 10});
    checkSizeSnapshot(*captureBuiltInComponentPropertySnapshot(widget), custom, false);
    checkSizeSnapshot(*initial, {}, true);
    std::optional<BuiltInComponentPropertySnapshot> owned;
    {
        Button button;
        auto snapshot = captureBuiltInComponentPropertySnapshot(button);
        CHECK(snapshot->type_id == "sasd.ui.Button");
        CHECK(snapshot->boolean_properties.size() == 3);
        CHECK(snapshot->string_properties.size() == 1);
        CHECK(snapshot->string_properties.front().declaring_type_id == "sasd.ui.Button");
        checkSizeSnapshot(*snapshot, {}, true);
        button.setSizeConstraints(custom);
        owned = captureBuiltInComponentPropertySnapshot(button);
        button.setSizeConstraints({});
        checkSizeSnapshot(*owned, custom, false);
    }
    checkSizeSnapshot(*owned, custom, false);
    ComboBox combo;
    const auto snapshot = captureBuiltInComponentPropertySnapshot(combo);
    checkSizeSnapshot(*snapshot, {}, true);
    CHECK(snapshot->boolean_properties.size() == 3);
    CHECK(snapshot->string_collection_properties.size() == 1);
    CHECK(snapshot->string_collection_properties.front().value.empty());
    CHECK(snapshot->string_collection_properties.front().is_default);
    CHECK(snapshot->optional_unsigned_integer_properties.size() == 1);
    CHECK(!snapshot->optional_unsigned_integer_properties.front().value);
    CHECK(snapshot->optional_unsigned_integer_properties.front().is_default);
}
