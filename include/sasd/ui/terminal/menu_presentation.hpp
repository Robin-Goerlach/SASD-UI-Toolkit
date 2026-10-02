#pragma once

#include <sasd/ui/menu_interaction_view.hpp>
#include <sasd/ui/shortcut_display.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/text_metrics.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace sasd::ui::terminal {

/**
 * Terminal-cell measurement of one owned menu-presentation snapshot.
 *
 * This layer deliberately keeps geometry and painting together at the terminal-presentation boundary,
 * while remaining completely independent from ANSI/VT output. A caller can therefore measure and render
 * menu snapshots deterministically into ScreenBuffer on every CI platform before any device/session layer
 * is involved.
 */
struct MenuPresentationSize {
    Size size{};
};

namespace detail {

[[nodiscard]] inline std::optional<Coordinate>
measureMenuLine(std::string_view text, AmbiguousWidthMode ambiguous_width) noexcept {
    const TextMeasurement measured = TextMetrics::measureUtf8(text, ambiguous_width);

    /*
     * Menu labels are single-line presentation atoms. Newlines would make one semantic item occupy
     * multiple terminal rows, and zero-width/control semantics cannot yet be represented faithfully by
     * the simple Cell model. Refuse those cases instead of silently producing misleading geometry.
     */
    if (measured.rows != 1 || !measured.simpleCellRenderable() || measured.saturated) {
        return std::nullopt;
    }

    return measured.columns;
}

[[nodiscard]] inline bool checkedAdd(Coordinate value,
                                     Coordinate addition,
                                     Coordinate& result) noexcept {
    const auto sum = static_cast<std::int64_t>(value) + static_cast<std::int64_t>(addition);
    if (sum > static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max())) {
        return false;
    }
    result = static_cast<Coordinate>(sum);
    return true;
}

[[nodiscard]] inline bool isInsideBuffer(const ScreenBuffer& buffer,
                                         std::int64_t x,
                                         std::int64_t y) noexcept {
    return x >= 0 && y >= 0 &&
           x < static_cast<std::int64_t>(buffer.size().width) &&
           y < static_cast<std::int64_t>(buffer.size().height);
}

inline void writeNarrowCell(ScreenBuffer& buffer,
                            std::int64_t x,
                            std::int64_t y,
                            char32_t value,
                            TextStyle style = {}) {
    if (!isInsideBuffer(buffer, x, y)) {
        return;
    }

    buffer.set({static_cast<Coordinate>(x), static_cast<Coordinate>(y)},
               Cell{value, CellRole::normal, style});
}

/**
 * Writes one measured Unicode scalar without ever leaving half of a two-cell glyph in ScreenBuffer.
 *
 * Clipping a narrow scalar is harmless. A wide scalar is stricter: both cells must be inside the buffer
 * before either cell is written. This mirrors the existing widget renderer and prevents a terminal diff
 * encoder from later seeing an orphaned wide-lead or wide-continuation cell at a clipped edge.
 */
inline void writeScalar(ScreenBuffer& buffer,
                        std::int64_t x,
                        std::int64_t y,
                        char32_t value,
                        int width,
                        TextStyle style = {}) {
    if (width == 1) {
        writeNarrowCell(buffer, x, y, value, style);
        return;
    }

    if (width == 2 &&
        isInsideBuffer(buffer, x, y) &&
        isInsideBuffer(buffer, x + 1, y)) {
        buffer.set({static_cast<Coordinate>(x), static_cast<Coordinate>(y)},
                   Cell{value, CellRole::wide_lead, style});
        buffer.set({static_cast<Coordinate>(x + 1), static_cast<Coordinate>(y)},
                   Cell{U' ', CellRole::wide_continuation, style});
    }
}

/**
 * Writes a preflighted, single-line UTF-8 string at one widened terminal-cell origin.
 *
 * Coordinates stay int64 while clipping so adding menu padding to an extreme int32 Point can never
 * overflow before we discover that the result lies outside ScreenBuffer. The caller must have validated
 * the string with measureMenuLine() before any ScreenBuffer mutation.
 */
inline void writeMenuLine(ScreenBuffer& buffer,
                          std::int64_t origin_x,
                          std::int64_t origin_y,
                          std::string_view text,
                          AmbiguousWidthMode ambiguous_width,
                          TextStyle style = {}) {
    std::int64_t column = 0;

    for (std::size_t offset = 0; offset < text.size();) {
        const DecodedCodePoint decoded = TextMetrics::decodeOne(text, offset);
        if (decoded.consumed == 0) {
            break;
        }
        offset += decoded.consumed;

        const int width = TextMetrics::codePointWidth(decoded.value, ambiguous_width);
        if (width <= 0) {
            // Preflight guarantees this branch is unreachable for accepted menu text.
            continue;
        }

        writeScalar(buffer,
                    origin_x + column,
                    origin_y,
                    decoded.value,
                    width,
                    style);
        column += width;
    }
}

[[nodiscard]] inline TextStyle menuRowStyle(const MenuItemPresentationSnapshot& item,
                                            bool selected) noexcept {
    TextStyle style;

    /*
     * Inverse video is a terminal-portable selection cue and does not require choosing semantic colors
     * prematurely. Disabled rows remain visibly subdued; if such a row were present in a synthetic stale
     * snapshot with selection still set, inverse remains the location cue while dim still communicates
     * that activation is unavailable.
     */
    style.inverse = selected;
    style.dim = !item.enabled && item.kind != MenuItemKind::separator;
    return style;
}

} // namespace detail

/**
 * Measures the complete one-row terminal menu bar from an owned presentation snapshot.
 *
 * Every title receives one leading and one trailing cell. The padding is part of the initial terminal
 * convention and intentionally lives here rather than in MenuBarModel. A backend can therefore evolve
 * its visual chrome without changing semantic menu data.
 */
[[nodiscard]] inline std::optional<MenuPresentationSize>
measureMenuBarPresentation(const MenuBarPresentationSnapshot& snapshot,
                           AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) noexcept {
    Coordinate width = 0;

    for (const std::string& title : snapshot.titles) {
        const auto title_width = detail::measureMenuLine(title, ambiguous_width);
        if (!title_width.has_value()) {
            return std::nullopt;
        }

        Coordinate padded = 0;
        if (!detail::checkedAdd(*title_width, 2, padded) || !detail::checkedAdd(width, padded, width)) {
            return std::nullopt;
        }
    }

    return MenuPresentationSize{Size{width, 1}};
}

/**
 * Measures one vertical terminal popup snapshot.
 *
 * Layout convention for command rows is:
 *
 *     " " + label + [two-cell gap + shortcut] + [" >" for submenu] + " "
 *
 * Separator rows do not establish a larger preferred width; they expand to the width chosen by the
 * content rows. Empty popups therefore still receive a minimal three-cell width so a renderer can draw
 * meaningful chrome without inventing geometry outside this measurement boundary.
 *
 * This function is intentionally not noexcept. Shortcut display formatting creates a temporary
 * std::string and may propagate allocation failure; terminating the process for an ordinary allocation
 * failure would be a substantially worse contract than letting the caller handle the exception.
 */
[[nodiscard]] inline std::optional<MenuPresentationSize>
measureMenuPopupPresentation(const MenuPopupPresentationSnapshot& snapshot,
                             AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    Coordinate widest = 3;

    for (const MenuItemPresentationSnapshot& item : snapshot.items) {
        if (item.kind == MenuItemKind::separator) {
            continue;
        }

        const auto label_width = detail::measureMenuLine(item.text, ambiguous_width);
        if (!label_width.has_value()) {
            return std::nullopt;
        }

        Coordinate row_width = 0;
        if (!detail::checkedAdd(*label_width, 2, row_width)) { // one leading + one trailing cell
            return std::nullopt;
        }

        if (item.shortcut.has_value()) {
            const std::string shortcut_text = shortcutDisplayText(*item.shortcut);
            if (!shortcut_text.empty()) {
                const auto shortcut_width = detail::measureMenuLine(shortcut_text, ambiguous_width);
                if (!shortcut_width.has_value()) {
                    return std::nullopt;
                }
                if (!detail::checkedAdd(row_width, 2, row_width) ||
                    !detail::checkedAdd(row_width, *shortcut_width, row_width)) {
                    return std::nullopt;
                }
            }
        }

        if (item.kind == MenuItemKind::submenu) {
            if (!detail::checkedAdd(row_width, 2, row_width)) { // space + '>'
                return std::nullopt;
            }
        }

        widest = std::max(widest, row_width);
    }

    if (snapshot.items.size() > static_cast<std::size_t>(std::numeric_limits<Coordinate>::max())) {
        return std::nullopt;
    }

    return MenuPresentationSize{Size{widest, static_cast<Coordinate>(snapshot.items.size())}};
}

/**
 * Paints one popup snapshot into ScreenBuffer using the geometry contract above.
 *
 * The function first measures the complete snapshot and returns false without touching the buffer when
 * any menu text cannot be represented by the current Cell model. Once preflight succeeds, each popup row
 * is cleared across the measured width before its content is painted. This makes repaint independent of
 * whatever widget/menu cells occupied the same rectangle in the previous frame.
 *
 * Rendering is clipped naturally at ScreenBuffer edges. A popup may therefore begin partially outside
 * the visible surface without causing range errors. Selection uses inverse video across the full visible
 * row; disabled command rows use dim text; separators are ASCII '-' runs for broad VT compatibility.
 * Shortcut text is right-aligned before the trailing cell (or before the submenu marker reserve), which
 * creates a stable visual column without adding presentation-only columns to MenuModel.
 *
 * @returns true when the snapshot was representable and painting was attempted; false when preflight
 *          failed and the buffer was deliberately left unchanged.
 */
[[nodiscard]] inline bool
renderMenuPopupPresentation(ScreenBuffer& buffer,
                            Point origin,
                            const MenuPopupPresentationSnapshot& snapshot,
                            AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    const auto measured = measureMenuPopupPresentation(snapshot, ambiguous_width);
    if (!measured.has_value()) {
        return false;
    }

    const Coordinate popup_width = measured->size.width;
    const std::int64_t origin_x = origin.x;
    const std::int64_t origin_y = origin.y;

    for (std::size_t index = 0; index < snapshot.items.size(); ++index) {
        const MenuItemPresentationSnapshot& item = snapshot.items[index];
        const bool selected = snapshot.selection == std::optional<std::size_t>{index};
        const TextStyle style = detail::menuRowStyle(item, selected);
        const std::int64_t row_y = origin_y + static_cast<std::int64_t>(index);

        /*
         * Clear/fill the complete logical popup row first. Applying the row style to blank cells makes
         * inverse selection extend through padding and the shortcut gap instead of highlighting only
         * glyph cells, which is the conventional terminal-menu appearance.
         */
        for (Coordinate x = 0; x < popup_width; ++x) {
            detail::writeNarrowCell(buffer, origin_x + x, row_y, U' ', style);
        }

        if (item.kind == MenuItemKind::separator) {
            for (Coordinate x = 1; x + 1 < popup_width; ++x) {
                detail::writeNarrowCell(buffer, origin_x + x, row_y, U'-', style);
            }
            continue;
        }

        detail::writeMenuLine(buffer,
                              origin_x + 1,
                              row_y,
                              item.text,
                              ambiguous_width,
                              style);

        Coordinate right_content_edge = popup_width - 1; // exclusive; final cell is trailing padding

        if (item.kind == MenuItemKind::submenu) {
            detail::writeNarrowCell(buffer,
                                    origin_x + popup_width - 2,
                                    row_y,
                                    U'>',
                                    style);
            right_content_edge -= 2; // reserve one gap cell plus the '>' marker
        }

        if (item.shortcut.has_value()) {
            const std::string shortcut_text = shortcutDisplayText(*item.shortcut);
            if (!shortcut_text.empty()) {
                const auto shortcut_width = detail::measureMenuLine(shortcut_text, ambiguous_width);
                if (shortcut_width.has_value()) {
                    const Coordinate shortcut_x = right_content_edge - *shortcut_width;
                    detail::writeMenuLine(buffer,
                                          origin_x + shortcut_x,
                                          row_y,
                                          shortcut_text,
                                          ambiguous_width,
                                          style);
                }
            }
        }
    }

    return true;
}

} // namespace sasd::ui::terminal
