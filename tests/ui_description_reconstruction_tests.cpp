#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/component_runtime_metadata.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/focus_manager.hpp>
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
#include <sasd/ui/ui_description_json.hpp>
#include <sasd/ui/ui_description_reconstruction.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/widget.hpp>
#include <sasd/ui/window.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

using namespace sasd::ui;

namespace {

std::string encoded(const Component& root) {
    const auto json = encodeBuiltInUiDescriptionJson(root);
    CHECK(json.has_value());
    return json.value_or(std::string{});
}

BuiltInUiDescriptionDocument parsed(const Component& root) {
    const auto result = parseBuiltInUiDescriptionJson(encoded(root));
    CHECK(result);
    return result.document.value_or(BuiltInUiDescriptionDocument{});
}

std::vector<Component*> flatten(Component& root) {
    std::vector<Component*> result;
    std::vector<Component*> pending{&root};
    while (!pending.empty()) {
        Component* component = pending.back();
        pending.pop_back();
        result.push_back(component);
        if (auto* container = dynamic_cast<Container*>(component)) {
            for (std::size_t index = container->componentCount(); index > 0; --index) {
                pending.push_back(&container->componentAt(index - 1));
            }
        }
    }
    return result;
}

template<class T>
T* firstExact(const std::vector<Component*>& components) {
    for (auto* component : components) {
        if (typeid(*component) == typeid(T)) return static_cast<T*>(component);
    }
    return nullptr;
}

template<class T>
std::vector<T*> allExact(const std::vector<Component*>& components) {
    std::vector<T*> result;
    for (auto* component : components) {
        if (typeid(*component) == typeid(T)) result.push_back(static_cast<T*>(component));
    }
    return result;
}

std::unique_ptr<Component> reconstruct(const Component& root) {
    const auto parse_result = parseBuiltInUiDescriptionJson(encoded(root));
    CHECK(parse_result);
    auto result = reconstructBuiltInUiDescription(*parse_result.document);
    CHECK(result);
    CHECK(result.error == UiDescriptionReconstructionErrorCode::none);
    return std::move(result.root);
}

} // namespace

TEST_CASE("UI description reconstruction preserves a styled standalone Label byte for byte") {
    Label original{"Gr\xc3\xbc\xc3\x9f"};
    original.setVisible(false);
    original.setEnabled(false);
    original.setFocusable(true);
    original.setSizeConstraints({{-3, -2}, {12, 4}, {40, 8}});
    const TextStyle style{Color::bright_cyan, true, true, true, true};
    original.setTextStyle(style);

    const auto before = encoded(original);
    const auto parse_result = parseBuiltInUiDescriptionJson(before);
    CHECK(parse_result);
    auto result = reconstructBuiltInUiDescription(*parse_result.document);
    CHECK(result);
    CHECK(result.error == UiDescriptionReconstructionErrorCode::none);
    auto* label = dynamic_cast<Label*>(result.root.get());
    CHECK(label != nullptr);
    CHECK(label->text() == original.text());
    CHECK(label->isVisible() == original.isVisible());
    CHECK(label->isEnabled() == original.isEnabled());
    CHECK(label->isFocusable() == original.isFocusable());
    CHECK(label->sizeConstraints() == original.sizeConstraints());
    CHECK(label->textStyle() == style);
    CHECK(encoded(*result.root) == before);
}

TEST_CASE("UI description reconstruction knows every concrete and abstract catalog type") {
    std::vector<std::unique_ptr<Component>> concrete;
    concrete.push_back(std::make_unique<Component>());
    concrete.push_back(std::make_unique<Widget>());
    concrete.push_back(std::make_unique<Container>());
    concrete.push_back(std::make_unique<Window>());
    concrete.push_back(std::make_unique<Command>());
    concrete.push_back(std::make_unique<Label>());
    concrete.push_back(std::make_unique<Button>());
    concrete.push_back(std::make_unique<TextField>());
    concrete.push_back(std::make_unique<CheckBox>());
    concrete.push_back(std::make_unique<RadioButton>());
    concrete.push_back(std::make_unique<RadioGroup>());
    concrete.push_back(std::make_unique<ComboBox>());
    concrete.push_back(std::make_unique<HBox>());
    concrete.push_back(std::make_unique<VBox>());
    concrete.push_back(std::make_unique<GridLayout>());
    concrete.push_back(std::make_unique<FormLayout>());
    concrete.push_back(std::make_unique<StackLayout>());
    concrete.push_back(std::make_unique<StringListModel>());
    concrete.push_back(std::make_unique<ListSelectionModel>());
    concrete.push_back(std::make_unique<ListView>());
    concrete.push_back(std::make_unique<StringTableModel>());
    concrete.push_back(std::make_unique<TableSelectionModel>());
    concrete.push_back(std::make_unique<TableView>());
    concrete.push_back(std::make_unique<StringTreeModel>());
    concrete.push_back(std::make_unique<TreeSelectionModel>());
    concrete.push_back(std::make_unique<TreeView>());

    for (const auto& original : concrete) {
        const auto* expected_type = builtInComponentTypeOf(*original);
        CHECK(expected_type != nullptr);
        auto rebuilt = reconstruct(*original);
        const auto* rebuilt_type = builtInComponentTypeOf(*rebuilt);
        CHECK(rebuilt_type != nullptr);
        CHECK(rebuilt_type->type_id == expected_type->type_id);
        CHECK(encoded(*rebuilt) == encoded(*original));
    }

    constexpr std::array abstract_types{
        "sasd.ui.ListModel", "sasd.ui.TableModel", "sasd.ui.TreeModel"};
    for (const auto type : abstract_types) {
        const std::string json = std::string{
            "{\n  \"format\": \"sasd.ui-description\",\n  \"formatVersion\": 1,\n"
            "  \"root\": 0,\n  \"nodes\": [{\"id\":0,\"type\":\""} + type +
            "\",\"owns\":[],\"properties\":[]}],\n  \"references\": []\n}\n";
        const auto parse_result = parseBuiltInUiDescriptionJson(json);
        CHECK(parse_result);
        const auto result = reconstructBuiltInUiDescription(*parse_result.document);
        CHECK(!result);
        CHECK(result.root == nullptr);
        CHECK(result.error == UiDescriptionReconstructionErrorCode::unconstructible_component_type);
        CHECK(result.node_id == 0);
    }
}

TEST_CASE("UI description reconstruction canonical comprehensive graph roundtrips") {
    Window original;
    auto& command = original.emplace<Command>("Command");
    command.setEnabled(true);
    auto& group = original.emplace<RadioGroup>();
    auto& list_model = original.emplace<StringListModel>(
        std::vector<std::string>{"application", "payload"});
    auto& list_selection = original.emplace<ListSelectionModel>();
    list_selection.setModel(&list_model);
    CHECK(list_selection.select(1));
    auto& table_model = original.emplace<StringTableModel>(
        std::vector<std::string>{"A"}, std::vector<std::vector<std::string>>{{"payload"}});
    auto& table_selection = original.emplace<TableSelectionModel>();
    table_selection.setModel(&table_model);
    CHECK(table_selection.select(0, 0));
    auto& tree_model = original.emplace<StringTreeModel>(std::vector<std::string>{"payload"});
    tree_model.append(TreeNodePath{{0}}, "child payload");
    auto& tree_selection = original.emplace<TreeSelectionModel>();
    tree_selection.setModel(&tree_model);
    CHECK(tree_selection.select(TreeNodePath{{0}}));

    auto& column = original.emplace<VBox>();
    column.setSpacing(3);
    auto& label = column.emplace<Label>("Heading");
    label.setTextStyle({Color::bright_yellow, true, false, true, true});
    label.setSizeConstraints({{2, 1}, {20, 2}, {80, 4}});
    auto& button = column.emplace<Button>();
    button.bindCommand(command);
    button.setText("Local snapshot");
    button.setEnabled(false);
    auto& text_field = column.emplace<TextField>("Input");
    text_field.setSelection(1, 4);
    auto& check = column.emplace<CheckBox>("Check");
    check.setChecked(true);
    auto& first_radio = column.emplace<RadioButton>(group, "One");
    auto& second_radio = column.emplace<RadioButton>(group, "Two");
    CHECK(second_radio.setSelected(true));
    auto& combo = column.emplace<ComboBox>(std::vector<std::string>{"One", "Two", "Three"});
    CHECK(combo.setSelectedIndex(1));
    FocusManager focus;
    CHECK(focus.requestFocus(combo));
    CHECK(combo.setDropDownOpen(true));
    CHECK(combo.setPreviewIndex(2));

    auto& grid = column.emplace<GridLayout>();
    grid.setColumnCount(2);
    grid.setColumnSpacing(4);
    grid.setRowSpacing(5);
    grid.emplace<Label>("G1");
    grid.emplace<Label>("G2");
    auto& form = column.emplace<FormLayout>();
    form.setColumnSpacing(6);
    form.setRowSpacing(7);
    form.adopt(std::make_unique<Label>("Name"));
    form.adopt(std::make_unique<TextField>("Ada"));
    auto& stack = column.emplace<StackLayout>();
    stack.emplace<Label>("Bottom");
    stack.emplace<Label>("Top");

    auto& list_view = column.emplace<ListView>();
    list_view.setModel(&list_model);
    list_view.setSelectionModel(&list_selection);
    list_view.setViewport(1, 1);
    auto& shared_list_view = column.emplace<ListView>();
    shared_list_view.setModel(&list_model);
    shared_list_view.setSelectionModel(&list_selection);
    auto& table_view = column.emplace<TableView>();
    table_view.setModel(&table_model);
    table_view.setSelectionModel(&table_selection);
    table_view.setViewport(0, 1, 0, 1);
    auto& tree_view = column.emplace<TreeView>();
    tree_view.setModel(&tree_model);
    tree_view.setSelectionModel(&tree_selection);
    CHECK(tree_view.expand(TreeNodePath{{0}}));
    first_radio.arrange({9, 8, 7, 6});

    const auto before = encoded(original);
    const auto parse_result = parseBuiltInUiDescriptionJson(before);
    CHECK(parse_result);
    CHECK(parse_result.document->reference_edges.size() == 14);
    auto result = reconstructBuiltInUiDescription(*parse_result.document);
    CHECK(result);
    auto components = flatten(*result.root);
    auto* rebuilt_command = firstExact<Command>(components);
    auto* rebuilt_group = firstExact<RadioGroup>(components);
    auto* rebuilt_button = firstExact<Button>(components);
    auto radios = allExact<RadioButton>(components);
    auto list_views = allExact<ListView>(components);
    auto* rebuilt_list_model = firstExact<StringListModel>(components);
    auto* rebuilt_list_selection = firstExact<ListSelectionModel>(components);
    auto* rebuilt_table_model = firstExact<StringTableModel>(components);
    auto* rebuilt_table_selection = firstExact<TableSelectionModel>(components);
    auto* rebuilt_table_view = firstExact<TableView>(components);
    auto* rebuilt_tree_model = firstExact<StringTreeModel>(components);
    auto* rebuilt_tree_selection = firstExact<TreeSelectionModel>(components);
    auto* rebuilt_tree_view = firstExact<TreeView>(components);
    auto* rebuilt_combo = firstExact<ComboBox>(components);
    auto* rebuilt_text_field = firstExact<TextField>(components);
    CHECK(rebuilt_command && rebuilt_group && rebuilt_button && rebuilt_list_model &&
            rebuilt_list_selection && rebuilt_table_model && rebuilt_table_selection &&
            rebuilt_table_view && rebuilt_tree_model && rebuilt_tree_selection &&
            rebuilt_tree_view && rebuilt_combo && rebuilt_text_field);
    CHECK(radios.size() == 2);
    CHECK(list_views.size() == 2);

    CHECK(rebuilt_button->boundCommand() == rebuilt_command);
    CHECK(rebuilt_button->text() == "Local snapshot");
    CHECK(!rebuilt_button->isEnabled());
    rebuilt_command->setText("Changed later");
    CHECK(rebuilt_button->text() == "Changed later");
    rebuilt_command->setText("Command");
    CHECK(rebuilt_button->text() == "Command");
    rebuilt_button->setText("Local snapshot");

    CHECK(radios[0]->group() == rebuilt_group);
    CHECK(radios[1]->group() == rebuilt_group);
    CHECK(rebuilt_group->memberCount() == 2);
    CHECK(!radios[0]->isSelected());
    CHECK(radios[1]->isSelected());
    CHECK(rebuilt_group->selectedButton() == radios[1]);
    CHECK(rebuilt_combo->itemCount() == 3);
    CHECK(rebuilt_combo->selectedIndex() == std::optional<std::size_t>{1});
    CHECK(rebuilt_combo->selectedText() == std::optional<std::string_view>{"Two"});
    CHECK(!rebuilt_combo->hasFocus());
    CHECK(!rebuilt_combo->isDropDownOpen());
    CHECK(!rebuilt_combo->previewIndex());
    CHECK(rebuilt_text_field->cursorPosition() == 0);
    CHECK(rebuilt_text_field->selectionAnchor() == 0);
    CHECK(!rebuilt_text_field->hasSelection());

    CHECK(rebuilt_list_model->rowCount() == 0);
    CHECK(rebuilt_table_model->rowCount() == 0);
    CHECK(rebuilt_tree_model->childCount({}) == 0);
    CHECK(rebuilt_list_selection->model() == rebuilt_list_model);
    CHECK(!rebuilt_list_selection->selectedRow());
    CHECK(list_views[0]->model() == rebuilt_list_model);
    CHECK(list_views[1]->model() == rebuilt_list_model);
    CHECK(list_views[0]->selectionModel() == rebuilt_list_selection);
    CHECK(list_views[1]->selectionModel() == rebuilt_list_selection);
    CHECK(rebuilt_table_selection->model() == rebuilt_table_model);
    CHECK(!rebuilt_table_selection->selectedCell());
    CHECK(rebuilt_table_view->model() == rebuilt_table_model);
    CHECK(rebuilt_table_view->selectionModel() == rebuilt_table_selection);
    CHECK(rebuilt_tree_selection->model() == rebuilt_tree_model);
    CHECK(!rebuilt_tree_selection->selectedNode());
    CHECK(rebuilt_tree_view->model() == rebuilt_tree_model);
    CHECK(rebuilt_tree_view->selectionModel() == rebuilt_tree_selection);
    CHECK(rebuilt_tree_view->firstVisibleNode() == std::nullopt);
    CHECK(radios[0]->bounds() == Rect{});

    // Payload, selection, viewport, expansion and arranged geometry were never persistent. Their
    // absence changes no canonical byte because the writer observes only the UI-description contract.
    CHECK(encoded(*result.root) == before);
}

TEST_CASE("UI description reconstruction revalidates mutable aggregate documents") {
    Window root;
    root.emplace<Label>("A");
    root.emplace<Label>("B");
    auto duplicate_ownership = parsed(root);
    duplicate_ownership.nodes[0].owned_component_ids.push_back(1);
    CHECK(reconstructBuiltInUiDescription(duplicate_ownership).error ==
          UiDescriptionReconstructionErrorCode::invalid_document);

    Label label;
    auto wrong_property = parsed(label);
    wrong_property.nodes[0].string_properties[0].property_id = "bogus";
    CHECK(reconstructBuiltInUiDescription(wrong_property).error ==
          UiDescriptionReconstructionErrorCode::invalid_document);

    Window references;
    auto& model = references.emplace<StringListModel>();
    auto& view = references.emplace<ListView>();
    view.setModel(&model);
    auto wrong_target = parsed(references);
    wrong_target.reference_edges.front().target_node_id = 0;
    CHECK(reconstructBuiltInUiDescription(wrong_target).error ==
          UiDescriptionReconstructionErrorCode::invalid_document);

    ComboBox combo{std::vector<std::string>{"Only"}};
    auto invalid_index = parsed(combo);
    invalid_index.nodes[0].optional_unsigned_integer_properties[0].value = 1;
    CHECK(reconstructBuiltInUiDescription(invalid_index).error ==
          UiDescriptionReconstructionErrorCode::invalid_document);
}

TEST_CASE("UI description reconstruction fails atomically for setter domain errors") {
    VBox original;
    auto document = parsed(original);
    document.nodes[0].signed_integer_properties[0].value = -1;
    const auto result = reconstructBuiltInUiDescription(document);
    CHECK(!result);
    CHECK(result.root == nullptr);
    CHECK(result.error == UiDescriptionReconstructionErrorCode::property_restore_failed);
    CHECK(result.node_id == 0);
}

TEST_CASE("UI description reconstruction reports host representability without narrowing") {
    GridLayout original;
    auto document = parsed(original);
    document.nodes[0].unsigned_integer_properties[0].value =
        std::numeric_limits<std::uint64_t>::max();
    const auto result = reconstructBuiltInUiDescription(document);
    if constexpr (std::numeric_limits<std::size_t>::max() <
                  std::numeric_limits<std::uint64_t>::max()) {
        CHECK(!result);
        CHECK(result.error == UiDescriptionReconstructionErrorCode::value_not_representable);
    } else {
        CHECK(result);
        CHECK(dynamic_cast<GridLayout*>(result.root.get())->columnCount() ==
              std::numeric_limits<std::size_t>::max());
    }
}
