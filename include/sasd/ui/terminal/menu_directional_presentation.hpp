#pragma once

#include <sasd/ui/terminal/menu_direction.hpp>
#include <sasd/ui/terminal/menu_presentation.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace sasd::ui::terminal {

/**
 * Checks whether one directional descriptor is structurally consistent with the popup snapshot it
 * annotates.
 *
 * The descriptor is presentation-only state. It is valid exactly when its item index names a submenu row;
 * commands and separators cannot own an open child popup. Keeping this validation in the directional
 * presentation layer lets both standalone rendering and whole-frame preflight share the same rule without
 * duplicating it or moving terminal-only state into MenuModel.
 */
[[nodiscard]] inline bool
isValidActiveSubmenuPresentationDirection(
    const MenuPopupPresentationSnapshot& snapshot,
    ActiveSubmenuPresentationDirection direction) noexcept {
    return direction.item_index < snapshot.items.size() &&
           snapshot.items[direction.item_index].kind == MenuItemKind::submenu;
}

/**
 * Renders one terminal popup with direction-sensitive chrome for its active submenu row.
 *
 * The ordinary popup renderer remains the source of truth for row geometry, text, shortcuts, selection,
 * disabled styling, clipping and wide-cell handling. This wrapper deliberately does not duplicate that
 * logic. It validates the directional descriptor before any ScreenBuffer mutation, renders the complete
 * popup through renderMenuPopupPresentation(), and only then replaces the already-reserved submenu marker
 * cell with '<' when the child opens to the left. Right-opening children retain '>'.
 *
 * The marker position does not change the popup measurement contract: both '<' and '>' occupy one narrow
 * terminal cell in the same reserved column. The selected/disabled row style is recomputed with the same
 * helper as the base renderer so replacing the glyph cannot accidentally lose inverse/dim presentation.
 *
 * The function fails closed when item_index is out of range or does not identify a submenu. In that case
 * the buffer is untouched. Once descriptor validation and menu measurement succeed, the final glyph patch
 * cannot introduce a partial wide-cell state because both direction markers are single-cell ASCII.
 *
 * This layer consumes an explicit side decision rather than inferring direction from coordinates. A future
 * overlap or gap policy may make coordinate inference ambiguous, while the explicit side remains stable
 * presentation data.
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
    if (!isValidActiveSubmenuPresentationDirection(snapshot, direction)) {
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
