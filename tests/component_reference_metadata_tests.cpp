#include "test_framework.hpp"

#include <sasd/ui/component_metadata.hpp>
#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_reference_metadata.hpp>

#include <array>
#include <cstdint>
#include <string_view>

using namespace sasd::ui;

namespace {

using Reference = BuiltInComponentReferenceKind;

struct ExpectedReference final {
    Reference kind;
    std::string_view source;
    std::string_view target;
};

constexpr std::array expected_references{
    ExpectedReference{Reference::button_command, "sasd.ui.Button", "sasd.ui.Command"},
    ExpectedReference{Reference::radio_button_group, "sasd.ui.RadioButton", "sasd.ui.RadioGroup"},
    ExpectedReference{Reference::list_view_model, "sasd.ui.ListView", "sasd.ui.ListModel"},
    ExpectedReference{Reference::table_view_model, "sasd.ui.TableView", "sasd.ui.TableModel"},
    ExpectedReference{Reference::tree_view_model, "sasd.ui.TreeView", "sasd.ui.TreeModel"},
    ExpectedReference{Reference::list_view_selection_model, "sasd.ui.ListView",
                      "sasd.ui.ListSelectionModel"},
    ExpectedReference{Reference::table_view_selection_model, "sasd.ui.TableView",
                      "sasd.ui.TableSelectionModel"},
    ExpectedReference{Reference::tree_view_selection_model, "sasd.ui.TreeView",
                      "sasd.ui.TreeSelectionModel"},
    ExpectedReference{Reference::list_selection_model_model, "sasd.ui.ListSelectionModel",
                      "sasd.ui.ListModel"},
    ExpectedReference{Reference::table_selection_model_model, "sasd.ui.TableSelectionModel",
                      "sasd.ui.TableModel"},
    ExpectedReference{Reference::tree_selection_model_model, "sasd.ui.TreeSelectionModel",
                      "sasd.ui.TreeModel"},
};

bool derivesFrom(std::string_view type_id, std::string_view base_type_id) {
    for (std::size_t depth = 0; depth < builtInComponentTypes().size(); ++depth) {
        if (type_id == base_type_id) {
            return true;
        }
        const auto* type = findBuiltInComponentType(type_id);
        if (type == nullptr || type->base_type_id.empty()) {
            return false;
        }
        type_id = type->base_type_id;
    }
    return false;
}

} // namespace

TEST_CASE("built-in reference metadata covers all eleven kinds in deterministic order") {
    const auto catalog = builtInComponentReferenceMetadata();
    CHECK(catalog.size() == expected_references.size());

    for (std::size_t index = 0; index < expected_references.size(); ++index) {
        const auto& actual = catalog[index];
        const auto& expected = expected_references[index];
        CHECK(actual.kind == expected.kind);
        CHECK(actual.source_type_id == expected.source);
        CHECK(actual.target_type_id == expected.target);
        CHECK(!actual.source_type_id.empty());
        CHECK(!actual.target_type_id.empty());
        CHECK(findBuiltInComponentType(actual.source_type_id) != nullptr);
        CHECK(findBuiltInComponentType(actual.target_type_id) != nullptr);
        CHECK(findBuiltInComponentReferenceMetadata(actual.kind) == &actual);

        for (std::size_t other = 0; other < index; ++other) {
            CHECK(catalog[other].kind != actual.kind);
        }
    }

    const auto fabricated = static_cast<Reference>(255);
    CHECK(findBuiltInComponentReferenceMetadata(fabricated) == nullptr);
    CHECK(builtInComponentReferenceGraphValidations(fabricated).empty());
}

TEST_CASE("model reference target contracts retain built-in base-chain polymorphism") {
    const auto* list = findBuiltInComponentReferenceMetadata(Reference::list_view_model);
    const auto* table = findBuiltInComponentReferenceMetadata(Reference::table_view_model);
    const auto* tree = findBuiltInComponentReferenceMetadata(Reference::tree_view_model);
    CHECK(list != nullptr);
    CHECK(table != nullptr);
    CHECK(tree != nullptr);
    if (list == nullptr || table == nullptr || tree == nullptr) {
        return;
    }

    CHECK(derivesFrom("sasd.ui.ListModel", list->target_type_id));
    CHECK(derivesFrom("sasd.ui.StringListModel", list->target_type_id));
    CHECK(!derivesFrom("sasd.ui.StringTableModel", list->target_type_id));
    CHECK(derivesFrom("sasd.ui.TableModel", table->target_type_id));
    CHECK(derivesFrom("sasd.ui.StringTableModel", table->target_type_id));
    CHECK(derivesFrom("sasd.ui.TreeModel", tree->target_type_id));
    CHECK(derivesFrom("sasd.ui.StringTreeModel", tree->target_type_id));
}

TEST_CASE("RadioGroup graph metadata names the exact RadioButton selected descriptor") {
    for (const auto& reference : builtInComponentReferenceMetadata()) {
        const auto validations = builtInComponentReferenceGraphValidations(reference.kind);
        if (reference.kind != Reference::radio_button_group) {
            CHECK(validations.empty());
            continue;
        }

        CHECK(validations.size() == 1);
        if (validations.size() != 1) {
            return;
        }
        const auto& validation = validations.front();
        CHECK(validation.reference_kind == Reference::radio_button_group);
        CHECK(validation.kind == BuiltInComponentReferenceGraphValidationKind::
                                      at_most_one_source_boolean_true_per_target);
        CHECK(validation.source_property.declaring_type_id == "sasd.ui.RadioButton");
        CHECK(validation.source_property.property_id == "selected");
        CHECK(validation.source_property.value_kind == ComponentPropertyValueKind::boolean);

        const auto* property = findBuiltInComponentProperty(
            validation.source_property.declaring_type_id,
            validation.source_property.property_id);
        CHECK(property != nullptr);
        if (property == nullptr) {
            return;
        }
        CHECK(property->declaring_type_id == validation.source_property.declaring_type_id);
        CHECK(property->property_id == validation.source_property.property_id);
        CHECK(property->value_kind == validation.source_property.value_kind);
        CHECK(derivesFrom(reference.source_type_id,
                          validation.source_property.declaring_type_id));
    }
}
