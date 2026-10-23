#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/component_ownership_snapshot.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/list_model.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <array>
#include <memory>
#include <string_view>

using namespace sasd::ui;

namespace {

class UnknownComponent final : public Component {};
class UnknownWidget final : public Widget {};
class UnknownContainer final : public Container {};

void checkText(const ComponentOwnershipNodeSnapshot& node, std::string_view value) {
    CHECK(node.properties.string_properties.size() == 1);
    CHECK(node.properties.string_properties[0].property_id == "text");
    CHECK(node.properties.string_properties[0].value == value);
    CHECK(node.properties.string_properties[0].is_default == value.empty());
}

} // namespace

TEST_CASE("Ownership snapshot accepts standalone visual and non-visual roots") {
    Label label{"Name"};
    const auto snapshot = captureBuiltInComponentOwnershipSnapshot(label);
    CHECK(snapshot.has_value());
    CHECK(snapshot->root_node_id == 0);
    CHECK(snapshot->nodes.size() == 1);
    CHECK(snapshot->nodes[0].node_id == 0);
    CHECK(snapshot->nodes[0].properties.type_id == "sasd.ui.Label");
    CHECK(snapshot->nodes[0].owned_component_ids.empty());
    checkText(snapshot->nodes[0], "Name");

    Command command{"Save"};
    const auto nonVisual = captureBuiltInComponentOwnershipSnapshot(command);
    CHECK(nonVisual.has_value());
    CHECK(nonVisual->root_node_id == 0);
    CHECK(nonVisual->nodes.size() == 1);
    CHECK(nonVisual->nodes[0].node_id == 0);
    CHECK(nonVisual->nodes[0].properties.type_id == "sasd.ui.Command");
    CHECK(nonVisual->nodes[0].owned_component_ids.empty());
    checkText(nonVisual->nodes[0], "Save");
}

TEST_CASE("Ownership snapshot preserves mixed adoption order including non-visual models") {
    Container root;
    auto& command = root.emplace<Command>("Save");
    root.emplace<Label>("Name");
    root.emplace<RadioGroup>();
    root.emplace<Button>("Save").bindCommand(command);
    root.emplace<StringListModel>(std::vector<std::string>{"application data"});
    CHECK(root.componentCount() == 5);
    CHECK(root.childCount() == 2);
    const auto snapshot = captureBuiltInComponentOwnershipSnapshot(root);
    CHECK(snapshot.has_value());
    CHECK(snapshot->nodes.size() == 6);
    CHECK(snapshot->nodes[0].owned_component_ids ==
          std::vector<ComponentDescriptionNodeId>({1, 2, 3, 4, 5}));
    const std::array<std::string_view, 6> types{
        "sasd.ui.Container", "sasd.ui.Command", "sasd.ui.Label", "sasd.ui.RadioGroup",
        "sasd.ui.Button", "sasd.ui.StringListModel"};
    for (std::size_t index = 0; index < types.size(); ++index) {
        CHECK(snapshot->nodes[index].node_id == index);
        CHECK(snapshot->nodes[index].properties.type_id == types[index]);
        if (index != 0) {
            CHECK(snapshot->nodes[index].owned_component_ids.empty());
        }
    }
    const auto& model = snapshot->nodes[5].properties;
    CHECK(model.boolean_properties.empty());
    CHECK(model.string_properties.empty());
    CHECK(model.signed_integer_properties.empty());
    CHECK(model.unsigned_integer_properties.empty());
}

TEST_CASE("Ownership snapshot uses dense deterministic preorder with explicit ordered edges") {
    Window root;
    root.emplace<Command>("Save");
    auto& box = root.emplace<VBox>();
    box.setSpacing(3);
    box.emplace<Label>("Name");
    box.emplace<TextField>("Value");
    auto& row = box.emplace<HBox>();
    row.emplace<Button>("OK");
    row.emplace<CheckBox>("Enabled");
    const std::array<const Component*, 8> components{
        &root, &root.componentAt(0), &box, &box.componentAt(0), &box.componentAt(1), &row,
        &row.componentAt(0), &row.componentAt(1)};
    const std::array<std::string_view, 8> types{
        "sasd.ui.Window", "sasd.ui.Command", "sasd.ui.VBox", "sasd.ui.Label",
        "sasd.ui.TextField", "sasd.ui.HBox", "sasd.ui.Button", "sasd.ui.CheckBox"};
    const std::array<std::vector<ComponentDescriptionNodeId>, 8> edges{
        std::vector<ComponentDescriptionNodeId>{1, 2}, {}, {3, 4, 5}, {}, {}, {6, 7}, {}, {}};
    const auto first = captureBuiltInComponentOwnershipSnapshot(root);
    const auto second = captureBuiltInComponentOwnershipSnapshot(root);
    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(first->root_node_id == 0);
    CHECK(first->nodes.size() == components.size());
    CHECK(second->nodes.size() == first->nodes.size());
    for (std::size_t index = 0; index < components.size(); ++index) {
        CHECK(first->nodes[index].node_id == index);
        CHECK(first->nodes[index].properties.type_id == types[index]);
        CHECK(first->nodes[index].owned_component_ids == edges[index]);
        CHECK(second->nodes[index].node_id == first->nodes[index].node_id);
        CHECK(second->nodes[index].owned_component_ids == edges[index]);
        // Validate the actual adoption invariants without exposing mutation seams just for tests.
        for (const auto childId : edges[index]) {
            CHECK(components[childId]->owner() == components[index]);
            if (const auto* widget = dynamic_cast<const Widget*>(components[childId])) {
                CHECK(widget->parent() == components[index]);
            }
        }
    }
    checkText(first->nodes[1], "Save");
    checkText(first->nodes[3], "Name");
    checkText(first->nodes[4], "Value");
    checkText(first->nodes[6], "OK");
    checkText(first->nodes[7], "Enabled");
    CHECK(first->nodes[2].properties.signed_integer_properties[0].value == 3);
    CHECK(!first->nodes[2].properties.signed_integer_properties[0].is_default);
}

TEST_CASE("Ownership snapshot rejects owned roots and unknown exact types atomically") {
    Container root;
    auto& child = root.emplace<Label>("child");
    CHECK(!captureBuiltInComponentOwnershipSnapshot(child).has_value());
    CHECK(!captureBuiltInComponentOwnershipSnapshot(root.emplace<Command>()).has_value());
    UnknownComponent unknown;
    CHECK(!captureBuiltInComponentOwnershipSnapshot(unknown).has_value());
    auto& nested = root.emplace<VBox>();
    auto& bad = nested.emplace<UnknownComponent>();
    CHECK(!captureBuiltInComponentOwnershipSnapshot(root).has_value());
    auto released = nested.release(bad);
    CHECK(captureBuiltInComponentOwnershipSnapshot(root).has_value());
    nested.emplace<UnknownWidget>();
    CHECK(!captureBuiltInComponentOwnershipSnapshot(root).has_value());
    Container other;
    other.emplace<UnknownContainer>().emplace<Label>("known descendant");
    CHECK(!captureBuiltInComponentOwnershipSnapshot(other).has_value());
}

TEST_CASE("Ownership snapshot owns identities edges and all property value categories") {
    std::optional<BuiltInComponentOwnershipSnapshot> snapshot;
    {
        auto root = std::make_unique<GridLayout>();
        root->setColumnCount(3);
        root->setColumnSpacing(2);
        auto& label = root->emplace<Label>("Original");
        label.setEnabled(false);
        snapshot = captureBuiltInComponentOwnershipSnapshot(*root);
        CHECK(snapshot.has_value());
        label.setText("Changed");
        root->setColumnCount(9);
        root->emplace<Command>("Later");
        auto removed = root->release(label);
    }
    CHECK(snapshot->root_node_id == 0);
    CHECK(snapshot->nodes.size() == 2);
    CHECK(snapshot->nodes[0].node_id == 0);
    CHECK(snapshot->nodes[1].node_id == 1);
    CHECK(snapshot->nodes[0].owned_component_ids == std::vector<ComponentDescriptionNodeId>{1});
    CHECK(snapshot->nodes[1].owned_component_ids.empty());
    CHECK(snapshot->nodes[0].properties.unsigned_integer_properties[0].value == 3);
    CHECK(!snapshot->nodes[0].properties.unsigned_integer_properties[0].is_default);
    CHECK(snapshot->nodes[0].properties.signed_integer_properties[0].value == 2);
    checkText(snapshot->nodes[1], "Original");
    bool foundEnabled = false;
    for (const auto& property : snapshot->nodes[1].properties.boolean_properties) {
        if (property.property_id == "enabled") {
            foundEnabled = true;
            CHECK(!property.value);
            CHECK(!property.is_default);
        }
    }
    CHECK(foundEnabled);
}

TEST_CASE("Ownership snapshot includes bound controls structurally without reference edges") {
    Container root;
    auto& group = root.emplace<RadioGroup>();
    auto& radio = root.emplace<RadioButton>(group, "Choice");
    auto& command = root.emplace<Command>("Save");
    root.emplace<Button>().bindCommand(command);
    CHECK(radio.setSelected(true));
    const auto snapshot = captureBuiltInComponentOwnershipSnapshot(root);
    CHECK(snapshot.has_value());
    CHECK(snapshot->nodes.size() == 5);
    CHECK(snapshot->nodes[0].owned_component_ids ==
          std::vector<ComponentDescriptionNodeId>({1, 2, 3, 4}));
    const std::array<std::string_view, 4> types{
        "sasd.ui.RadioGroup", "sasd.ui.RadioButton", "sasd.ui.Command", "sasd.ui.Button"};
    for (std::size_t index = 0; index < types.size(); ++index) {
        CHECK(snapshot->nodes[index + 1].properties.type_id == types[index]);
        CHECK(snapshot->nodes[index + 1].owned_component_ids.empty());
    }
    checkText(snapshot->nodes[2], "Choice");
    bool foundSelected = false;
    for (const auto& property : snapshot->nodes[2].properties.boolean_properties) {
        if (property.property_id == "selected") {
            foundSelected = true;
            CHECK(property.value);
            CHECK(!property.is_default);
        }
    }
    CHECK(foundSelected);
    // The value API exposes only ownership IDs and scalar properties, no binding/membership edges.
}
