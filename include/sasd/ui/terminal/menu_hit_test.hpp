#pragma once

#include <sasd/ui/terminal/menu_frame_presentation.hpp>
#include <sasd/ui/terminal/menu_presentation.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace sasd::ui::terminal {

/** Identifies one popup row inside the complete terminal menu frame. */
struct TerminalMenuPopupHit {
    std::size_t level{0};
    std::size_t item_index{0};

    friend constexpr bool operator==(const TerminalMenuPopupHit&,
                                     const TerminalMenuPopupHit&) = default;
};

/**
 * Read-only terminal-cell hit testing for an already-built menu presentation frame.
 *
 * This class deliberately consumes MenuFramePresentationSnapshot rather than MenuBarModel or
 * MenuInteractionController. Placement has already decided where every popup is painted, including
 * viewport fitting and left/right submenu flipping, so pointer geometry must interrogate that same owned
 * presentation transaction instead of re-deriving a second set of rectangles from semantic menu state.
 *
 * The helper owns no interaction state and performs no menu action. In particular it does not select a
 * menu, open a popup, activate a Command, close a menu, or retain model pointers. A later interaction layer
 * can translate the returned presentation identity into semantic controller operations while keeping this
 * terminal-specific cell geometry isolated from Core menu semantics.
 */
class TerminalMenuHitTest final {
public:
    TerminalMenuHitTest() = delete;

    /**
     * Returns the top-level title whose complete padded terminal span contains point.
     *
     * The renderer gives every title one leading and one trailing padding cell. Those cells are part of
     * the visible selectable title surface, so hit testing intentionally includes them. Unicode cell width
     * is measured with the same primitive and AmbiguousWidthMode used by terminal menu presentation.
     *
     * The complete menu-bar snapshot is preflighted first. If any title is not representable by the current
     * simple Cell model, the function fails closed with std::nullopt rather than returning geometry from a
     * bar that renderMenuPresentationFrame() would reject transactionally.
     */
    [[nodiscard]] static std::optional<std::size_t>
    menuBarIndexAt(const MenuFramePresentationSnapshot& frame,
                   Point point,
                   AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) noexcept {
        if (!measureMenuBarPresentation(frame.menu_bar, ambiguous_width).has_value()) {
            return std::nullopt;
        }

        if (point.y != frame.menu_bar_origin.y) {
            return std::nullopt;
        }

        const std::int64_t pointer_x = point.x;
        std::int64_t logical_x = frame.menu_bar_origin.x;

        for (std::size_t index = 0; index < frame.menu_bar.titles.size(); ++index) {
            /*
             * Use the renderer's exact single-line measurement primitive. Reusing this internal helper is
             * intentional inside the terminal presentation layer: introducing a second Unicode-width rule
             * here would eventually let painting and pointer hit geometry disagree.
             */
            const auto title_width =
                detail::measureMenuLine(frame.menu_bar.titles[index], ambiguous_width);
            if (!title_width.has_value()) {
                return std::nullopt;
            }

            const std::int64_t span_width = static_cast<std::int64_t>(*title_width) + 2;
            const std::int64_t span_end = logical_x + span_width;

            if (pointer_x >= logical_x && pointer_x < span_end) {
                return index;
            }

            logical_x = span_end;
        }

        return std::nullopt;
    }

    /**
     * Returns the topmost popup row containing point.
     *
     * Popup layers are searched in reverse frame order because MenuFramePresentationSnapshot explicitly
     * stores them in paint order: later popups are drawn above earlier ones. This matters for fitted nested
     * menus where rectangles can touch or, in synthetic/testing frames, overlap. Pointer identity must match
     * what is visually on top rather than whichever semantic menu happened to be inspected first.
     *
     * A row remains a geometric hit even when its snapshot describes a separator or disabled command. That
     * distinction belongs to the later semantic interaction policy: this helper answers only which painted
     * row occupies the cell. Keeping unavailable-item policy out of geometry prevents presentation hit
     * testing from becoming a second MenuInteractionController.
     *
     * All popup snapshots are preflighted before any hit is returned. A directly-constructed frame with one
     * unrepresentable layer is therefore rejected as a whole, matching the transactional frame renderer.
     */
    [[nodiscard]] static std::optional<TerminalMenuPopupHit>
    popupItemAt(const MenuFramePresentationSnapshot& frame,
                Point point,
                AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        if (!measureMenuBarPresentation(frame.menu_bar, ambiguous_width).has_value()) {
            return std::nullopt;
        }

        for (const PositionedMenuPopupPresentationSnapshot& popup : frame.popups) {
            if (!measureMenuPopupPresentation(popup.snapshot, ambiguous_width).has_value()) {
                return std::nullopt;
            }
        }

        const std::int64_t pointer_x = point.x;
        const std::int64_t pointer_y = point.y;

        /*
         * Re-measuring during the reverse pass is deliberately accepted for now. It keeps this helper
         * allocation-free apart from measurement's existing shortcut formatting and avoids introducing
         * cached geometry/lifetime state solely for a micro-optimization. A later profiling pass can reuse
         * measurements without changing the public semantics.
         */
        for (std::size_t level = frame.popups.size(); level > 0U; --level) {
            const std::size_t index = level - 1U;
            const PositionedMenuPopupPresentationSnapshot& popup = frame.popups[index];
            const auto measured = measureMenuPopupPresentation(popup.snapshot, ambiguous_width);
            if (!measured.has_value()) {
                return std::nullopt;
            }

            const std::int64_t left = popup.origin.x;
            const std::int64_t top = popup.origin.y;
            const std::int64_t right = left + static_cast<std::int64_t>(measured->size.width);
            const std::int64_t bottom = top + static_cast<std::int64_t>(measured->size.height);

            if (pointer_x < left || pointer_x >= right ||
                pointer_y < top || pointer_y >= bottom) {
                continue;
            }

            const std::int64_t row = pointer_y - top;
            if (row < 0 ||
                row >= static_cast<std::int64_t>(popup.snapshot.items.size())) {
                /*
                 * Measurement height is defined by items.size(), so this is defensive only. Fail closed
                 * rather than manufacturing an item identity if that invariant ever changes.
                 */
                return std::nullopt;
            }

            return TerminalMenuPopupHit{
                index,
                static_cast<std::size_t>(row),
            };
        }

        return std::nullopt;
    }
};

} // namespace sasd::ui::terminal
