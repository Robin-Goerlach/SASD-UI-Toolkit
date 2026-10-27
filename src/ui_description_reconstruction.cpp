#include <sasd/ui/ui_description_reconstruction.hpp>

#include "ui_description_document_validation.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/component_property_access.hpp>
#include <sasd/ui/component_property_metadata.hpp>
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

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace sasd::ui {
namespace {

using Error = UiDescriptionReconstructionErrorCode;
using Kind = ComponentPropertyValueKind;
using Reference = BuiltInComponentReferenceKind;

BuiltInUiDescriptionReconstructionResult failure(Error error, ComponentDescriptionNodeId node_id) {
    return {nullptr, error, node_id};
}

std::unique_ptr<Component> constructOrdinary(std::string_view type_id) {
    if (type_id == "sasd.ui.Component") return std::make_unique<Component>();
    if (type_id == "sasd.ui.Widget") return std::make_unique<Widget>();
    if (type_id == "sasd.ui.Container") return std::make_unique<Container>();
    if (type_id == "sasd.ui.Window") return std::make_unique<Window>();
    if (type_id == "sasd.ui.Command") return std::make_unique<Command>();
    if (type_id == "sasd.ui.Label") return std::make_unique<Label>();
    if (type_id == "sasd.ui.Button") return std::make_unique<Button>();
    if (type_id == "sasd.ui.TextField") return std::make_unique<TextField>();
    if (type_id == "sasd.ui.CheckBox") return std::make_unique<CheckBox>();
    if (type_id == "sasd.ui.RadioGroup") return std::make_unique<RadioGroup>();
    if (type_id == "sasd.ui.ComboBox") return std::make_unique<ComboBox>();
    if (type_id == "sasd.ui.HBox") return std::make_unique<HBox>();
    if (type_id == "sasd.ui.VBox") return std::make_unique<VBox>();
    if (type_id == "sasd.ui.GridLayout") return std::make_unique<GridLayout>();
    if (type_id == "sasd.ui.FormLayout") return std::make_unique<FormLayout>();
    if (type_id == "sasd.ui.StackLayout") return std::make_unique<StackLayout>();
    if (type_id == "sasd.ui.StringListModel") return std::make_unique<StringListModel>();
    if (type_id == "sasd.ui.ListSelectionModel") return std::make_unique<ListSelectionModel>();
    if (type_id == "sasd.ui.ListView") return std::make_unique<ListView>();
    if (type_id == "sasd.ui.StringTableModel") return std::make_unique<StringTableModel>();
    if (type_id == "sasd.ui.TableSelectionModel") return std::make_unique<TableSelectionModel>();
    if (type_id == "sasd.ui.TableView") return std::make_unique<TableView>();
    if (type_id == "sasd.ui.StringTreeModel") return std::make_unique<StringTreeModel>();
    if (type_id == "sasd.ui.TreeSelectionModel") return std::make_unique<TreeSelectionModel>();
    if (type_id == "sasd.ui.TreeView") return std::make_unique<TreeView>();
    // Abstract model contracts remain known schema types. Substituting an empty String*Model would
    // change the exact persisted type and silently invent runtime semantics.
    return nullptr;
}

const UiDescriptionReferenceEdge* findReference(
    const BuiltInUiDescriptionDocument& document,
    ComponentDescriptionNodeId source,
    Reference kind) {
    const auto found = std::find_if(document.reference_edges.begin(), document.reference_edges.end(),
                                    [&](const auto& edge) {
        return edge.source_node_id == source && edge.kind == kind;
    });
    return found == document.reference_edges.end() ? nullptr : &*found;
}

template<class Properties>
const typename Properties::value_type* findProperty(
    const Properties& properties, const ComponentPropertyMetadata& descriptor) {
    const auto found = std::find_if(properties.begin(), properties.end(), [&](const auto& property) {
        return property.declaring_type_id == descriptor.declaring_type_id &&
               property.property_id == descriptor.property_id;
    });
    return found == properties.end() ? nullptr : &*found;
}

struct RestoreResult final {
    bool restored{false};
    bool representable{true};
};

struct StepResult final {
    Error error{Error::none};
    ComponentDescriptionNodeId node_id{0};

    [[nodiscard]] explicit operator bool() const noexcept { return error == Error::none; }
};

StepResult stepFailure(Error error, ComponentDescriptionNodeId node_id) {
    return {error, node_id};
}

RestoreResult restoreProperty(Component& component,
                              const BuiltInUiDescriptionNode& node,
                              const ComponentPropertyMetadata& descriptor) {
    switch (descriptor.value_kind) {
    case Kind::boolean: {
        const auto* property = findProperty(node.boolean_properties, descriptor);
        return {property != nullptr &&
                    writeBuiltInBooleanProperty(component, descriptor, property->value),
                true};
    }
    case Kind::string: {
        const auto* property = findProperty(node.string_properties, descriptor);
        return {property != nullptr &&
                    writeBuiltInStringProperty(component, descriptor, property->value),
                true};
    }
    case Kind::signed_integer: {
        const auto* property = findProperty(node.signed_integer_properties, descriptor);
        if (property == nullptr) return {};
        const bool restored = writeBuiltInSignedIntegerProperty(component, descriptor, property->value);
        return {restored, restored};
    }
    case Kind::unsigned_integer: {
        const auto* property = findProperty(node.unsigned_integer_properties, descriptor);
        if (property == nullptr) return {};
        const bool restored = writeBuiltInUnsignedIntegerProperty(component, descriptor, property->value);
        return {restored, restored};
    }
    case Kind::string_collection: {
        const auto* property = findProperty(node.string_collection_properties, descriptor);
        return {property != nullptr &&
                    writeBuiltInStringCollectionProperty(component, descriptor, property->value),
                true};
    }
    case Kind::optional_unsigned_integer: {
        const auto* property = findProperty(node.optional_unsigned_integer_properties, descriptor);
        if (property == nullptr) return {};
        const bool restored =
            writeBuiltInOptionalUnsignedIntegerProperty(component, descriptor, property->value);
        return {restored, restored};
    }
    case Kind::size_constraints: {
        const auto* property = findProperty(node.size_constraints_properties, descriptor);
        return {property != nullptr &&
                    writeBuiltInSizeConstraintsProperty(component, descriptor, property->value),
                true};
    }
    case Kind::text_style: {
        const auto* property = findProperty(node.text_style_properties, descriptor);
        return {property != nullptr &&
                    writeBuiltInTextStyleProperty(component, descriptor, property->value),
                true};
    }
    }
    return {};
}

template<class Restore>
RestoreResult forPropertiesBaseToDerived(std::string_view type_id, Restore&& restore) {
    std::vector<const ComponentTypeMetadata*> chain;
    for (const auto* type = findBuiltInComponentType(type_id); type != nullptr;
         type = findBuiltInComponentType(type->base_type_id)) {
        chain.push_back(type);
    }
    for (auto type = chain.rbegin(); type != chain.rend(); ++type) {
        for (const auto& descriptor : declaredBuiltInComponentProperties((*type)->type_id)) {
            const auto result = restore(descriptor);
            if (!result.restored) {
                return result;
            }
        }
    }
    return {true, true};
}

class Reconstructor final {
public:
    explicit Reconstructor(BuiltInUiDescriptionDocument document)
        : document_{std::move(document)},
          instances_(document_.nodes.size()),
          nodes_(document_.nodes.size(), nullptr) {}

    BuiltInUiDescriptionReconstructionResult reconstruct() {
        // Phase 0: the public document is mutable. Normalize a private copy before any live object
        // exists, so a post-parse mutation cannot create UB or escape a partial runtime graph.
        const auto validation =
            detail::validateAndNormalizeBuiltInUiDescriptionDocument(document_);
        if (!validation) {
            return failure(Error::invalid_document, validation.node_id);
        }

        if (auto result = constructComponents(); !result) return failure(result.error, result.node_id);
        if (auto result = restoreCommandProperties(); !result) return failure(result.error, result.node_id);
        if (auto result = restoreReferences(); !result) return failure(result.error, result.node_id);
        if (auto result = restoreAllProperties(); !result) return failure(result.error, result.node_id);
        if (auto result = adoptOwnership(); !result) return failure(result.error, result.node_id);
        return transferVerifiedRoot();
    }

private:
    BuiltInUiDescriptionDocument document_;
    std::vector<std::unique_ptr<Component>> instances_;
    std::vector<Component*> nodes_;

    StepResult constructComponents() {
        // Groups and all other targets must exist before RadioButton's constructor-only membership
        // relation is established. No setGroup(), friend access or post-construction injection exists.
        for (const auto& node : document_.nodes) {
            if (node.type_id == "sasd.ui.RadioButton") continue;
            auto component = constructOrdinary(node.type_id);
            if (!component) {
                return stepFailure(Error::unconstructible_component_type, node.node_id);
            }
            store(node.node_id, std::move(component));
        }
        for (const auto& node : document_.nodes) {
            if (node.type_id != "sasd.ui.RadioButton") continue;
            std::unique_ptr<Component> component;
            if (const auto* edge = findReference(
                    document_, node.node_id, Reference::radio_button_group)) {
                auto* group = dynamic_cast<RadioGroup*>(nodes_[index(edge->target_node_id)]);
                if (group == nullptr) {
                    return stepFailure(Error::reference_restore_failed, node.node_id);
                }
                component = std::make_unique<RadioButton>(*group, std::string{});
            } else {
                component = std::make_unique<RadioButton>();
            }
            store(node.node_id, std::move(component));
        }
        return {};
    }

    StepResult restoreCommandProperties() {
        // bindCommand() snapshots Command text/enabled immediately. Establish the persisted target
        // state first; the final all-property pass later restores the Button's independent snapshot.
        for (const auto& node : document_.nodes) {
            if (node.type_id != "sasd.ui.Command") continue;
            if (auto result = restoreNodeProperties(node); !result) return result;
        }
        return {};
    }

    StepResult restoreReferences() {
        // This semantic order is independent of serialized edge order. Selection models observe
        // their models before views observe either side; Button binding is deliberately last.
        constexpr std::array order{
            Reference::list_selection_model_model,
            Reference::table_selection_model_model,
            Reference::tree_selection_model_model,
            Reference::list_view_model,
            Reference::table_view_model,
            Reference::tree_view_model,
            Reference::list_view_selection_model,
            Reference::table_view_selection_model,
            Reference::tree_view_selection_model,
            Reference::button_command,
        };
        for (const auto kind : order) {
            for (const auto& edge : document_.reference_edges) {
                if (edge.kind != kind) continue;
                if (!restoreReference(edge)) {
                    return stepFailure(Error::reference_restore_failed, edge.source_node_id);
                }
            }
        }
        return {};
    }

    bool restoreReference(const UiDescriptionReferenceEdge& edge) {
        Component* source = nodes_[index(edge.source_node_id)];
        Component* target = nodes_[index(edge.target_node_id)];
        switch (edge.kind) {
        case Reference::button_command: {
            auto* typed_source = dynamic_cast<Button*>(source);
            auto* typed_target = dynamic_cast<Command*>(target);
            if (!typed_source || !typed_target) return false;
            typed_source->bindCommand(*typed_target);
            return true;
        }
        case Reference::radio_button_group:
            // Membership was committed by the only public API that can establish it: construction.
            return dynamic_cast<RadioButton*>(source)->group() == dynamic_cast<RadioGroup*>(target);
        case Reference::list_selection_model_model: return setModel<ListSelectionModel, ListModel>(source, target);
        case Reference::table_selection_model_model: return setModel<TableSelectionModel, TableModel>(source, target);
        case Reference::tree_selection_model_model: return setModel<TreeSelectionModel, TreeModel>(source, target);
        case Reference::list_view_model: return setModel<ListView, ListModel>(source, target);
        case Reference::table_view_model: return setModel<TableView, TableModel>(source, target);
        case Reference::tree_view_model: return setModel<TreeView, TreeModel>(source, target);
        case Reference::list_view_selection_model: return setSelectionModel<ListView, ListSelectionModel>(source, target);
        case Reference::table_view_selection_model: return setSelectionModel<TableView, TableSelectionModel>(source, target);
        case Reference::tree_view_selection_model: return setSelectionModel<TreeView, TreeSelectionModel>(source, target);
        }
        return false;
    }

    template<class Source, class Target>
    static bool setModel(Component* source, Component* target) {
        auto* typed_source = dynamic_cast<Source*>(source);
        auto* typed_target = dynamic_cast<Target*>(target);
        if (!typed_source || !typed_target) return false;
        typed_source->setModel(typed_target);
        return typed_source->model() == typed_target;
    }

    template<class Source, class Target>
    static bool setSelectionModel(Component* source, Component* target) {
        auto* typed_source = dynamic_cast<Source*>(source);
        auto* typed_target = dynamic_cast<Target*>(target);
        if (!typed_source || !typed_target) return false;
        typed_source->setSelectionModel(typed_target);
        return typed_source->selectionModel() == typed_target;
    }

    StepResult restoreAllProperties() {
        // Fresh controls have no application callbacks. Normal semantic setters and subscriptions
        // are therefore the correct transaction; a global restore/silent-setter mode is unwarranted.
        for (const auto& node : document_.nodes) {
            if (auto result = restoreNodeProperties(node); !result) return result;
        }
        return {};
    }

    StepResult restoreNodeProperties(
        const BuiltInUiDescriptionNode& node) {
        try {
            const auto restored = forPropertiesBaseToDerived(node.type_id, [&](const auto& descriptor) {
                return restoreProperty(*nodes_[index(node.node_id)], node, descriptor);
            });
            if (!restored.restored) {
                return stepFailure(restored.representable ? Error::property_restore_failed
                                                          : Error::value_not_representable,
                                   node.node_id);
            }
        } catch (const std::invalid_argument&) {
            return stepFailure(Error::property_restore_failed, node.node_id);
        } catch (const std::out_of_range&) {
            return stepFailure(Error::property_restore_failed, node.node_id);
        }
        return {};
    }

    StepResult adoptOwnership() {
        // Keep every object flat and independently owned until semantic reconstruction succeeds.
        // Moving a unique_ptr never moves the Component, so raw lookup addresses remain stable as
        // nested containers adopt their children in the exact persisted order.
        try {
            for (const auto& node : document_.nodes) {
                auto* owner = dynamic_cast<Container*>(nodes_[index(node.node_id)]);
                if (!node.owned_component_ids.empty() && owner == nullptr) {
                    return stepFailure(Error::ownership_restore_failed, node.node_id);
                }
                for (const auto child_id : node.owned_component_ids) {
                    auto& child = instances_[index(child_id)];
                    if (!child) return stepFailure(Error::ownership_restore_failed, child_id);
                    owner->adopt(std::move(child));
                }
            }
        } catch (const std::invalid_argument&) {
            return stepFailure(Error::ownership_restore_failed, 0);
        }
        return {};
    }

    BuiltInUiDescriptionReconstructionResult transferVerifiedRoot() {
        const auto root_index = index(document_.root_node_id);
        if (!instances_[root_index] || instances_[root_index].get() != nodes_[root_index] ||
            nodes_[root_index]->owner() != nullptr) {
            return failure(Error::ownership_restore_failed, document_.root_node_id);
        }
        for (std::size_t i = 0; i < instances_.size(); ++i) {
            if (i != root_index && instances_[i]) {
                return failure(Error::ownership_restore_failed,
                               static_cast<ComponentDescriptionNodeId>(i));
            }
            const auto* exact_type = builtInComponentTypeOf(*nodes_[i]);
            if (exact_type == nullptr || exact_type->type_id != document_.nodes[i].type_id) {
                return failure(Error::unconstructible_component_type,
                               static_cast<ComponentDescriptionNodeId>(i));
            }
        }
        for (const auto& node : document_.nodes) {
            auto* owner = dynamic_cast<Container*>(nodes_[index(node.node_id)]);
            for (std::size_t child_index = 0; child_index < node.owned_component_ids.size(); ++child_index) {
                const auto child_id = node.owned_component_ids[child_index];
                Component* child = nodes_[index(child_id)];
                if (owner == nullptr || child->owner() != owner ||
                    &owner->componentAt(child_index) != child) {
                    return failure(Error::ownership_restore_failed, child_id);
                }
                if (auto* widget = dynamic_cast<Widget*>(child); widget && widget->parent() != owner) {
                    return failure(Error::ownership_restore_failed, child_id);
                }
            }
        }
        if (!referencesMatch()) {
            return failure(Error::reference_restore_failed, 0);
        }
        auto root = std::move(instances_[root_index]);
        return {std::move(root), Error::none, document_.root_node_id};
    }

    bool referencesMatch() const {
        std::vector<std::size_t> group_members(nodes_.size(), 0);
        for (const auto& edge : document_.reference_edges) {
            const Component* source = nodes_[index(edge.source_node_id)];
            const Component* target = nodes_[index(edge.target_node_id)];
            bool matches = false;
            switch (edge.kind) {
            case Reference::button_command: matches = dynamic_cast<const Button*>(source)->boundCommand() == target; break;
            case Reference::radio_button_group:
                matches = dynamic_cast<const RadioButton*>(source)->group() == target;
                ++group_members[index(edge.target_node_id)];
                break;
            case Reference::list_view_model: matches = dynamic_cast<const ListView*>(source)->model() == target; break;
            case Reference::table_view_model: matches = dynamic_cast<const TableView*>(source)->model() == target; break;
            case Reference::tree_view_model: matches = dynamic_cast<const TreeView*>(source)->model() == target; break;
            case Reference::list_view_selection_model: matches = dynamic_cast<const ListView*>(source)->selectionModel() == target; break;
            case Reference::table_view_selection_model: matches = dynamic_cast<const TableView*>(source)->selectionModel() == target; break;
            case Reference::tree_view_selection_model: matches = dynamic_cast<const TreeView*>(source)->selectionModel() == target; break;
            case Reference::list_selection_model_model: matches = dynamic_cast<const ListSelectionModel*>(source)->model() == target; break;
            case Reference::table_selection_model_model: matches = dynamic_cast<const TableSelectionModel*>(source)->model() == target; break;
            case Reference::tree_selection_model_model: matches = dynamic_cast<const TreeSelectionModel*>(source)->model() == target; break;
            }
            if (!matches) return false;
        }
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            if (const auto* group = dynamic_cast<const RadioGroup*>(nodes_[i]);
                group && group->memberCount() != group_members[i]) {
                return false;
            }
            if (const auto* radio = dynamic_cast<const RadioButton*>(nodes_[i]);
                radio && radio->group() != nullptr &&
                ((radio->isSelected() && radio->group()->selectedButton() != radio) ||
                 (!radio->isSelected() && radio->group()->selectedButton() == radio))) {
                return false;
            }
        }
        return true;
    }

    static std::size_t index(ComponentDescriptionNodeId node_id) {
        return static_cast<std::size_t>(node_id);
    }

    void store(ComponentDescriptionNodeId node_id, std::unique_ptr<Component> component) {
        const auto node_index = index(node_id);
        nodes_[node_index] = component.get();
        instances_[node_index] = std::move(component);
    }
};

} // namespace

BuiltInUiDescriptionReconstructionResult
reconstructBuiltInUiDescription(const BuiltInUiDescriptionDocument& document) {
    return Reconstructor{document}.reconstruct();
}

} // namespace sasd::ui
