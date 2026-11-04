#include <sasd/ui/component_property_resource_metadata.hpp>

#include <array>
#include <string_view>

namespace sasd::ui {
namespace {

struct ResourceMetadataEntry final {
    std::string_view declaring_type_id;
    std::string_view property_id;
    ComponentPropertyValueKind value_kind;
    ResourceKind resource_kind;
};

// Resource capability is an explicit authoring policy, not a property-name or value-kind heuristic.
// Keeping the complete descriptor identity in every entry makes accidental opt-in impossible when
// another text_style property is added. Inherited properties would retain their declaring entry
// rather than receiving derived-type duplicates, matching the property catalog's identity model.
constexpr std::array<ResourceMetadataEntry, 6> resource_metadata_entries{{
    {"sasd.ui.Label", "textStyle", ComponentPropertyValueKind::text_style,
     ResourceKind::text_style},
    {"sasd.ui.Button", "textStyle", ComponentPropertyValueKind::text_style,
     ResourceKind::text_style},
    {"sasd.ui.TextField", "textStyle", ComponentPropertyValueKind::text_style,
     ResourceKind::text_style},
    {"sasd.ui.CheckBox", "textStyle", ComponentPropertyValueKind::text_style,
     ResourceKind::text_style},
    {"sasd.ui.RadioButton", "textStyle", ComponentPropertyValueKind::text_style,
     ResourceKind::text_style},
    {"sasd.ui.ComboBox", "textStyle", ComponentPropertyValueKind::text_style,
     ResourceKind::text_style},
}};

} // namespace

std::optional<ComponentPropertyResourceMetadata>
builtInComponentPropertyResourceMetadata(
    const ComponentPropertyMetadata& property) noexcept {
    for (const auto& entry : resource_metadata_entries) {
        if (entry.declaring_type_id == property.declaring_type_id &&
            entry.property_id == property.property_id &&
            entry.value_kind == property.value_kind) {
            return ComponentPropertyResourceMetadata{entry.resource_kind};
        }
    }

    return std::nullopt;
}

} // namespace sasd::ui
