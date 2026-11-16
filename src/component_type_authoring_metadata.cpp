#include <sasd/ui/component_type_authoring_metadata.hpp>

#include <array>
#include <string_view>

namespace sasd::ui {
namespace {

struct AuthoringEntry final {
    std::string_view type_id;
    ComponentTypeAuthoringInstantiationKind instantiation;
};

using Instantiation = ComponentTypeAuthoringInstantiationKind;

// Keep every catalog member explicit. Defaulting every known descriptor to constructible would let
// a future abstract schema type leak into object palettes until somebody happened to notice. This
// private policy table instead makes the complete-catalog test fail whenever the type catalog grows
// without a corresponding authoring decision.
constexpr std::array<AuthoringEntry, 29> authoring_entries{{
    {"sasd.ui.Component", Instantiation::constructible},
    {"sasd.ui.Widget", Instantiation::constructible},
    {"sasd.ui.Container", Instantiation::constructible},
    {"sasd.ui.Window", Instantiation::constructible},
    {"sasd.ui.Command", Instantiation::constructible},
    {"sasd.ui.Label", Instantiation::constructible},
    {"sasd.ui.Button", Instantiation::constructible},
    {"sasd.ui.TextField", Instantiation::constructible},
    {"sasd.ui.CheckBox", Instantiation::constructible},
    {"sasd.ui.RadioButton", Instantiation::constructible},
    {"sasd.ui.RadioGroup", Instantiation::constructible},
    {"sasd.ui.ComboBox", Instantiation::constructible},
    {"sasd.ui.HBox", Instantiation::constructible},
    {"sasd.ui.VBox", Instantiation::constructible},
    {"sasd.ui.GridLayout", Instantiation::constructible},
    {"sasd.ui.FormLayout", Instantiation::constructible},
    {"sasd.ui.StackLayout", Instantiation::constructible},
    {"sasd.ui.ListModel", Instantiation::abstract_contract},
    {"sasd.ui.StringListModel", Instantiation::constructible},
    {"sasd.ui.ListSelectionModel", Instantiation::constructible},
    {"sasd.ui.ListView", Instantiation::constructible},
    {"sasd.ui.TableModel", Instantiation::abstract_contract},
    {"sasd.ui.StringTableModel", Instantiation::constructible},
    {"sasd.ui.TableSelectionModel", Instantiation::constructible},
    {"sasd.ui.TableView", Instantiation::constructible},
    {"sasd.ui.TreeModel", Instantiation::abstract_contract},
    {"sasd.ui.StringTreeModel", Instantiation::constructible},
    {"sasd.ui.TreeSelectionModel", Instantiation::constructible},
    {"sasd.ui.TreeView", Instantiation::constructible},
}};

bool isExactBuiltInType(const ComponentTypeMetadata& type) noexcept {
    // Resolving by stable ID first and then comparing every descriptor field gives copied values the
    // same identity as catalog storage while rejecting a plausible-looking fabricated base or kind.
    const auto* resolved = findBuiltInComponentType(type.type_id);
    return resolved != nullptr && resolved->type_id == type.type_id &&
           resolved->base_type_id == type.base_type_id && resolved->kind == type.kind;
}

} // namespace

std::optional<ComponentTypeAuthoringMetadata>
builtInComponentTypeAuthoringMetadata(const ComponentTypeMetadata& type) noexcept {
    if (!isExactBuiltInType(type)) {
        return std::nullopt;
    }

    for (const auto& entry : authoring_entries) {
        if (entry.type_id == type.type_id) {
            return ComponentTypeAuthoringMetadata{entry.instantiation};
        }
    }

    // No fallback is intentional. Catalog growth without an authoring decision fails closed.
    return std::nullopt;
}

} // namespace sasd::ui
