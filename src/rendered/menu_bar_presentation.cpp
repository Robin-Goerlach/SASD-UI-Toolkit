#include <sasd/ui/rendered/menu_bar_presentation.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>

namespace sasd::ui::rendered {
namespace {
using Wide = std::int64_t;

[[nodiscard]] std::optional<Coordinate> add(Coordinate left, Coordinate right) noexcept {
    const Wide value = static_cast<Wide>(left) + static_cast<Wide>(right);
    if (value < std::numeric_limits<Coordinate>::min() ||
        value > std::numeric_limits<Coordinate>::max()) {
        return std::nullopt;
    }
    return static_cast<Coordinate>(value);
}

[[nodiscard]] bool valid(const RenderedMenuBarPresentationSnapshot& snapshot) noexcept {
    if (snapshot.horizontal_padding < 0 || snapshot.row_height <= 0 ||
        (snapshot.selection.has_value() && *snapshot.selection >= snapshot.items.size())) {
        return false;
    }

    if (snapshot.items.empty()) {
        return snapshot.bounds.width == 0 && snapshot.bounds.height == snapshot.row_height;
    }

    if (snapshot.bounds.isEmpty() || snapshot.bounds.height != snapshot.row_height) {
        return false;
    }

    for (std::size_t index = 0; index < snapshot.items.size(); ++index) {
        const auto& item = snapshot.items[index];
        if (item.measured_size.width < 0 || item.measured_size.height < 0 ||
            item.bounds.isEmpty() || item.bounds.y != snapshot.bounds.y ||
            item.bounds.height != snapshot.row_height ||
            !snapshot.bounds.contains({item.bounds.x, item.bounds.y}) ||
            !snapshot.bounds.contains({
                static_cast<Coordinate>(static_cast<Wide>(item.bounds.x) + item.bounds.width - 1),
                static_cast<Coordinate>(static_cast<Wide>(item.bounds.y) + item.bounds.height - 1)})) {
            return false;
        }
        if (index > 0 && snapshot.items[index - 1].bounds.x +
                              snapshot.items[index - 1].bounds.width != item.bounds.x) {
            return false;
        }
    }
    return true;
}
} // namespace

std::optional<RenderedMenuBarPresentationSnapshot> buildMenuBarPresentation(
    const MenuBarPresentationSnapshot& menu,
    Point origin,
    Rect viewport,
    const RenderedMeasurementContext& metrics) {
    if (viewport.width < 0 || viewport.height < 0 || viewport.isEmpty()) {
        return std::nullopt;
    }

    const auto theme = metrics.themeMetrics().normalized();
    const Coordinate padding = std::max(Coordinate{2}, theme.control_border_thickness);
    const Coordinate row_height = metrics.lineHeight();
    if (row_height <= 0) {
        return std::nullopt;
    }

    RenderedMenuBarPresentationSnapshot snapshot;
    snapshot.selection = menu.selection;
    snapshot.popup_open = menu.popup_open;
    snapshot.horizontal_padding = padding;
    snapshot.row_height = row_height;
    snapshot.items.reserve(menu.titles.size());

    Coordinate x = origin.x;
    for (const std::string& title : menu.titles) {
        const Size measured = metrics.measureText(title);
        if (measured.width < 0 || measured.height < 0 || measured.height > row_height) {
            return std::nullopt;
        }
        const auto content_width = add(measured.width, padding);
        const auto item_width = content_width.has_value() ? add(*content_width, padding) : std::nullopt;
        if (!item_width.has_value() || *item_width <= 0) {
            return std::nullopt;
        }
        const auto next_x = add(x, *item_width);
        if (!next_x.has_value()) {
            return std::nullopt;
        }
        snapshot.items.push_back(RenderedMenuBarItem{
            title,
            measured,
            Rect{x, origin.y, *item_width, row_height},
        });
        x = *next_x;
    }

    snapshot.bounds = {origin.x, origin.y,
                       static_cast<Coordinate>(static_cast<Wide>(x) - origin.x), row_height};
    if (!snapshot.items.empty() &&
        (!viewport.contains({snapshot.bounds.x, snapshot.bounds.y}) ||
         !viewport.contains({static_cast<Coordinate>(static_cast<Wide>(snapshot.bounds.x) +
                                                       snapshot.bounds.width - 1),
                             static_cast<Coordinate>(static_cast<Wide>(snapshot.bounds.y) +
                                                       snapshot.bounds.height - 1)}))) {
        return std::nullopt;
    }
    return snapshot;
}

bool renderMenuBarPresentation(DisplayList& display_list,
                               const RenderedMenuBarPresentationSnapshot& snapshot,
                               Color background_color) {
    if (!valid(snapshot)) {
        return false;
    }

    // Preflight has completed before this local copy is changed. A malformed public value cannot leave a
    // valid base list partially erased, which is important when the bar is an always-visible surface.
    DisplayList candidate = display_list;
    for (std::size_t index = 0; index < snapshot.items.size(); ++index) {
        const auto& item = snapshot.items[index];
        const bool selected = snapshot.selection == index;
        if (selected) {
            candidate.fillRect(item.bounds, background_color);
        }
        TextStyle style;
        style.inverse = selected;
        candidate.drawText({item.bounds.x + snapshot.horizontal_padding,
                            item.bounds.y + snapshot.horizontal_padding},
                           item.title, style);
    }
    display_list = std::move(candidate);
    return true;
}

std::optional<std::size_t> menuBarItemAt(
    const RenderedMenuBarPresentationSnapshot& snapshot,
    Point point) noexcept {
    if (!valid(snapshot) || !snapshot.bounds.contains(point)) {
        return std::nullopt;
    }
    for (std::size_t index = snapshot.items.size(); index-- > 0;) {
        if (snapshot.items[index].bounds.contains(point)) {
            return index;
        }
    }
    return std::nullopt;
}
} // namespace sasd::ui::rendered
