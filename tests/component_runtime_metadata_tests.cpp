#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/component_runtime_metadata.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/list_model.hpp>
#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/stack_layout.hpp>
#include <sasd/ui/table_model.hpp>
#include <sasd/ui/table_selection_model.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/tree_model.hpp>
#include <sasd/ui/tree_selection_model.hpp>
#include <sasd/ui/tree_view.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/widget.hpp>
#include <sasd/ui/window.hpp>

#include <string_view>

using namespace sasd::ui;

namespace {

void checkExactType(Component& component, std::string_view expected_type_id) {
    const auto* metadata = builtInComponentTypeOf(component);
    CHECK(metadata != nullptr);
    CHECK(metadata == findBuiltInComponentType(expected_type_id));
    CHECK(metadata->type_id == expected_type_id);
}

class UnknownWidget final : public Widget {};
class UnknownLabel final : public Label {
public:
    UnknownLabel() : Label{"custom"} {}
};
class UnknownComponent final : public Component {};

} // namespace

TEST_CASE("runtime metadata identifies exact built-in component types") {
    Component component;
    Widget widget;
    Container container;
    Window window;
    Command command{"Save"};
    Label label{"Name"};
    Button button{"Save"};
    TextField text_field{"initial"};
    CheckBox check_box{"Remember", true};
    RadioButton radio_button{"Choice"};
    ComboBox combo_box{{"one", "two"}};
    HBox hbox;
    VBox vbox;
    GridLayout grid_layout{2};
    FormLayout form_layout;
    StackLayout stack_layout;
    StringListModel list_model{{"one", "two"}};
    ListSelectionModel list_selection;
    ListView list_view;
    StringTableModel table_model{{"Name"}, {{"Ada"}}};
    TableSelectionModel table_selection;
    TableView table_view;
    StringTreeModel tree_model{{"root"}};
    TreeSelectionModel tree_selection;
    TreeView tree_view;

    checkExactType(component, "sasd.ui.Component");
    checkExactType(widget, "sasd.ui.Widget");
    checkExactType(container, "sasd.ui.Container");
    checkExactType(window, "sasd.ui.Window");
    checkExactType(command, "sasd.ui.Command");
    checkExactType(label, "sasd.ui.Label");
    checkExactType(button, "sasd.ui.Button");
    checkExactType(text_field, "sasd.ui.TextField");
    checkExactType(check_box, "sasd.ui.CheckBox");
    checkExactType(radio_button, "sasd.ui.RadioButton");
    checkExactType(combo_box, "sasd.ui.ComboBox");
    checkExactType(hbox, "sasd.ui.HBox");
    checkExactType(vbox, "sasd.ui.VBox");
    checkExactType(grid_layout, "sasd.ui.GridLayout");
    checkExactType(form_layout, "sasd.ui.FormLayout");
    checkExactType(stack_layout, "sasd.ui.StackLayout");
    checkExactType(list_model, "sasd.ui.StringListModel");
    checkExactType(list_selection, "sasd.ui.ListSelectionModel");
    checkExactType(list_view, "sasd.ui.ListView");
    checkExactType(table_model, "sasd.ui.StringTableModel");
    checkExactType(table_selection, "sasd.ui.TableSelectionModel");
    checkExactType(table_view, "sasd.ui.TableView");
    checkExactType(tree_model, "sasd.ui.StringTreeModel");
    checkExactType(tree_selection, "sasd.ui.TreeSelectionModel");
    checkExactType(tree_view, "sasd.ui.TreeView");
}

TEST_CASE("runtime metadata preserves exact identity through a base reference") {
    Button button{"Save"};
    Component& base = button;

    checkExactType(base, "sasd.ui.Button");
}

TEST_CASE("runtime metadata fails closed for unknown derived component types") {
    UnknownWidget unknown_widget;
    UnknownLabel unknown_label;
    UnknownComponent unknown_component;
    Widget& widget_base = unknown_widget;
    Widget& label_base = unknown_label;
    Component& component_base = unknown_component;

    CHECK(builtInComponentTypeOf(unknown_widget) == nullptr);
    CHECK(builtInComponentTypeOf(widget_base) == nullptr);
    CHECK(builtInComponentTypeOf(unknown_label) == nullptr);
    CHECK(builtInComponentTypeOf(label_base) == nullptr);
    CHECK(builtInComponentTypeOf(unknown_component) == nullptr);
    CHECK(builtInComponentTypeOf(component_base) == nullptr);
}

TEST_CASE("runtime metadata returns only descriptors from the immutable catalog") {
    Button button{"Save"};
    const auto* metadata = builtInComponentTypeOf(button);

    CHECK(metadata != nullptr);
    CHECK(metadata >= builtInComponentTypes().data());
    CHECK(metadata < builtInComponentTypes().data() + builtInComponentTypes().size());
    CHECK(metadata->type_id != "Button");
}

TEST_CASE("runtime metadata does not change ownership or parenting") {
    Container root;
    auto& button = root.emplace<Button>("Save");

    CHECK(builtInComponentTypeOf(button) != nullptr);
    CHECK(button.owner() == &root);
    CHECK(button.parent() == &root);
    CHECK(root.componentCount() == 1);
}
