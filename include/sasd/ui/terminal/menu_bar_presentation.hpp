#pragma once

#include <sasd/ui/terminal/menu_presentation.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace sasd::ui::terminal {

/**
 * Paints one owned menu-bar snapshot into ScreenBuffer.
 *
 * This renderer deliberately consumes the same presentation snapshot and the same measurement contract
 * as popup rendering, but stays independent from ANSI/VT emission and terminal-device state. The complete
 * snapshot is validated before the first cell is changed, so unsupported Unicode cannot partially erase
 * a previously synchronized frame.
 *
 * Each top-level title owns exactly the two padding cells already accounted for by
 * measureMenuBarPresentation(): one before the title and one after it. If the snapshot carries a valid
 * selected index, inverse video covers that title's complete padded span. This makes selection geometry
 * deterministic without introducing presentation state into MenuBarModel.
 *
 * Rendering clips naturally at ScreenBuffer edges. Wide glyphs still obey detail::writeScalar(), so a
 * partially clipped two-cell glyph is omitted rather than leaving an orphaned lead/continuation cell.
 *
 * @returns true when the snapshot is representable and painting was attempted; false when preflight
 *          failed and the buffer was left unchanged.
 */
[[nodiscard]] inline bool
renderMenuBarPresentation(ScreenBuffer& buffer,
                          Point origin,
                          const MenuBarPresentationSnapshot& snapshot,
                          AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) noexcept {
    const auto measured = measureMenuBarPresentation(snapshot, ambiguous_width);
    if (!measured.has_value()) {
        return false;
    }

    const std::int64_t origin_x = origin.x;
    const std::int64_t origin_y = origin.y;
    Coordinate logical_x = 0;

    for (std::size_t index = 0; index < snapshot.titles.size(); ++index) {
        const std::string& title = snapshot.titles[index];
        const auto title_width = detail::measureMenuLine(title, ambiguous_width);
        if (!title_width.has_value()) {
            // The complete snapshot was already preflighted above; keep this as a defensive fail-closed guard.
            return false;
        }

        const bool selected = snapshot.selection == std::optional<std::size_t>{index};
        TextStyle style;
        style.inverse = selected;

        /*
         * Fill the complete padded title span before drawing glyphs. This ensures inverse selection also
         * covers whitespace and overwrites stale cells from a previous title of equal or smaller width.
         */
        const Coordinate span_width = *title_width + 2;
        for (Coordinate x = 0; x < span_width; ++x) {
            detail::writeNarrowCell(buffer,
                                    origin_x + logical_x + x,
                                    origin_y,
                                    U' ',
                                    style);
        }

        detail::writeMenuLine(buffer,
                              origin_x + logical_x + 1,
                              origin_y,
                              title,
                              ambiguous_width,
                              style);

        logical_x += span_width;
    }

    return true;
}

} // namespace sasd::ui::terminal
