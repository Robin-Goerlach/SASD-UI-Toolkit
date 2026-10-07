#pragma once

#include <sasd/ui/presentation/anchored_popup_layout.hpp>
#include <sasd/ui/rendered/combo_box_popup_presentation.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace sasd::ui::rendered {

/**
 * Hit-tests logical pointer coordinates against an already-built rendered ComboBox popup snapshot.
 *
 * The helper consumes the exact final content rectangle and fixed row height used by rendering. It
 * deliberately does not remeasure text, repeat popup placement or inspect ComboBox state. Semantic
 * freshness is a separate concern of RenderedComboBoxPopupPointerInteraction.
 */
class RenderedComboBoxPopupHitTest final {
public:
    RenderedComboBoxPopupHitTest() = delete;

    /**
     * Returns the painted item-row index at point, or std::nullopt outside rows/malformed geometry.
     *
     * Border pixels are intentionally not rows. Rect's half-open right and bottom edges are outside,
     * and fixedPopupRowBounds() performs the same complete-row validation used by presentation.
     */
    [[nodiscard]] static std::optional<std::size_t>
    rowIndexAt(const RenderedComboBoxPopupPresentationSnapshot& snapshot,
               Point point) noexcept {
        if (snapshot.items.empty() ||
            snapshot.content_bounds.isEmpty() ||
            snapshot.row_height <= 0 ||
            !snapshot.content_bounds.contains(point)) {
            return std::nullopt;
        }

        /*
         * contains() proves the widened difference is non-negative. Division happens in 64-bit
         * arithmetic so a popup positioned near Coordinate's limits cannot overflow subtraction.
         */
        const auto row_offset =
            static_cast<std::int64_t>(point.y) -
            static_cast<std::int64_t>(snapshot.content_bounds.y);
        const auto row_index = static_cast<std::size_t>(
            row_offset / static_cast<std::int64_t>(snapshot.row_height));

        const auto row = presentation::fixedPopupRowBounds(
            snapshot.content_bounds,
            snapshot.items.size(),
            row_index,
            snapshot.row_height);
        if (!row.has_value() || !row->contains(point)) {
            return std::nullopt;
        }

        return row_index;
    }
};

} // namespace sasd::ui::rendered
