#include "test_framework.hpp"

#include <sasd/ui/component_reference_snapshot.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/table_selection_model.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/tree_selection_model.hpp>
#include <sasd/ui/tree_view.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <array>
#include <memory>

using namespace sasd::ui;

namespace {

using Kind = BuiltInComponentReferenceKind;

void checkEdge(const ComponentReferenceEdgeSnapshot& edge, ComponentDescriptionNodeId source,
               ComponentDescriptionNodeId target, Kind kind) {
    CHECK(edge.source_node_id == source);
    CHECK(edge.target_node_id == target);
    CHECK(edge.kind == kind);
}

void checkNoPayload(const ComponentOwnershipNodeSnapshot& node, std::string_view type) {
    CHECK(node.properties.type_id == type);
    CHECK(node.properties.boolean_properties.empty());
    CHECK(node.properties.string_properties.empty());
    CHECK(node.properties.signed_integer_properties.empty());
    CHECK(node.properties.unsigned_integer_properties.empty());
}

class UnknownComponent final : public Component {};
class UnknownWidget final : public Widget {};

} // namespace

TEST_CASE("Reference graph accepts standalone and empty or unbound trees") {
    Label label{"Name"};
    const auto standalone = captureBuiltInComponentReferenceGraphSnapshot(label);
    CHECK(standalone.has_value());
    CHECK(standalone->ownership.nodes.size() == 1);
    CHECK(standalone->ownership.nodes[0].properties.string_properties[0].value == "Name");
    CHECK(standalone->reference_edges.empty());
    Container root;
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    root.emplace<Button>();
    root.emplace<RadioButton>();
    root.emplace<ListView>();
    root.emplace<TableView>();
    root.emplace<TreeView>();
    const auto graph = captureBuiltInComponentReferenceGraphSnapshot(root);
    CHECK(graph.has_value());
    CHECK(graph->ownership.nodes.size() == 6);
    CHECK(graph->reference_edges.empty());
}

TEST_CASE("Reference graph captures Button Command IDs with owned properties") {
    Container root;
    auto& command = root.emplace<Command>("Save");
    root.emplace<Button>("Save").bindCommand(command);
    const auto graph = captureBuiltInComponentReferenceGraphSnapshot(root);
    CHECK(graph.has_value());
    CHECK(graph->reference_edges.size() == 1);
    checkEdge(graph->reference_edges[0], 2, 1, Kind::button_command);
    CHECK(graph->ownership.nodes[1].properties.type_id == "sasd.ui.Command");
    CHECK(graph->ownership.nodes[2].properties.string_properties[0].value == "Save");
}

TEST_CASE("Reference graph captures radio membership without selectedButton back edge") {
    Container root;
    auto& group = root.emplace<RadioGroup>();
    root.emplace<RadioButton>(group, "First");
    auto& second = root.emplace<RadioButton>(group, "Second");
    CHECK(second.setSelected(true));
    const auto graph = captureBuiltInComponentReferenceGraphSnapshot(root);
    CHECK(graph.has_value());
    CHECK(graph->reference_edges.size() == 2);
    checkEdge(graph->reference_edges[0], 2, 1, Kind::radio_button_group);
    checkEdge(graph->reference_edges[1], 3, 1, Kind::radio_button_group);
    bool selectedFound = false;
    for (const auto& property : graph->ownership.nodes[3].properties.boolean_properties) {
        if (property.property_id == "selected") {
            CHECK(property.value);
            selectedFound = true;
        }
    }
    CHECK(selectedFound);
    checkNoPayload(graph->ownership.nodes[1], "sasd.ui.RadioGroup");
}

TEST_CASE("Reference graph captures all View Model kinds and exact target types without payload") {
    Container root;
    auto& list = root.emplace<StringListModel>(std::vector<std::string>{"data"});
    root.emplace<ListView>().setModel(&list);
    auto& table = root.emplace<StringTableModel>(std::vector<std::string>{"Column"},
                                               std::vector<std::vector<std::string>>{{"data"}});
    root.emplace<TableView>().setModel(&table);
    auto& tree = root.emplace<StringTreeModel>(std::vector<std::string>{"data"});
    root.emplace<TreeView>().setModel(&tree);
    const auto graph = captureBuiltInComponentReferenceGraphSnapshot(root);
    CHECK(graph.has_value());
    CHECK(graph->reference_edges.size() == 3);
    checkEdge(graph->reference_edges[0], 2, 1, Kind::list_view_model);
    checkEdge(graph->reference_edges[1], 4, 3, Kind::table_view_model);
    checkEdge(graph->reference_edges[2], 6, 5, Kind::tree_view_model);
    checkNoPayload(graph->ownership.nodes[1], "sasd.ui.StringListModel");
    checkNoPayload(graph->ownership.nodes[3], "sasd.ui.StringTableModel");
    checkNoPayload(graph->ownership.nodes[5], "sasd.ui.StringTreeModel");
}

TEST_CASE("Reference graph is deterministic across nested forward and shared target relations") {
    Window root;
    auto groupOwner = std::make_unique<RadioGroup>();
    auto& group = *groupOwner;
    auto& box = root.emplace<VBox>(); // 1; references can point forward outside this visual subtree.
    auto& button = box.emplace<Button>(); // 2
    box.emplace<RadioButton>(group, "First"); // 3
    box.emplace<RadioButton>(group, "Second"); // 4
    auto& listView = box.emplace<ListView>(); // 5
    auto& tableView = box.emplace<TableView>(); // 6
    auto& treeView = box.emplace<TreeView>(); // 7
    box.emplace<Label>("Name"); // 8
    auto& command = root.emplace<Command>("Save"); // 9
    root.adopt(std::move(groupOwner)); // 10
    auto& list = root.emplace<StringListModel>(); // 11
    auto& table = root.emplace<StringTableModel>(); // 12
    auto& tree = root.emplace<StringTreeModel>(); // 13
    button.bindCommand(command);
    listView.setModel(&list);
    tableView.setModel(&table);
    treeView.setModel(&tree);
    const auto ownership = captureBuiltInComponentOwnershipSnapshot(root);
    const auto graph = captureBuiltInComponentReferenceGraphSnapshot(root);
    const auto again = captureBuiltInComponentReferenceGraphSnapshot(root);
    CHECK(ownership.has_value());
    CHECK(graph.has_value());
    CHECK(again.has_value());
    CHECK(graph->ownership.nodes.size() == 14);
    CHECK(graph->ownership.root_node_id == ownership->root_node_id);
    for (std::size_t index = 0; index < ownership->nodes.size(); ++index) {
        const auto& node = graph->ownership.nodes[index];
        CHECK(node.node_id == ownership->nodes[index].node_id);
        CHECK(node.properties.type_id == ownership->nodes[index].properties.type_id);
        CHECK(node.owned_component_ids == ownership->nodes[index].owned_component_ids);
    }
    const std::array<ComponentDescriptionNodeId, 6> targets{9, 10, 10, 11, 12, 13};
    const std::array<Kind, 6> kinds{Kind::button_command, Kind::radio_button_group,
        Kind::radio_button_group, Kind::list_view_model, Kind::table_view_model,
        Kind::tree_view_model};
    CHECK(graph->reference_edges.size() == targets.size());
    CHECK(again->reference_edges.size() == targets.size());
    for (std::size_t index = 0; index < targets.size(); ++index) {
        checkEdge(graph->reference_edges[index], static_cast<ComponentDescriptionNodeId>(index + 2),
                  targets[index], kinds[index]);
        checkEdge(again->reference_edges[index], graph->reference_edges[index].source_node_id,
                  targets[index], kinds[index]);
    }
}

TEST_CASE("Reference graph rejects every live external supported target atomically") {
    Command command;
    RadioGroup group;
    StringListModel list;
    StringTableModel table;
    StringTreeModel tree;
    Container root;
    auto& internal = root.emplace<Command>();
    root.emplace<Button>().bindCommand(internal); // A valid earlier edge must not escape on failure.
    auto& button = root.emplace<Button>();
    button.bindCommand(command);
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    button.unbindCommand();
    auto& radio = root.emplace<RadioButton>(group, "A");
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    auto removedRadio = root.release(radio);
    auto& listView = root.emplace<ListView>();
    listView.setModel(&list);
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    listView.setModel(nullptr);
    auto& tableView = root.emplace<TableView>();
    tableView.setModel(&table);
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    tableView.setModel(nullptr);
    auto& treeView = root.emplace<TreeView>();
    treeView.setModel(&tree);
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    treeView.setModel(nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.size() == 1);
}

TEST_CASE("Reference graph omits expired targets through safe public getters") {
    Container root;
    auto& button = root.emplace<Button>();
    auto& list = root.emplace<ListView>();
    auto& table = root.emplace<TableView>();
    auto& tree = root.emplace<TreeView>();
    {
        Command command;
        RadioGroup group;
        StringListModel listModel;
        StringTableModel tableModel;
        StringTreeModel treeModel;
        button.bindCommand(command);
        root.emplace<RadioButton>(group, "A");
        list.setModel(&listModel);
        table.setModel(&tableModel);
        tree.setModel(&treeModel);
    }
    CHECK(button.boundCommand() == nullptr);
    CHECK(dynamic_cast<const RadioButton&>(root.componentAt(4)).group() == nullptr);
    CHECK(list.model() == nullptr);
    CHECK(table.model() == nullptr);
    CHECK(tree.model() == nullptr);
    const auto graph = captureBuiltInComponentReferenceGraphSnapshot(root);
    CHECK(graph.has_value());
    CHECK(graph->reference_edges.empty());
}

TEST_CASE("Reference graph owns values after runtime mutation and destruction") {
    std::optional<BuiltInComponentReferenceGraphSnapshot> graph;
    {
        Container root;
        auto& command = root.emplace<Command>("Original");
        auto& button = root.emplace<Button>();
        button.bindCommand(command);
        graph = captureBuiltInComponentReferenceGraphSnapshot(root);
        CHECK(graph.has_value());
        command.setText("Changed");
        button.unbindCommand();
        root.emplace<Label>("Later");
    }
    CHECK(graph->ownership.nodes.size() == 3);
    CHECK(graph->ownership.nodes[0].owned_component_ids ==
          std::vector<ComponentDescriptionNodeId>({1, 2}));
    CHECK(graph->ownership.nodes[1].properties.string_properties[0].value == "Original");
    CHECK(graph->reference_edges.size() == 1);
    checkEdge(graph->reference_edges[0], 2, 1, Kind::button_command);
}

TEST_CASE("Reference graph preserves ownership rejection for unknown derived and owned roots") {
    UnknownComponent unknown;
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(unknown));
    Container root;
    auto& command = root.emplace<Command>();
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(command));
    root.emplace<Button>().bindCommand(command);
    root.emplace<UnknownWidget>();
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
}

TEST_CASE("Reference graph List SelectionModel captures complete wiring without current selection") {
    std::optional<BuiltInComponentReferenceGraphSnapshot> graph;
    {
        Window root;
        auto& model = root.emplace<StringListModel>(std::vector<std::string>{"data"});
        auto& selection = root.emplace<ListSelectionModel>();
        auto& view = root.emplace<ListView>();
        selection.setModel(&model);
        view.setModel(&model);
        view.setSelectionModel(&selection);
        CHECK(selection.select(0));
        CHECK(selection.owner() == &root);
        CHECK(root.componentCount() == 3);
        CHECK(root.childCount() == 1);
        CHECK(&root.componentAt(1) == &selection);
        // Direct property capture is valid even with a live selection; no scalar state leaks out.
        const auto properties = captureBuiltInComponentPropertySnapshot(selection);
        CHECK(properties.has_value());
        CHECK(properties->type_id == "sasd.ui.ListSelectionModel");
        CHECK(properties->boolean_properties.empty());
        CHECK(properties->string_properties.empty());
        CHECK(properties->signed_integer_properties.empty());
        CHECK(properties->unsigned_integer_properties.empty());
        graph = captureBuiltInComponentReferenceGraphSnapshot(root);
        CHECK(graph.has_value());
        const auto again = captureBuiltInComponentReferenceGraphSnapshot(root);
        CHECK(again.has_value());
        CHECK(again->reference_edges.size() == 3);
        checkEdge(again->reference_edges[1], 3, 1, Kind::list_view_model);
        checkEdge(again->reference_edges[2], 3, 2, Kind::list_view_selection_model);
    }
    // All nodes/types/wiring survive runtime destruction; selection values have no representation.
    CHECK(graph->ownership.nodes.size() == 4);
    CHECK(graph->ownership.nodes[0].owned_component_ids ==
          std::vector<ComponentDescriptionNodeId>({1, 2, 3}));
    checkNoPayload(graph->ownership.nodes[1], "sasd.ui.StringListModel");
    checkNoPayload(graph->ownership.nodes[2], "sasd.ui.ListSelectionModel");
    CHECK(graph->ownership.nodes[3].properties.type_id == "sasd.ui.ListView");
    CHECK(graph->reference_edges.size() == 3);
    checkEdge(graph->reference_edges[0], 2, 1, Kind::list_selection_model_model);
    checkEdge(graph->reference_edges[1], 3, 1, Kind::list_view_model);
    checkEdge(graph->reference_edges[2], 3, 2, Kind::list_view_selection_model);
}

TEST_CASE("Reference graph List SelectionModel rejects external and omits expired targets") {
    Window root;
    auto& selection = root.emplace<ListSelectionModel>();
    auto& view = root.emplace<ListView>();
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    {
        ListSelectionModel external;
        view.setSelectionModel(&external);
        CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    }
    CHECK(view.selectionModel() == nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    {
        StringListModel external;
        selection.setModel(&external);
        CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    }
    CHECK(selection.model() == nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    view.setSelectionModel(&selection);
    // Release via the ordinary Component API, then destroy: existing safe references must expire.
    auto released = root.release(selection);
    CHECK(released->owner() == nullptr);
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    released.reset();
    CHECK(view.selectionModel() == nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
}

TEST_CASE("Reference graph Table SelectionModel captures complete wiring without current selection") {
    std::optional<BuiltInComponentReferenceGraphSnapshot> graph;
    {
        Window root;
        auto& model = root.emplace<StringTableModel>(std::vector<std::string>{"Column"}, std::vector<std::vector<std::string>>{{"data"}});
        auto& selection = root.emplace<TableSelectionModel>();
        auto& view = root.emplace<TableView>();
        selection.setModel(&model);
        view.setModel(&model);
        view.setSelectionModel(&selection);
        CHECK(selection.select(0, 0));
        CHECK(selection.owner() == &root);
        CHECK(root.componentCount() == 3);
        CHECK(root.childCount() == 1);
        CHECK(&root.componentAt(1) == &selection);
        // Direct property capture is valid even with a live selection; no scalar state leaks out.
        const auto properties = captureBuiltInComponentPropertySnapshot(selection);
        CHECK(properties.has_value());
        CHECK(properties->type_id == "sasd.ui.TableSelectionModel");
        CHECK(properties->boolean_properties.empty());
        CHECK(properties->string_properties.empty());
        CHECK(properties->signed_integer_properties.empty());
        CHECK(properties->unsigned_integer_properties.empty());
        graph = captureBuiltInComponentReferenceGraphSnapshot(root);
        CHECK(graph.has_value());
        const auto again = captureBuiltInComponentReferenceGraphSnapshot(root);
        CHECK(again.has_value());
        CHECK(again->reference_edges.size() == 3);
        checkEdge(again->reference_edges[1], 3, 1, Kind::table_view_model);
        checkEdge(again->reference_edges[2], 3, 2, Kind::table_view_selection_model);
    }
    // All nodes/types/wiring survive runtime destruction; selection values have no representation.
    CHECK(graph->ownership.nodes.size() == 4);
    CHECK(graph->ownership.nodes[0].owned_component_ids ==
          std::vector<ComponentDescriptionNodeId>({1, 2, 3}));
    checkNoPayload(graph->ownership.nodes[1], "sasd.ui.StringTableModel");
    checkNoPayload(graph->ownership.nodes[2], "sasd.ui.TableSelectionModel");
    CHECK(graph->ownership.nodes[3].properties.type_id == "sasd.ui.TableView");
    CHECK(graph->reference_edges.size() == 3);
    checkEdge(graph->reference_edges[0], 2, 1, Kind::table_selection_model_model);
    checkEdge(graph->reference_edges[1], 3, 1, Kind::table_view_model);
    checkEdge(graph->reference_edges[2], 3, 2, Kind::table_view_selection_model);
}

TEST_CASE("Reference graph Table SelectionModel rejects external and omits expired targets") {
    Window root;
    auto& selection = root.emplace<TableSelectionModel>();
    auto& view = root.emplace<TableView>();
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    {
        TableSelectionModel external;
        view.setSelectionModel(&external);
        CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    }
    CHECK(view.selectionModel() == nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    {
        StringTableModel external;
        selection.setModel(&external);
        CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    }
    CHECK(selection.model() == nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    view.setSelectionModel(&selection);
    // Release via the ordinary Component API, then destroy: existing safe references must expire.
    auto released = root.release(selection);
    CHECK(released->owner() == nullptr);
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    released.reset();
    CHECK(view.selectionModel() == nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
}

TEST_CASE("Reference graph Tree SelectionModel captures complete wiring without current selection") {
    std::optional<BuiltInComponentReferenceGraphSnapshot> graph;
    {
        Window root;
        auto& model = root.emplace<StringTreeModel>(std::vector<std::string>{"data"});
        auto& selection = root.emplace<TreeSelectionModel>();
        auto& view = root.emplace<TreeView>();
        selection.setModel(&model);
        view.setModel(&model);
        view.setSelectionModel(&selection);
        CHECK(selection.select(TreeNodePath{{0}}));
        CHECK(selection.owner() == &root);
        CHECK(root.componentCount() == 3);
        CHECK(root.childCount() == 1);
        CHECK(&root.componentAt(1) == &selection);
        // Direct property capture is valid even with a live selection; no scalar state leaks out.
        const auto properties = captureBuiltInComponentPropertySnapshot(selection);
        CHECK(properties.has_value());
        CHECK(properties->type_id == "sasd.ui.TreeSelectionModel");
        CHECK(properties->boolean_properties.empty());
        CHECK(properties->string_properties.empty());
        CHECK(properties->signed_integer_properties.empty());
        CHECK(properties->unsigned_integer_properties.empty());
        graph = captureBuiltInComponentReferenceGraphSnapshot(root);
        CHECK(graph.has_value());
        const auto again = captureBuiltInComponentReferenceGraphSnapshot(root);
        CHECK(again.has_value());
        CHECK(again->reference_edges.size() == 3);
        checkEdge(again->reference_edges[1], 3, 1, Kind::tree_view_model);
        checkEdge(again->reference_edges[2], 3, 2, Kind::tree_view_selection_model);
    }
    // All nodes/types/wiring survive runtime destruction; selection values have no representation.
    CHECK(graph->ownership.nodes.size() == 4);
    CHECK(graph->ownership.nodes[0].owned_component_ids ==
          std::vector<ComponentDescriptionNodeId>({1, 2, 3}));
    checkNoPayload(graph->ownership.nodes[1], "sasd.ui.StringTreeModel");
    checkNoPayload(graph->ownership.nodes[2], "sasd.ui.TreeSelectionModel");
    CHECK(graph->ownership.nodes[3].properties.type_id == "sasd.ui.TreeView");
    CHECK(graph->reference_edges.size() == 3);
    checkEdge(graph->reference_edges[0], 2, 1, Kind::tree_selection_model_model);
    checkEdge(graph->reference_edges[1], 3, 1, Kind::tree_view_model);
    checkEdge(graph->reference_edges[2], 3, 2, Kind::tree_view_selection_model);
}

TEST_CASE("Reference graph Tree SelectionModel rejects external and omits expired targets") {
    Window root;
    auto& selection = root.emplace<TreeSelectionModel>();
    auto& view = root.emplace<TreeView>();
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    {
        TreeSelectionModel external;
        view.setSelectionModel(&external);
        CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    }
    CHECK(view.selectionModel() == nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    {
        StringTreeModel external;
        selection.setModel(&external);
        CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    }
    CHECK(selection.model() == nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
    view.setSelectionModel(&selection);
    // Release via the ordinary Component API, then destroy: existing safe references must expire.
    auto released = root.release(selection);
    CHECK(released->owner() == nullptr);
    CHECK(!captureBuiltInComponentReferenceGraphSnapshot(root));
    released.reset();
    CHECK(view.selectionModel() == nullptr);
    CHECK(captureBuiltInComponentReferenceGraphSnapshot(root)->reference_edges.empty());
}

TEST_CASE("Reference graph preserves shared SelectionModel identity and independent model bindings") {
    Window root;
    // Put views before targets to exercise forward references and within-source kind order.
    auto& first = root.emplace<ListView>(); // 1
    auto& second = root.emplace<ListView>(); // 2
    auto& modelA = root.emplace<StringListModel>(); // 3
    auto& modelB = root.emplace<StringListModel>(); // 4
    auto& selection = root.emplace<ListSelectionModel>(); // 5
    selection.setModel(&modelA);
    first.setModel(&modelA);
    second.setModel(&modelA);
    first.setSelectionModel(&selection);
    second.setSelectionModel(&selection);
    const auto shared = captureBuiltInComponentReferenceGraphSnapshot(root);
    CHECK(shared.has_value());
    CHECK(shared->ownership.nodes.size() == 6);
    CHECK(shared->reference_edges.size() == 5);
    checkEdge(shared->reference_edges[0], 1, 3, Kind::list_view_model);
    checkEdge(shared->reference_edges[1], 1, 5, Kind::list_view_selection_model);
    checkEdge(shared->reference_edges[2], 2, 3, Kind::list_view_model);
    checkEdge(shared->reference_edges[3], 2, 5, Kind::list_view_selection_model);
    checkEdge(shared->reference_edges[4], 5, 3, Kind::list_selection_model_model);
    selection.setModel(&modelB);
    CHECK(first.model() == &modelA);
    CHECK(selection.model() == &modelB);
    const auto mismatched = captureBuiltInComponentReferenceGraphSnapshot(root);
    CHECK(mismatched.has_value());
    CHECK(mismatched->reference_edges.size() == 5);
    checkEdge(mismatched->reference_edges[0], 1, 3, Kind::list_view_model);
    checkEdge(mismatched->reference_edges[1], 1, 5, Kind::list_view_selection_model);
    checkEdge(mismatched->reference_edges[4], 5, 4, Kind::list_selection_model_model);
}
