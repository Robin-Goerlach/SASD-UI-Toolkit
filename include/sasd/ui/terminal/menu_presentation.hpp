#pragma once

#include <sasd/ui/menu_interaction_view.hpp>
#include <sasd/ui/shortcut_display.hpp>
#include <sasd/ui/terminal/text_metrics.hpp>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace sasd::ui::terminal {

/**
 * Terminal-cell measurement of one owned menu-presentation snapshot.
 *
 * This layer deliberately stops before painting cells. It answers the geometry question using exactly
 * the same Unicode width policy as the rest of the Terminal backend, while leaving colors, borders and
 * clipping to the later renderer. Keeping measurement separate makes the first menu slice deterministic
 * and easy to test without prematurely coupling semantic menu state to ScreenBuffer drawing details.
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
 * Separator rows do not establish a larger preferred width; they will later expand to the width chosen
 * by the content rows. Empty popups therefore still receive a minimal three-cell width so a renderer can
 * draw meaningful chrome without inventing geometry outside this measurement boundary.
 */
[[nodiscard]] inline std::optional<MenuPresentationSize>
measureMenuPopupPresentation(const MenuPopupPresentationSnapshot& snapshot,
                             AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) noexcept {
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

        if (row_width > widest) {
            widest = row_width;
        }
    }

    if (snapshot.items.size() > static_cast<std::size_t>(std::numeric_limits<Coordinate>::max())) {
        return std::nullopt;
    }

    return MenuPresentationSize{Size{widest, static_cast<Coordinate>(snapshot.items.size())}};
}

} // namespace sasd::ui::terminal
