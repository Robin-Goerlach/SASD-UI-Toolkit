#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/component_metadata.hpp>
#include <sasd/ui/container.hpp>

#include <array>
#include <string_view>

using namespace sasd::ui;

namespace {

struct ExpectedType final {
    std::string_view type_id;
    std::string_view base_type_id;
    ComponentTypeKind kind;
};

constexpr std::array<ExpectedType, 25> expected_types{{
    ExpectedType{"sasd.ui.Component", "", ComponentTypeKind::component},
    ExpectedType{"sasd.ui.Widget", "sasd.ui.Component", ComponentTypeKind::widget},
    ExpectedType{"sasd.ui.Container", "sasd.ui.Widget", ComponentTypeKind::container},
    ExpectedType{"sasd.ui.Window", "sasd.ui.Container", ComponentTypeKind::container},
    ExpectedType{"sasd.ui.Command", "sasd.ui.Component", ComponentTypeKind::component},
    ExpectedType{"sasd.ui.Label", "sasd.ui.Widget", ComponentTypeKind::widget},
    ExpectedType{"sasd.ui.Button", "sasd.ui.Widget", ComponentTypeKind::widget},
    ExpectedType{"sasd.ui.TextField", "sasd.ui.Widget", ComponentTypeKind::widget},
    ExpectedType{"sasd.ui.CheckBox", "sasd.ui.Widget", ComponentTypeKind::widget},
    ExpectedType{"sasd.ui.RadioButton", "sasd.ui.Widget", ComponentTypeKind::widget},
    ExpectedType{"sasd.ui.RadioGroup", "sasd.ui.Component", ComponentTypeKind::component},
    ExpectedType{"sasd.ui.ComboBox", "sasd.ui.Widget", ComponentTypeKind::widget},
    ExpectedType{"sasd.ui.HBox", "sasd.ui.Container", ComponentTypeKind::container},
    ExpectedType{"sasd.ui.VBox", "sasd.ui.Container", ComponentTypeKind::container},
    ExpectedType{"sasd.ui.GridLayout", "sasd.ui.Container", ComponentTypeKind::container},
    ExpectedType{"sasd.ui.FormLayout", "sasd.ui.Container", ComponentTypeKind::container},
    ExpectedType{"sasd.ui.StackLayout", "sasd.ui.Container", ComponentTypeKind::container},
    ExpectedType{"sasd.ui.ListModel", "sasd.ui.Component", ComponentTypeKind::component},
    ExpectedType{"sasd.ui.StringListModel", "sasd.ui.ListModel", ComponentTypeKind::component},
    ExpectedType{"sasd.ui.ListView", "sasd.ui.Widget", ComponentTypeKind::widget},
    ExpectedType{"sasd.ui.TableModel", "sasd.ui.Component", ComponentTypeKind::component},
    ExpectedType{"sasd.ui.StringTableModel", "sasd.ui.TableModel", ComponentTypeKind::component},
    ExpectedType{"sasd.ui.TableView", "sasd.ui.Widget", ComponentTypeKind::widget},
    ExpectedType{"sasd.ui.TreeModel", "sasd.ui.Component", ComponentTypeKind::component},
    ExpectedType{"sasd.ui.StringTreeModel", "sasd.ui.TreeModel", ComponentTypeKind::component},
}};

} // namespace

TEST_CASE("built-in component metadata catalog is explicit and complete") {
    const auto catalog = builtInComponentTypes();
    CHECK(catalog.size() == expected_types.size());

    for (const auto& expected : expected_types) {
        const auto* metadata = findBuiltInComponentType(expected.type_id);
        CHECK(metadata != nullptr);
        CHECK(metadata->type_id == expected.type_id);
        CHECK(metadata->base_type_id == expected.base_type_id);
        CHECK(metadata->kind == expected.kind);
    }

    for (std::size_t index = 0; index < catalog.size(); ++index) {
        CHECK(!catalog[index].type_id.empty());
        for (std::size_t other = index + 1; other < catalog.size(); ++other) {
            CHECK(catalog[index].type_id != catalog[other].type_id);
        }
    }
}

TEST_CASE("component metadata base relationships are direct and acyclic") {
    const auto catalog = builtInComponentTypes();

    for (const auto& metadata : catalog) {
        if (metadata.base_type_id.empty()) {
            CHECK(metadata.type_id == "sasd.ui.Component");
            continue;
        }

        CHECK(metadata.base_type_id != metadata.type_id);
        CHECK(findBuiltInComponentType(metadata.base_type_id) != nullptr);

        auto current = metadata;
        for (std::size_t depth = 0; depth < catalog.size(); ++depth) {
            if (current.base_type_id.empty()) {
                break;
            }
            const auto* base = findBuiltInComponentType(current.base_type_id);
            CHECK(base != nullptr);
            current = *base;
        }
        CHECK(current.base_type_id.empty());
    }
}

TEST_CASE("component metadata lookup is exact and unknown IDs are not classified") {
    CHECK(findBuiltInComponentType("sasd.ui.Button") != nullptr);
    CHECK(findBuiltInComponentType("SASD.UI.Button") == nullptr);
    CHECK(findBuiltInComponentType("sasd.ui.ButtonExtra") == nullptr);
    CHECK(findBuiltInComponentType("") == nullptr);
}

TEST_CASE("component metadata catalog is a read-only query and does not own components") {
    Container root;
    auto& button = root.emplace<Button>();
    const auto catalog = builtInComponentTypes();

    CHECK(button.owner() == &root);
    CHECK(button.parent() == &root);
    CHECK(catalog.data() != nullptr);
    CHECK(findBuiltInComponentType("sasd.ui.Button")->kind == ComponentTypeKind::widget);
}
