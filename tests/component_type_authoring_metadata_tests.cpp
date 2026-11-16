#include "test_framework.hpp"

#include <sasd/ui/component_type_authoring_metadata.hpp>
#include <sasd/ui/component_property_metadata.hpp>

#include <array>
#include <cstddef>
#include <string_view>

using namespace sasd::ui;

namespace {

using Instantiation = ComponentTypeAuthoringInstantiationKind;

constexpr std::array<std::string_view, 3> abstract_types{
    "sasd.ui.ListModel", "sasd.ui.TableModel", "sasd.ui.TreeModel"};

bool isExpectedAbstract(std::string_view type_id) {
    for (const auto expected : abstract_types) {
        if (expected == type_id) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST_CASE("every built-in type has an explicit authoring instantiation classification") {
    std::size_t constructible_count = 0;
    std::size_t abstract_count = 0;

    for (const auto& type : builtInComponentTypes()) {
        const auto metadata = builtInComponentTypeAuthoringMetadata(type);
        CHECK(metadata.has_value());
        if (isExpectedAbstract(type.type_id)) {
            ++abstract_count;
            CHECK(metadata->instantiation == Instantiation::abstract_contract);
        } else {
            ++constructible_count;
            CHECK(metadata->instantiation == Instantiation::constructible);
        }
    }

    CHECK(builtInComponentTypes().size() == 29);
    CHECK(constructible_count == 26);
    CHECK(abstract_count == abstract_types.size());
}

TEST_CASE("type authoring metadata uses complete descriptor value identity and fails closed") {
    const auto* catalog = findBuiltInComponentType("sasd.ui.Button");
    CHECK(catalog != nullptr);

    const ComponentTypeMetadata copied = *catalog;
    CHECK(builtInComponentTypeAuthoringMetadata(copied) ==
          ComponentTypeAuthoringMetadata{Instantiation::constructible});

    auto malformed = copied;
    malformed.base_type_id = "sasd.ui.Container";
    CHECK(!builtInComponentTypeAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.kind = ComponentTypeKind::container;
    CHECK(!builtInComponentTypeAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.kind = static_cast<ComponentTypeKind>(255);
    CHECK(!builtInComponentTypeAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.type_id = "SASD.UI.Button";
    CHECK(!builtInComponentTypeAuthoringMetadata(malformed).has_value());
    malformed = copied;
    malformed.type_id = "sasd.ui.ButtonExtra";
    CHECK(!builtInComponentTypeAuthoringMetadata(malformed).has_value());

    const ComponentTypeMetadata fabricated{
        "sasd.ui.DoesNotExist", "sasd.ui.Widget", ComponentTypeKind::widget};
    CHECK(!builtInComponentTypeAuthoringMetadata(fabricated).has_value());
}

TEST_CASE("abstract model contracts remain discoverable without invented properties") {
    for (const auto type_id : abstract_types) {
        const auto* type = findBuiltInComponentType(type_id);
        CHECK(type != nullptr);
        CHECK(builtInComponentTypeAuthoringMetadata(*type) ==
              ComponentTypeAuthoringMetadata{Instantiation::abstract_contract});
        CHECK(declaredBuiltInComponentProperties(type_id).empty());
    }
}
