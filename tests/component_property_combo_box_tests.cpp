#include "test_framework.hpp"

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_defaults.hpp>
#include <sasd/ui/component_property_snapshot.hpp>

#include <limits>
#include <memory>
#include <utility>

using namespace sasd::ui;

namespace {
ComponentPropertyMetadata itemsProperty() {
    return *findBuiltInComponentProperty("sasd.ui.ComboBox", "items");
}
ComponentPropertyMetadata indexProperty() {
    return *findBuiltInComponentProperty("sasd.ui.ComboBox", "selectedIndex");
}
}

TEST_CASE("ComboBox declares items then selectedIndex then textStyle and excludes derived and transient state") {
    const auto properties = declaredBuiltInComponentProperties("sasd.ui.ComboBox");
    CHECK(properties.size() == 3);
    CHECK(properties[2].property_id == "textStyle");
    CHECK(properties[2].value_kind == ComponentPropertyValueKind::text_style);
    CHECK(properties[0].property_id == "items");
    CHECK(properties[0].value_kind == ComponentPropertyValueKind::string_collection);
    CHECK(properties[1].property_id == "selectedIndex");
    CHECK(properties[1].value_kind == ComponentPropertyValueKind::optional_unsigned_integer);
    for (const auto& property : properties) {
        CHECK(property.declaring_type_id == "sasd.ui.ComboBox");
    }
    for (const auto name : {"selectedText", "previewIndex", "previewText", "dropDownOpen", "pressed"}) {
        CHECK(findBuiltInComponentProperty("sasd.ui.ComboBox", name) == nullptr);
    }
    for (const auto name : {"visible", "enabled", "focusable"}) {
        CHECK(findBuiltInComponentProperty("sasd.ui.ComboBox", name)->declaring_type_id == "sasd.ui.Widget");
    }
    CHECK(findBuiltInComponentProperty("sasd.ui.ComboBox", "Items") == nullptr);
    CHECK(findBuiltInComponentProperty("sasd.ui.ComboBox", "SelectedIndex") == nullptr);
}

TEST_CASE("ComboBox collection reads own strings including empty strings across mutation and destruction") {
    std::optional<std::vector<std::string>> value;
    {
        ComboBox combo;
        const auto empty = readBuiltInStringCollectionProperty(combo, itemsProperty());
        CHECK(empty.has_value());
        CHECK(empty->empty());
        CHECK(writeBuiltInStringCollectionProperty(combo, itemsProperty(), {"A", "", "C"}));
        value = readBuiltInStringCollectionProperty(combo, itemsProperty());
        CHECK(value == std::optional<std::vector<std::string>>{{"A", "", "C"}});
        CHECK(combo.setItems({"replacement"}));
        CHECK(*value == std::vector<std::string>{"A", "", "C"});
    }
    CHECK(*value == std::vector<std::string>{"A", "", "C"});
}

TEST_CASE("ComboBox property writes preserve setter callbacks and same-value semantics") {
    ComboBox combo;
    const auto items = itemsProperty();
    const auto index = indexProperty();
    CHECK(writeBuiltInStringCollectionProperty(combo, items, {"A", "B", "C"}));
    CHECK(combo.itemCount() == 3);
    CHECK(combo.itemAt(0) == "A");
    CHECK(combo.itemAt(1) == "B");
    const auto none = readBuiltInOptionalUnsignedIntegerProperty(combo, index);
    CHECK(none.has_value());
    CHECK(!none->has_value());
    std::size_t calls = 0;
    combo.setOnSelectionChanged([&](std::optional<std::size_t> selected) {
        ++calls;
        CHECK(combo.selectedIndex() == selected);
    });
    CHECK(writeBuiltInOptionalUnsignedIntegerProperty(combo, index, 1));
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{1});
    CHECK(**readBuiltInOptionalUnsignedIntegerProperty(combo, index) == 1);
    CHECK(calls == 1);
    CHECK(writeBuiltInOptionalUnsignedIntegerProperty(combo, index, 1));
    CHECK(calls == 1);
    CHECK(writeBuiltInStringCollectionProperty(combo, items, {"A", "B", "C"}));
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{1});
    CHECK(calls == 1);
    CHECK(writeBuiltInOptionalUnsignedIntegerProperty(combo, index, std::nullopt));
    CHECK(calls == 2);
    CHECK(writeBuiltInOptionalUnsignedIntegerProperty(combo, index, std::nullopt));
    CHECK(calls == 2);
    CHECK(writeBuiltInOptionalUnsignedIntegerProperty(combo, index, 1));
    CHECK(calls == 3);
    CHECK(writeBuiltInStringCollectionProperty(combo, items, {"X", "Y"}));
    CHECK(!combo.selectedIndex().has_value());
    CHECK(calls == 4);
    CHECK(writeBuiltInStringCollectionProperty(combo, items, {"X", "Y"}));
    CHECK(calls == 4);
}

TEST_CASE("ComboBox typed access and defaults reject every malformed descriptor identity") {
    ComboBox combo{{"A", "B"}};
    Button other;
    const auto items = itemsProperty();
    const auto index = indexProperty();
    CHECK(!readBuiltInStringCollectionProperty(other, items));
    CHECK(!writeBuiltInStringCollectionProperty(other, items, {"X"}));
    CHECK(!readBuiltInOptionalUnsignedIntegerProperty(other, index));
    CHECK(!writeBuiltInOptionalUnsignedIntegerProperty(other, index, 1));
    for (const auto target : {"sasd.ui.Button", "sasd.ui.combobox", "unknown"}) {
        CHECK(!builtInStringCollectionPropertyDefault(target, items));
        CHECK(!builtInOptionalUnsignedIntegerPropertyDefault(target, index));
    }
    for (auto invalid : {ComponentPropertyMetadata{"sasd.ui.ComboBox", "items", ComponentPropertyValueKind::string},
                         ComponentPropertyMetadata{"sasd.ui.Widget", "items", ComponentPropertyValueKind::string_collection},
                         ComponentPropertyMetadata{"sasd.ui.ComboBox", "Items", ComponentPropertyValueKind::string_collection}}) {
        CHECK(!readBuiltInStringCollectionProperty(combo, invalid));
        CHECK(!writeBuiltInStringCollectionProperty(combo, invalid, {"X"}));
        CHECK(!builtInStringCollectionPropertyDefault("sasd.ui.ComboBox", invalid));
    }
    for (auto invalid : {ComponentPropertyMetadata{"sasd.ui.ComboBox", "selectedIndex", ComponentPropertyValueKind::unsigned_integer},
                         ComponentPropertyMetadata{"sasd.ui.Widget", "selectedIndex", ComponentPropertyValueKind::optional_unsigned_integer},
                         ComponentPropertyMetadata{"sasd.ui.ComboBox", "SelectedIndex", ComponentPropertyValueKind::optional_unsigned_integer}}) {
        CHECK(!readBuiltInOptionalUnsignedIntegerProperty(combo, invalid));
        CHECK(!writeBuiltInOptionalUnsignedIntegerProperty(combo, invalid, 1));
        CHECK(!builtInOptionalUnsignedIntegerPropertyDefault("sasd.ui.ComboBox", invalid));
    }
    CHECK(combo.itemCount() == 2);
    CHECK(!combo.selectedIndex());
    CHECK(!readBuiltInStringCollectionProperty(combo, index));
    CHECK(!readBuiltInOptionalUnsignedIntegerProperty(combo, items));
    CHECK(!builtInStringCollectionPropertyDefault("sasd.ui.ComboBox", index));
    CHECK(!builtInOptionalUnsignedIntegerPropertyDefault("sasd.ui.ComboBox", items));
}

TEST_CASE("ComboBox defaults distinguish empty collection and valid absent selection") {
    const auto items = builtInStringCollectionPropertyDefault("sasd.ui.ComboBox", itemsProperty());
    CHECK(items.has_value());
    CHECK(items->empty());
    const auto index = builtInOptionalUnsignedIntegerPropertyDefault("sasd.ui.ComboBox", indexProperty());
    CHECK(index.has_value());
    CHECK(!index->has_value());
    CHECK(builtInBooleanPropertyDefault("sasd.ui.ComboBox",
          *findBuiltInComponentProperty("sasd.ui.ComboBox", "focusable")) == std::optional<bool>{true});
}

TEST_CASE("ComboBox index writes check representation but propagate setter domain exceptions") {
    ComboBox combo{{"A", "B"}};
    CHECK(combo.setSelectedIndex(1));
    bool threw = false;
    try {
        (void)writeBuiltInOptionalUnsignedIntegerProperty(combo, indexProperty(), 5);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{1});
    // On narrower size_t platforms this exercises a value outside its range, without assuming bits.
    const auto largest = std::numeric_limits<std::uint64_t>::max();
    if (!std::in_range<std::size_t>(largest)) {
        CHECK(!writeBuiltInOptionalUnsignedIntegerProperty(combo, indexProperty(), largest));
        CHECK(combo.selectedIndex() == std::optional<std::size_t>{1});
    }
}

TEST_CASE("ComboBox property writers permit destruction during semantic callbacks") {
    auto combo = std::make_unique<ComboBox>(std::vector<std::string>{"A", "B"});
    combo->setOnSelectionChanged([&](auto) { combo.reset(); });
    CHECK(writeBuiltInOptionalUnsignedIntegerProperty(*combo, indexProperty(), 1));
    CHECK(!combo);
    combo = std::make_unique<ComboBox>(std::vector<std::string>{"A", "B"});
    CHECK(combo->setSelectedIndex(1));
    combo->setOnSelectionChanged([&](auto selected) {
        CHECK(!selected);
        combo.reset();
    });
    CHECK(writeBuiltInStringCollectionProperty(*combo, itemsProperty(), {"X", "Y"}));
    CHECK(!combo);
}

TEST_CASE("ComboBox snapshots capture defaults owned items selection and replacement clearing") {
    std::optional<BuiltInComponentPropertySnapshot> owned;
    {
        ComboBox combo;
        auto snapshot = captureBuiltInComponentPropertySnapshot(combo);
        CHECK(snapshot.has_value());
        CHECK(snapshot->type_id == "sasd.ui.ComboBox");
        CHECK(snapshot->boolean_properties.size() == 3);
        for (const auto& property : snapshot->boolean_properties) {
            CHECK(property.declaring_type_id == "sasd.ui.Widget");
            CHECK(property.value);
            CHECK(property.is_default);
        }
        CHECK(snapshot->string_properties.empty());
        CHECK(snapshot->signed_integer_properties.empty());
        CHECK(snapshot->unsigned_integer_properties.empty());
        CHECK(snapshot->string_collection_properties.size() == 1);
        CHECK(snapshot->optional_unsigned_integer_properties.size() == 1);
        const auto& items = snapshot->string_collection_properties[0];
        CHECK(items.declaring_type_id == "sasd.ui.ComboBox");
        CHECK(items.property_id == "items");
        CHECK(items.value.empty());
        CHECK(items.is_default);
        const auto& index = snapshot->optional_unsigned_integer_properties[0];
        CHECK(index.declaring_type_id == "sasd.ui.ComboBox");
        CHECK(index.property_id == "selectedIndex");
        CHECK(!index.value);
        CHECK(index.is_default);
        CHECK(combo.setItems({"One", "Two", "Three"}));
        snapshot = captureBuiltInComponentPropertySnapshot(combo);
        CHECK(snapshot.has_value());
        CHECK(!snapshot->string_collection_properties[0].is_default);
        CHECK(snapshot->optional_unsigned_integer_properties[0].is_default);
        CHECK(combo.setSelectedIndex(1));
        owned = captureBuiltInComponentPropertySnapshot(combo);
        CHECK(owned.has_value());
        CHECK(owned->optional_unsigned_integer_properties[0].value == std::optional<std::uint64_t>{1});
        CHECK(!owned->optional_unsigned_integer_properties[0].is_default);
        CHECK(combo.setItems({"X", "Y"}));
        snapshot = captureBuiltInComponentPropertySnapshot(combo);
        CHECK(snapshot.has_value());
        CHECK(snapshot->string_collection_properties[0].value == std::vector<std::string>{"X", "Y"});
        CHECK(!snapshot->string_collection_properties[0].is_default);
        CHECK(!snapshot->optional_unsigned_integer_properties[0].value);
        CHECK(snapshot->optional_unsigned_integer_properties[0].is_default);
    }
    CHECK(owned->string_collection_properties[0].value == std::vector<std::string>{"One", "Two", "Three"});
    CHECK(owned->optional_unsigned_integer_properties[0].value == std::optional<std::uint64_t>{1});
}
