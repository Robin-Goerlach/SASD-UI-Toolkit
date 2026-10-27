#pragma once

#include <sasd/ui/component.hpp>
#include <sasd/ui/ui_description_document.hpp>

#include <memory>

namespace sasd::ui {

/** Stable failure classes for format-neutral built-in reconstruction. */
enum class UiDescriptionReconstructionErrorCode {
    none,
    invalid_document,
    unconstructible_component_type,
    value_not_representable,
    property_restore_failed,
    reference_restore_failed,
    ownership_restore_failed,
};

struct BuiltInUiDescriptionReconstructionResult final {
    std::unique_ptr<Component> root;
    UiDescriptionReconstructionErrorCode error{UiDescriptionReconstructionErrorCode::none};
    ComponentDescriptionNodeId node_id{0};

    [[nodiscard]] explicit operator bool() const noexcept {
        return root != nullptr;
    }
};

/**
 * Reconstructs one complete live built-in Component ownership graph from an already decoded value.
 *
 * The mutable aggregate is revalidated before any Component is constructed. All runtime objects are
 * staged under unique ownership until properties and references have been restored; ownership is
 * adopted only afterwards. A failure therefore returns no partial graph. Allocation and other
 * system failures propagate, while document/domain failures are reported through the result.
 * Description node IDs are temporary lookup identity and are never stored in Components.
 */
[[nodiscard]] BuiltInUiDescriptionReconstructionResult
reconstructBuiltInUiDescription(const BuiltInUiDescriptionDocument& document);

} // namespace sasd::ui
