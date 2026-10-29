#include <sasd/ui/component_reference_metadata.hpp>

#include <array>

namespace sasd::ui {
namespace {

using Reference = BuiltInComponentReferenceKind;

// Keep format-neutral endpoint contracts in one immutable catalog. The order is deterministic for
// tooling and review, but persistence continues to use JSON symbols rather than enum ordinals.
constexpr std::array<BuiltInComponentReferenceMetadata, 11> reference_metadata{{
    {Reference::button_command, "sasd.ui.Button", "sasd.ui.Command"},
    {Reference::radio_button_group, "sasd.ui.RadioButton", "sasd.ui.RadioGroup"},
    {Reference::list_view_model, "sasd.ui.ListView", "sasd.ui.ListModel"},
    {Reference::table_view_model, "sasd.ui.TableView", "sasd.ui.TableModel"},
    {Reference::tree_view_model, "sasd.ui.TreeView", "sasd.ui.TreeModel"},
    {Reference::list_view_selection_model, "sasd.ui.ListView",
     "sasd.ui.ListSelectionModel"},
    {Reference::table_view_selection_model, "sasd.ui.TableView",
     "sasd.ui.TableSelectionModel"},
    {Reference::tree_view_selection_model, "sasd.ui.TreeView",
     "sasd.ui.TreeSelectionModel"},
    {Reference::list_selection_model_model, "sasd.ui.ListSelectionModel",
     "sasd.ui.ListModel"},
    {Reference::table_selection_model_model, "sasd.ui.TableSelectionModel",
     "sasd.ui.TableModel"},
    {Reference::tree_selection_model_model, "sasd.ui.TreeSelectionModel",
     "sasd.ui.TreeModel"},
}};

// RadioGroup exclusivity is a relation-graph invariant, not a property-local rule: only selected
// RadioButtons with an actual group edge participate, and the count is scoped to each target.
constexpr std::array<BuiltInComponentReferenceGraphValidationMetadata, 1>
    radio_button_group_validations{{
        {Reference::radio_button_group,
         BuiltInComponentReferenceGraphValidationKind::
             at_most_one_source_boolean_true_per_target,
         {"sasd.ui.RadioButton", "selected", ComponentPropertyValueKind::boolean}},
    }};

} // namespace

std::span<const BuiltInComponentReferenceMetadata>
builtInComponentReferenceMetadata() noexcept {
    return reference_metadata;
}

const BuiltInComponentReferenceMetadata* findBuiltInComponentReferenceMetadata(
    BuiltInComponentReferenceKind kind) noexcept {
    for (const auto& metadata : reference_metadata) {
        if (metadata.kind == kind) {
            return &metadata;
        }
    }
    return nullptr;
}

std::span<const BuiltInComponentReferenceGraphValidationMetadata>
builtInComponentReferenceGraphValidations(BuiltInComponentReferenceKind kind) noexcept {
    if (kind == Reference::radio_button_group) {
        return radio_button_group_validations;
    }
    return {};
}

} // namespace sasd::ui
