#include <sasd/ui/rendered/menu_presentation.hpp>
#include <sasd/ui/shortcut_display.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

namespace sasd::ui::rendered {
namespace {
using Wide = std::int64_t;
[[nodiscard]] std::optional<Coordinate> add(Coordinate a, Coordinate b) noexcept {
    const Wide value = static_cast<Wide>(a) + static_cast<Wide>(b);
    if (value < std::numeric_limits<Coordinate>::min() ||
        value > std::numeric_limits<Coordinate>::max()) {
        return std::nullopt;
    }
    return static_cast<Coordinate>(value);
}

[[nodiscard]] std::optional<Coordinate> multiply(Coordinate value,
                                                  std::size_t count) noexcept {
    const Wide product = static_cast<Wide>(value) * static_cast<Wide>(count);
    if (product < std::numeric_limits<Coordinate>::min() ||
        product > std::numeric_limits<Coordinate>::max()) {
        return std::nullopt;
    }
    return static_cast<Coordinate>(product);
}

[[nodiscard]] bool exactInset(const Rect& outer,
                              const Rect& inner,
                              Coordinate border) noexcept {
    if (outer.isEmpty() || inner.isEmpty() || border <= 0) {
        return false;
    }

    const Wide expected_x = static_cast<Wide>(outer.x) + border;
    const Wide expected_y = static_cast<Wide>(outer.y) + border;
    const Wide expected_width = static_cast<Wide>(outer.width) - border - border;
    const Wide expected_height = static_cast<Wide>(outer.height) - border - border;
    return expected_width > 0 && expected_height > 0 &&
           expected_x == inner.x && expected_y == inner.y &&
           expected_width == inner.width && expected_height == inner.height;
}

[[nodiscard]] bool exactLane(const Rect& row,
                             const Rect& lane,
                             Coordinate x,
                             Coordinate width) noexcept {
    if (width == 0) {
        return lane.isEmpty();
    }
    return lane.x == x && lane.y == row.y && lane.width == width &&
           lane.height == row.height && row.contains({lane.x, lane.y}) &&
           row.contains({static_cast<Coordinate>(static_cast<Wide>(lane.x) + lane.width - 1),
                         static_cast<Coordinate>(static_cast<Wide>(lane.y) + lane.height - 1)});
}

[[nodiscard]] bool valid(const RenderedMenuPopupPresentationSnapshot& snapshot) noexcept {
    if (snapshot.bounds.isEmpty() || snapshot.content_bounds.isEmpty() ||
        snapshot.rows.empty() || snapshot.row_height <= 0 || snapshot.padding < 0 ||
        snapshot.border_thickness <= 0 || snapshot.label_lane_width < 0 ||
        snapshot.shortcut_lane_width < 0 || snapshot.submenu_indicator_lane_width < 0 ||
        (snapshot.selection.has_value() && *snapshot.selection >= snapshot.rows.size()) ||
        !exactInset(snapshot.bounds, snapshot.content_bounds, snapshot.border_thickness)) {
        return false;
    }

    const auto expected_height = multiply(snapshot.row_height, snapshot.rows.size());
    if (!expected_height.has_value() || snapshot.content_bounds.height != *expected_height) {
        return false;
    }

    for (std::size_t index = 0; index < snapshot.rows.size(); ++index) {
        const auto& row = snapshot.rows[index];
        const auto row_offset = multiply(snapshot.row_height, index);
        const auto expected_y = row_offset.has_value()
                                    ? add(snapshot.content_bounds.y, *row_offset)
                                    : std::nullopt;
        if (!expected_y.has_value() || row.bounds.x != snapshot.content_bounds.x ||
            row.bounds.y != *expected_y || row.bounds.width != snapshot.content_bounds.width ||
            row.bounds.height != snapshot.row_height ||
            row.label_size.width < 0 || row.label_size.height < 0 ||
            row.shortcut_size.width < 0 || row.shortcut_size.height < 0 ||
            row.label_size.width > snapshot.label_lane_width ||
            row.shortcut_size.width > snapshot.shortcut_lane_width) {
            return false;
        }

        const auto label_x = add(row.bounds.x, snapshot.padding);
        const auto label_lane_step = add(snapshot.label_lane_width, snapshot.padding);
        const auto shortcut_base = label_x.has_value() && label_lane_step.has_value()
                                       ? add(*label_x, *label_lane_step)
                                       : std::nullopt;
        const auto indicator_lane_step = add(snapshot.shortcut_lane_width, snapshot.padding);
        const auto indicator_x = shortcut_base.has_value()
                                     && indicator_lane_step.has_value()
                                     ? add(*shortcut_base, *indicator_lane_step)
                                     : std::nullopt;
        if (!label_x.has_value() || !shortcut_base.has_value() || !indicator_x.has_value() ||
            !exactLane(row.bounds, row.label_bounds, *label_x, snapshot.label_lane_width) ||
            !exactLane(row.bounds, row.shortcut_bounds, *shortcut_base,
                       snapshot.shortcut_lane_width) ||
            (row.item.kind == MenuItemKind::submenu
                 ? !exactLane(row.bounds, row.submenu_indicator_bounds, *indicator_x,
                              snapshot.submenu_indicator_lane_width)
                 : !row.submenu_indicator_bounds.isEmpty())) {
            return false;
        }
    }
    return true;
}
} // namespace

std::optional<RenderedMenuPopupPresentationSnapshot> buildMenuPopupPresentation(
    const MenuPopupPresentationSnapshot& menu, Point origin, Rect viewport,
    const RenderedMeasurementContext& metrics) {
    if (menu.items.empty() || viewport.isEmpty()) return std::nullopt;
    const auto theme = metrics.themeMetrics().normalized();
    const Coordinate padding = std::max(Coordinate{2}, theme.control_border_thickness);
    const Coordinate border = std::max(Coordinate{1}, theme.control_border_thickness);
    const Coordinate line = metrics.lineHeight();
    const auto double_padding = add(padding, padding);
    if (!double_padding.has_value() || line <= 0) return std::nullopt;
    const auto row_height = add(line, *double_padding);
    if (!row_height.has_value() || *row_height <= 0) return std::nullopt;

    Coordinate label_lane_width = 0;
    Coordinate shortcut_lane_width = 0;
    bool has_submenu = false;
    for (const auto& item : menu.items) {
        const Size text = metrics.measureText(item.text);
        if (text.width < 0 || text.height < 0 || text.height > line) return std::nullopt;
        label_lane_width = std::max(label_lane_width, text.width);
        if (item.shortcut.has_value()) {
            const std::string shortcut = shortcutDisplayText(*item.shortcut);
            const Size shortcut_size = metrics.measureText(shortcut);
            if (shortcut_size.width < 0 || shortcut_size.height < 0 || shortcut_size.height > line) {
                return std::nullopt;
            }
            shortcut_lane_width = std::max(shortcut_lane_width, shortcut_size.width);
        }
        has_submenu = has_submenu || item.kind == MenuItemKind::submenu;
    }

    const Coordinate indicator_lane_width = has_submenu ? Coordinate{1} : Coordinate{0};
    Coordinate content_width = label_lane_width;
    if (shortcut_lane_width > 0) {
        const auto with_gap = add(content_width, padding);
        if (!with_gap.has_value()) return std::nullopt;
        const auto with_shortcut = add(*with_gap, shortcut_lane_width);
        if (!with_shortcut.has_value()) return std::nullopt;
        content_width = *with_shortcut;
    }
    if (indicator_lane_width > 0) {
        const auto with_gap = add(content_width, padding);
        if (!with_gap.has_value()) return std::nullopt;
        const auto with_indicator = add(*with_gap, indicator_lane_width);
        if (!with_indicator.has_value()) return std::nullopt;
        content_width = *with_indicator;
    }
    const auto padded_width = add(content_width, *double_padding);
    const auto with_border = padded_width.has_value() ? add(*padded_width, border) : std::nullopt;
    const auto outer_width = with_border.has_value() ? add(*with_border, border) : std::nullopt;
    const auto rows_height = multiply(*row_height, menu.items.size());
    const auto with_height_border = rows_height.has_value() ? add(*rows_height, border) : std::nullopt;
    const auto outer_height = with_height_border.has_value() ? add(*with_height_border, border) : std::nullopt;
    if (!outer_width.has_value() || !outer_height.has_value()) return std::nullopt;

    const Rect bounds{origin.x, origin.y, *outer_width, *outer_height};
    if (bounds.isEmpty() || !viewport.contains({bounds.x, bounds.y}) ||
        !viewport.contains({static_cast<Coordinate>(static_cast<Wide>(bounds.x) + bounds.width - 1),
                            static_cast<Coordinate>(static_cast<Wide>(bounds.y) + bounds.height - 1)})) {
        return std::nullopt;
    }

    const auto content_x = add(origin.x, border);
    const auto content_y = add(origin.y, border);
    if (!content_x.has_value() || !content_y.has_value() || !padded_width.has_value()) {
        return std::nullopt;
    }
    const Rect content_bounds{*content_x, *content_y, *padded_width, *rows_height};
    RenderedMenuPopupPresentationSnapshot snapshot;
    snapshot.bounds = bounds;
    snapshot.content_bounds = content_bounds;
    snapshot.selection = menu.selection;
    snapshot.padding = padding;
    snapshot.row_height = *row_height;
    snapshot.border_thickness = border;
    snapshot.label_lane_width = label_lane_width;
    snapshot.shortcut_lane_width = shortcut_lane_width;
    snapshot.submenu_indicator_lane_width = indicator_lane_width;
    snapshot.rows.reserve(menu.items.size());
    for (std::size_t i = 0; i < menu.items.size(); ++i) {
        const auto row_offset = multiply(*row_height, i);
        if (!row_offset.has_value()) return std::nullopt;
        const auto row_y = add(content_bounds.y, *row_offset);
        if (!row_y.has_value()) return std::nullopt;
        const auto label_x = add(content_bounds.x, padding);
        const auto shortcut_x = label_x.has_value()
                                    ? add(*label_x, label_lane_width + padding)
                                    : std::nullopt;
        const auto indicator_x = shortcut_x.has_value()
                                     ? add(*shortcut_x, shortcut_lane_width + padding)
                                     : std::nullopt;
        if (!label_x.has_value() || !shortcut_x.has_value() || !indicator_x.has_value()) return std::nullopt;

        const auto& item = menu.items[i];
        RenderedMenuPopupRow row;
        row.item = item;
        row.bounds = {content_bounds.x, *row_y, content_bounds.width, *row_height};
        row.label_size = metrics.measureText(item.text);
        row.label_bounds = {*label_x, *row_y, label_lane_width, *row_height};
        if (item.shortcut.has_value()) {
            row.shortcut_text = shortcutDisplayText(*item.shortcut);
            row.shortcut_size = metrics.measureText(row.shortcut_text);
        }
        if (shortcut_lane_width > 0) {
            row.shortcut_bounds = {*shortcut_x, *row_y, shortcut_lane_width, *row_height};
        }
        if (indicator_lane_width > 0 && item.kind == MenuItemKind::submenu) {
            row.submenu_indicator_bounds = {*indicator_x, *row_y,
                                            indicator_lane_width, *row_height};
        }
        snapshot.rows.push_back(std::move(row));
    }
    return snapshot;
}

bool renderMenuPopupPresentation(DisplayList& display_list,
                                 const RenderedMenuPopupPresentationSnapshot& snapshot,
                                 Color background_color) {
    if (!valid(snapshot)) return false;
    // Validate everything before copying/mutating the caller's list: malformed public snapshots must
    // fail closed without erasing a previously valid base frame.
    DisplayList candidate = display_list;
    candidate.fillRect(snapshot.bounds, background_color);
    candidate.strokeRect(snapshot.bounds, Color::default_color, snapshot.border_thickness);
    for (std::size_t i = 0; i < snapshot.rows.size(); ++i) {
        const auto& row = snapshot.rows[i];
        const bool selected = snapshot.selection == i;
        if (selected) candidate.fillRect(row.bounds, Color::default_color);
        if (row.item.kind == MenuItemKind::separator) continue;

        TextStyle style;
        style.inverse = selected;
        style.dim = !row.item.enabled;
        candidate.drawText({row.label_bounds.x, row.label_bounds.y + snapshot.padding},
                           row.item.text, style);
        if (!row.shortcut_text.empty()) {
            candidate.drawText({row.shortcut_bounds.x + row.shortcut_bounds.width - row.shortcut_size.width,
                                row.shortcut_bounds.y + snapshot.padding},
                               row.shortcut_text, style);
        }
        if (row.item.kind == MenuItemKind::submenu) {
            candidate.drawText({row.submenu_indicator_bounds.x,
                                row.submenu_indicator_bounds.y + snapshot.padding}, ">", style);
        }
    }
    display_list = std::move(candidate);
    return true;
}

std::optional<std::size_t> menuPopupRowAt(const RenderedMenuPopupPresentationSnapshot& snapshot,
                                          Point point) noexcept {
    if (!valid(snapshot) || !snapshot.bounds.contains(point)) return std::nullopt;
    for (std::size_t i = snapshot.rows.size(); i-- > 0;) if (snapshot.rows[i].bounds.contains(point)) return i;
    return std::nullopt;
}
} // namespace sasd::ui::rendered
