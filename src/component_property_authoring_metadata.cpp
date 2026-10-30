#include <sasd/ui/component_property_authoring_metadata.hpp>

#include <array>
#include <string_view>

namespace sasd::ui {
namespace {

struct AuthoringEntry final {
    std::string_view declaring_type_id;
    std::string_view property_id;
    ComponentPropertyAuthoringCategory category;
};

// Categories are intentionally explicit rather than derived from names. A future property such as
// "contentAlignment" must make an authoring decision instead of accidentally inheriting a category
// because its spelling happens to contain "content". Keeping this private also avoids creating a
// second public property catalog: declaredBuiltInComponentProperties() remains the enumeration
// source, while this table is only the exhaustive policy applied to those schema descriptors.
constexpr std::array<AuthoringEntry, 28> authoring_entries{{
    {"sasd.ui.Widget", "visible", ComponentPropertyAuthoringCategory::behavior},
    {"sasd.ui.Widget", "enabled", ComponentPropertyAuthoringCategory::behavior},
    {"sasd.ui.Widget", "focusable", ComponentPropertyAuthoringCategory::behavior},
    {"sasd.ui.Widget", "sizeConstraints", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.Command", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.Command", "enabled", ComponentPropertyAuthoringCategory::behavior},
    {"sasd.ui.Label", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.Label", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.Button", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.Button", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.TextField", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.TextField", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.CheckBox", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.CheckBox", "checked", ComponentPropertyAuthoringCategory::state},
    {"sasd.ui.CheckBox", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.RadioButton", "text", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.RadioButton", "selected", ComponentPropertyAuthoringCategory::state},
    {"sasd.ui.RadioButton", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.ComboBox", "items", ComponentPropertyAuthoringCategory::content},
    {"sasd.ui.ComboBox", "selectedIndex", ComponentPropertyAuthoringCategory::state},
    {"sasd.ui.ComboBox", "textStyle", ComponentPropertyAuthoringCategory::appearance},
    {"sasd.ui.HBox", "spacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.VBox", "spacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.GridLayout", "columnCount", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.GridLayout", "columnSpacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.GridLayout", "rowSpacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.FormLayout", "columnSpacing", ComponentPropertyAuthoringCategory::layout},
    {"sasd.ui.FormLayout", "rowSpacing", ComponentPropertyAuthoringCategory::layout},
}};

bool isExactBuiltInProperty(const ComponentPropertyMetadata& property) noexcept {
    // Resolve through the existing schema first and then compare the complete descriptor by value.
    // This rejects a fabricated kind and also rejects a derived-type alias such as Button.visible:
    // inherited metadata belongs to the actual Widget.visible declaring descriptor.
    const auto* resolved =
        findBuiltInComponentProperty(property.declaring_type_id, property.property_id);
    return resolved != nullptr && resolved->declaring_type_id == property.declaring_type_id &&
           resolved->property_id == property.property_id &&
           resolved->value_kind == property.value_kind;
}

} // namespace

std::optional<ComponentPropertyAuthoringMetadata>
builtInComponentPropertyAuthoringMetadata(
    const ComponentPropertyMetadata& property) noexcept {
    if (!isExactBuiltInProperty(property)) {
        return std::nullopt;
    }

    for (const auto& entry : authoring_entries) {
        if (entry.declaring_type_id == property.declaring_type_id &&
            entry.property_id == property.property_id) {
            return ComponentPropertyAuthoringMetadata{entry.category};
        }
    }

    // There is deliberately no default category. Complete-catalog tests make a newly added schema
    // property fail until its authoring meaning is chosen explicitly.
    return std::nullopt;
}

} // namespace sasd::ui
