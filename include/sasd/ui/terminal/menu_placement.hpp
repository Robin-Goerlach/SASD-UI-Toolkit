#pragma once

#include <sasd/ui/terminal/menu_frame_presentation.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace sasd::ui::terminal {

namespace detail {

/** Converts widened placement arithmetic back to the public Coordinate domain without truncation. */
[[nodiscard]] inline std::optional<Coordinate>
menuPlacementCoordinate(std::int64_t value) noexcept {
    constexpr auto minimum = static_cast<std::int64_t>(std::numeric_limits<Coordinate>::min());
    constexpr auto maximum = static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max());
    if (value < minimum || value > maximum) {
        return std::nullopt;
    }
    return static_cast<Coordinate>(value);
}

} // namespace detail

/**
 * Computes the natural origin of one root popup below a top-level menu title.
 *
 * "Natural" deliberately means geometry before viewport fitting: the popup starts at the left edge of
 * the selected title's padded terminal span and one row below the menu bar. Keeping this primitive free
 * from ScreenBuffer dimensions separates structural menu geometry from the later policy decision of how
 * a too-wide/too-tall popup should be shifted, clipped or otherwise adapted to a concrete viewport.
 *
 * The complete menu-bar snapshot is measured before prefix geometry is accumulated. This keeps placement
 * on exactly the same representability contract as rendering: a caller cannot obtain a plausible origin
 * for a bar that the terminal presentation layer would subsequently reject. Prefix arithmetic is widened
 * to int64_t and converted back only after range checks, so extreme caller supplied origins fail closed
 * rather than wrapping signed Coordinate values.
 *
 * @returns the natural popup origin, or std::nullopt when menu_index is invalid, the bar is not
 *          representable, or the resulting Point would leave the Coordinate domain.
 */
[[nodiscard]] inline std::optional<Point>
naturalMenuBarPopupOrigin(const MenuBarPresentationSnapshot& bar,
                          std::size_t menu_index,
                          Point menu_bar_origin = {},
                          AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) noexcept {
    if (menu_index >= bar.titles.size() ||
        !measureMenuBarPresentation(bar, ambiguous_width).has_value()) {
        return std::nullopt;
    }

    std::int64_t prefix_width = 0;
    for (std::size_t index = 0; index < menu_index; ++index) {
        const auto title_width = detail::measureMenuLine(bar.titles[index], ambiguous_width);
        if (!title_width.has_value()) {
            return std::nullopt;
        }
        prefix_width += static_cast<std::int64_t>(*title_width) + 2;
    }

    const auto x = detail::menuPlacementCoordinate(
        static_cast<std::int64_t>(menu_bar_origin.x) + prefix_width);
    const auto y = detail::menuPlacementCoordinate(
        static_cast<std::int64_t>(menu_bar_origin.y) + 1);
    if (!x.has_value() || !y.has_value()) {
        return std::nullopt;
    }

    return Point{*x, *y};
}

/**
 * Computes the natural origin of a child popup opened by one submenu row.
 *
 * The child is placed immediately to the right of the measured parent popup and aligned vertically with
 * the row that owns the submenu. As with naturalMenuBarPopupOrigin(), this is intentionally a viewport-
 * independent structural placement. A future fitting policy may move the returned rectangle left/up when
 * the natural position would exceed a concrete terminal surface, without changing menu semantics or the
 * parent/child relationship represented here.
 *
 * The parent popup is measured as a whole before placement is accepted. item_index must identify a
 * submenu item; command and separator rows cannot be treated as submenu anchors even if a synthetic
 * presentation snapshot supplies an arbitrary index. Enabled state is not reinterpreted here because
 * interaction-state validation belongs to MenuInteractionController/snapshot construction, while this
 * function is strictly concerned with geometry of an already supplied presentation value.
 *
 * @returns the natural child-popup origin, or std::nullopt when the parent is not representable, the
 *          anchor is invalid/not a submenu, or widened arithmetic cannot fit back into Point.
 */
[[nodiscard]] inline std::optional<Point>
naturalSubmenuPopupOrigin(const PositionedMenuPopupPresentationSnapshot& parent,
                          std::size_t item_index,
                          AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    if (item_index >= parent.snapshot.items.size() ||
        parent.snapshot.items[item_index].kind != MenuItemKind::submenu) {
        return std::nullopt;
    }

    const auto measured = measureMenuPopupPresentation(parent.snapshot, ambiguous_width);
    if (!measured.has_value()) {
        return std::nullopt;
    }

    const auto x = detail::menuPlacementCoordinate(
        static_cast<std::int64_t>(parent.origin.x) + measured->size.width);
    const auto y = detail::menuPlacementCoordinate(
        static_cast<std::int64_t>(parent.origin.y) + static_cast<std::int64_t>(item_index));
    if (!x.has_value() || !y.has_value()) {
        return std::nullopt;
    }

    return Point{*x, *y};
}

} // namespace sasd::ui::terminal
