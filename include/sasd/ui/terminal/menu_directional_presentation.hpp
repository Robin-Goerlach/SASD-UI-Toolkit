#pragma once

#include <sasd/ui/terminal/menu_viewport_placement.hpp>

#include <cstddef>
#include <cstdint>

namespace sasd::ui::terminal {

/**
 * Directional chrome for the one submenu row that currently owns an open child popup.
 *
 * One popup level can have at most one direct child open at a time. Keeping only that active row and the
 * already-decided SubmenuPopupSide avoids copying placement state into every semantic menu item while still
 * giving the renderer enough information to draw a direction-sensitive indicator.
 */
struct ActiveSubmenuPresentationDirection {
    std::size_t item_index{0};
    SubmenuPopupSide side{SubmenuPopupSide::right};
};

/**
 * Renders one terminal popup with direction-sensitive chrome for its active submenu row.
 *
 * The ordinary popup renderer remains the source of truth for row geometry, text, shortcuts, selection,
 * disabled styling, clipping and wide-cell handling. This wrapper deliberately does not duplicate that
 * logic. It validates the directional descriptor before any ScreenBuffer mutation, renders the complete
 * popup through renderMenuPopupPresentation(), and only then replaces the already-reserved submenu marker
 * cell with '<' when the child was viewport-fitted to the left. Right-opening children retain '>'.
 *
 * The marker position does not change the popup measurement contract: both '<' and '>' occupy one narrow
 * terminal cell in the same reserved column. The selected/disabled row style is recomputed with the same
 * helper as the base renderer so replacing the glyph cannot accidentally lose inverse/dim presentation.
 *
 * The function fails closed when item_index is out of range or does not identify a submenu. In that case
 * the buffer is untouched. Once descriptor validation and menu measurement succeed, the final glyph patch
 * cannot introduce a partial wide-cell state because both direction markers are single-cell ASCII.
 *
 * This layer intentionally consumes the side decision produced by fitSubmenuPopupToViewport() rather than
 * inferring direction from coordinates. A future overlap or gap policy may make coordinate inference
 * ambiguous, while the explicit side remains a stable presentation fact.
 *
 * @returns true when the popup was representable and rendered; false when the descriptor or popup failed
 *          preflight, with no ScreenBuffer mutation performed before failure.
 */
[[nodiscard]] inline bool
renderDirectionalMenuPopupPresentation(
    ScreenBuffer& buffer,
    Point origin,
    const MenuPopupPresentationSnapshot& snapshot,
    ActiveSubmenuPresentationDirection direction,
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    if (direction.item_index >= snapshot.items.size() ||
        snapshot.items[direction.item_index].kind != MenuItemKind::submenu) {
        return false;
    }

    const auto measured = measureMenuPopupPresentation(snapshot, ambiguous_width);
    if (!measured.has_value()) {
        return false;
    }

    /*
     * Validation above deliberately happens before the base renderer. If a synthetic/stale directional
     * descriptor is inconsistent with the snapshot, callers get the same transactional "no mutation on
     * rejected input" behavior as the rest of the terminal-menu presentation boundary.
     */
    if (!renderMenuPopupPresentation(buffer, origin, snapshot, ambiguous_width)) {
        return false;
    }

    const MenuItemPresentationSnapshot& item = snapshot.items[direction.item_index];
    const bool selected = snapshot.selection == std::optional<std::size_t>{direction.item_index};
    const TextStyle style = detail::menuRowStyle(item, selected);

    const char32_t marker = direction.side == SubmenuPopupSide::left ? U'<' : U'>';
    const std::int64_t marker_x =
        static_cast<std::int64_t>(origin.x) + static_cast<std::int64_t>(measured->size.width) - 2;
    const std::int64_t marker_y =
        static_cast<std::int64_t>(origin.y) + static_cast<std::int64_t>(direction.item_index);

    detail::writeNarrowCell(buffer, marker_x, marker_y, marker, style);
    return true;
}

} // namespace sasd::ui::terminal
