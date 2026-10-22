#include <sasd/ui/component_runtime_metadata.hpp>

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/list_model.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/stack_layout.hpp>
#include <sasd/ui/table_model.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/tree_model.hpp>
#include <sasd/ui/tree_view.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/widget.hpp>
#include <sasd/ui/window.hpp>

#include <typeinfo>

namespace sasd::ui {
namespace {

[[nodiscard]] const ComponentTypeMetadata* findExact(const Component& component,
                                                       const std::type_info& built_in_type,
                                                       std::string_view type_id) noexcept {
    if (typeid(component) != built_in_type) {
        return nullptr;
    }

    // Keep the public identity in the existing immutable catalog. The string is only the explicit
    // bridge from a C++ built-in class to that catalog; it is never derived from type_info::name().
    return findBuiltInComponentType(type_id);
}

} // namespace

const ComponentTypeMetadata* builtInComponentTypeOf(const Component& component) noexcept {
    // Exact type_info equality is deliberate. A dynamic_cast to Widget or Component would also
    // accept unknown third-party derived types and would therefore lose their additional state.
    if (const auto* metadata = findExact(component, typeid(Component), "sasd.ui.Component")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(Widget), "sasd.ui.Widget")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(Container), "sasd.ui.Container")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(Window), "sasd.ui.Window")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(Command), "sasd.ui.Command")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(Label), "sasd.ui.Label")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(Button), "sasd.ui.Button")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(TextField), "sasd.ui.TextField")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(CheckBox), "sasd.ui.CheckBox")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(RadioButton), "sasd.ui.RadioButton")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(RadioGroup), "sasd.ui.RadioGroup")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(ComboBox), "sasd.ui.ComboBox")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(HBox), "sasd.ui.HBox")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(VBox), "sasd.ui.VBox")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(GridLayout), "sasd.ui.GridLayout")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(FormLayout), "sasd.ui.FormLayout")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(StackLayout), "sasd.ui.StackLayout")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(ListModel), "sasd.ui.ListModel")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(StringListModel),
                                         "sasd.ui.StringListModel")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(ListView), "sasd.ui.ListView")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(TableModel), "sasd.ui.TableModel")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(StringTableModel),
                                         "sasd.ui.StringTableModel")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(TableView), "sasd.ui.TableView")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(TreeModel), "sasd.ui.TreeModel")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(StringTreeModel),
                                         "sasd.ui.StringTreeModel")) {
        return metadata;
    }
    if (const auto* metadata = findExact(component, typeid(TreeView), "sasd.ui.TreeView")) {
        return metadata;
    }
    return nullptr;
}

} // namespace sasd::ui
