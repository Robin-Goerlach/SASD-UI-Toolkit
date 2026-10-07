#pragma once

#include <sasd/ui/presentation/anchored_popup_layout.hpp>
#include <sasd/ui/terminal/combo_box_popup_presentation.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace sasd::ui::terminal {

/**
 * Hit-tests terminal cells against an already-built ComboBox popup presentation snapshot.
 *
 * The helper consumes the *same final row geometry* that rendering uses. It never re-measures text,
 * re-runs popup placement, walks the Widget tree, or asks ComboBox for semantic state. This keeps one
 * important invariant explicit: if a row is paintable at a terminal rectangle, pointer identity is
 * derived from that rectangle rather than from a second approximation of layout.
 *
 * Snapshot validity is conservative. A non-empty popup must expose exactly one terminal row per owned
 * item. Empty item collections are valid only with empty bounds and cannot produce a hit. Malformed or
 * stale synthetic geometry returns std::nullopt instead of manufacturing a row index.
 */
class TerminalComboBoxPopupHitTest final {
public:
    TerminalComboBoxPopupHitTest() = delete;

    /**
     * Returns the semantic item-row index owning point, or std::nullopt when point is outside the popup
     * or the snapshot's row geometry is inconsistent.
     *
     * Rect uses half-open bounds, so the popup's right/bottom edges do not belong to any row. The index
     * calculation is intentionally followed by fixedPopupRowBounds() validation rather than trusting
     * subtraction alone; rendering and hit testing therefore share ADR 0122's row geometry contract.
     */
    [[nodiscard]] static std::optional<std::size_t>
    rowIndexAt(const ComboBoxPopupPresentationSnapshot& snapshot,
               Point point) noexcept {
        if (snapshot.items.empty()) {
            return std::nullopt;
        }

        if (snapshot.bounds.isEmpty() ||
            snapshot.items.size() >
                static_cast<std::size_t>(std::numeric_limits<Coordinate>::max()) ||
            snapshot.bounds.height != static_cast<Coordinate>(snapshot.items.size()) ||
            !snapshot.bounds.contains(point)) {
            return std::nullopt;
        }

        /*
         * contains() proved point.y is in [bounds.y, bounds.y + height), so widened subtraction is
         * non-negative and strictly smaller than the item count. Keep the arithmetic widened anyway:
         * snapshots may legitimately sit near Coordinate limits and signed int32 subtraction must not
         * become a hidden overflow source.
         */
        const auto row_offset =
            static_cast<std::int64_t>(point.y) -
            static_cast<std::int64_t>(snapshot.bounds.y);
        const auto row_index = static_cast<std::size_t>(row_offset);

        const auto row = presentation::fixedPopupRowBounds(
            snapshot.bounds,
            snapshot.items.size(),
            row_index,
            1);
        if (!row.has_value() || !row->contains(point)) {
            return std::nullopt;
        }

        return row_index;
    }
};

} // namespace sasd::ui::terminal
