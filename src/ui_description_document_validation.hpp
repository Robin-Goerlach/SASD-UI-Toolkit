#pragma once

#include <sasd/ui/ui_description_document.hpp>
#include <sasd/ui/ui_description_json.hpp>

#include <cstddef>

namespace sasd::ui::detail {

enum class UiDescriptionValidationLocation {
    document,
    node,
    reference,
};

struct UiDescriptionValidationResult final {
    UiDescriptionJsonParseErrorCode error{UiDescriptionJsonParseErrorCode::none};
    UiDescriptionValidationLocation location{UiDescriptionValidationLocation::document};
    std::size_t input_index{0};
    ComponentDescriptionNodeId node_id{0};

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == UiDescriptionJsonParseErrorCode::none;
    }
};

/**
 * Revalidates and normalizes the mutable format-neutral document value.
 *
 * The JSON parser maps the reported input position back to its byte offsets. Reconstruction uses
 * the same contract on a private copy, so application mutation can never bypass parser invariants.
 */
[[nodiscard]] UiDescriptionValidationResult
validateAndNormalizeBuiltInUiDescriptionDocument(BuiltInUiDescriptionDocument& document);

} // namespace sasd::ui::detail
