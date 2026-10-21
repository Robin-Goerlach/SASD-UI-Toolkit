#include <sasd/ui/component_metadata.hpp>

#include <array>

namespace sasd::ui {
namespace {

// Keep this table explicit rather than deriving IDs from RTTI. Besides making IDs stable across
// compilers, the direct base references make omissions and accidental hierarchy changes visible in
// review. The array is immutable and uses only string literals, so lookups need no allocation or
// registration-time initialization.
constexpr std::array<ComponentTypeMetadata, 25> built_in_types{{
    {"sasd.ui.Component", "", ComponentTypeKind::component},
    {"sasd.ui.Widget", "sasd.ui.Component", ComponentTypeKind::widget},
    {"sasd.ui.Container", "sasd.ui.Widget", ComponentTypeKind::container},
    {"sasd.ui.Window", "sasd.ui.Container", ComponentTypeKind::container},
    {"sasd.ui.Command", "sasd.ui.Component", ComponentTypeKind::component},
    {"sasd.ui.Label", "sasd.ui.Widget", ComponentTypeKind::widget},
    {"sasd.ui.Button", "sasd.ui.Widget", ComponentTypeKind::widget},
    {"sasd.ui.TextField", "sasd.ui.Widget", ComponentTypeKind::widget},
    {"sasd.ui.CheckBox", "sasd.ui.Widget", ComponentTypeKind::widget},
    {"sasd.ui.RadioButton", "sasd.ui.Widget", ComponentTypeKind::widget},
    {"sasd.ui.RadioGroup", "sasd.ui.Component", ComponentTypeKind::component},
    {"sasd.ui.ComboBox", "sasd.ui.Widget", ComponentTypeKind::widget},
    {"sasd.ui.HBox", "sasd.ui.Container", ComponentTypeKind::container},
    {"sasd.ui.VBox", "sasd.ui.Container", ComponentTypeKind::container},
    {"sasd.ui.GridLayout", "sasd.ui.Container", ComponentTypeKind::container},
    {"sasd.ui.FormLayout", "sasd.ui.Container", ComponentTypeKind::container},
    {"sasd.ui.StackLayout", "sasd.ui.Container", ComponentTypeKind::container},
    {"sasd.ui.ListModel", "sasd.ui.Component", ComponentTypeKind::component},
    {"sasd.ui.StringListModel", "sasd.ui.ListModel", ComponentTypeKind::component},
    {"sasd.ui.ListView", "sasd.ui.Widget", ComponentTypeKind::widget},
    {"sasd.ui.TableModel", "sasd.ui.Component", ComponentTypeKind::component},
    {"sasd.ui.StringTableModel", "sasd.ui.TableModel", ComponentTypeKind::component},
    {"sasd.ui.TableView", "sasd.ui.Widget", ComponentTypeKind::widget},
    {"sasd.ui.TreeModel", "sasd.ui.Component", ComponentTypeKind::component},
    // StringTreeModel is intentionally included: concrete built-in model types are part of the
    // catalog just like concrete widgets, even though they are not visual components.
    {"sasd.ui.StringTreeModel", "sasd.ui.TreeModel", ComponentTypeKind::component},
}};

} // namespace

std::span<const ComponentTypeMetadata> builtInComponentTypes() noexcept {
    return built_in_types;
}

const ComponentTypeMetadata* findBuiltInComponentType(std::string_view type_id) noexcept {
    for (const auto& metadata : built_in_types) {
        if (metadata.type_id == type_id) {
            return &metadata;
        }
    }
    return nullptr;
}

} // namespace sasd::ui
