#include <sasd/ui/rendered/menu_presentation.hpp>

#include <algorithm>
#include <limits>

namespace sasd::ui::rendered {
namespace {
using Wide = std::int64_t;
std::optional<Coordinate> add(Coordinate a, Coordinate b) {
    const Wide value = static_cast<Wide>(a) + static_cast<Wide>(b);
    if (value < std::numeric_limits<Coordinate>::min() || value > std::numeric_limits<Coordinate>::max()) return std::nullopt;
    return static_cast<Coordinate>(value);
}
bool valid(const RenderedMenuPopupPresentationSnapshot& s) noexcept {
    if (s.bounds.isEmpty() || s.rows.empty() || s.row_height <= 0 || s.padding < 0 || s.border_thickness <= 0 ||
        (s.selection.has_value() && *s.selection >= s.rows.size())) return false;
    for (const auto& row : s.rows) if (row.bounds.isEmpty() || !s.bounds.contains({row.bounds.x, row.bounds.y}) ||
        static_cast<Wide>(row.bounds.x) + row.bounds.width > static_cast<Wide>(s.bounds.x) + s.bounds.width ||
        static_cast<Wide>(row.bounds.y) + row.bounds.height > static_cast<Wide>(s.bounds.y) + s.bounds.height) return false;
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
    const auto row_height = add(line, padding + padding);
    if (!row_height.has_value() || *row_height <= 0) return std::nullopt;
    Coordinate width = 0;
    for (const auto& item : menu.items) {
        const Size text = metrics.measureText(item.text);
        if (text.width < 0 || text.height < 0) return std::nullopt;
        const auto candidate = add(text.width, padding + padding + border + border);
        if (!candidate.has_value()) return std::nullopt;
        width = std::max(width, *candidate);
    }
    const Wide height = static_cast<Wide>(*row_height) * menu.items.size() + static_cast<Wide>(border) * 2;
    if (height > std::numeric_limits<Coordinate>::max()) return std::nullopt;
    const Rect bounds{origin.x, origin.y, width, static_cast<Coordinate>(height)};
    if (bounds.isEmpty() || bounds.x < viewport.x || bounds.y < viewport.y ||
        static_cast<Wide>(bounds.x) + bounds.width > static_cast<Wide>(viewport.x) + viewport.width ||
        static_cast<Wide>(bounds.y) + bounds.height > static_cast<Wide>(viewport.y) + viewport.height) return std::nullopt;
    RenderedMenuPopupPresentationSnapshot snapshot;
    snapshot.bounds = bounds; snapshot.selection = menu.selection; snapshot.padding = padding;
    snapshot.row_height = *row_height; snapshot.border_thickness = border;
    snapshot.rows.reserve(menu.items.size());
    for (std::size_t i = 0; i < menu.items.size(); ++i) {
        const Wide row_offset = static_cast<Wide>(i) * static_cast<Wide>(*row_height);
        if (row_offset > std::numeric_limits<Coordinate>::max()) return std::nullopt;
        snapshot.rows.push_back({menu.items[i], {origin.x + border, origin.y + border + static_cast<Coordinate>(row_offset), width - border - border, *row_height}});
    }
    return snapshot;
}

bool renderMenuPopupPresentation(DisplayList& display_list,
                                 const RenderedMenuPopupPresentationSnapshot& snapshot,
                                 Color background_color) {
    if (!valid(snapshot)) return false;
    DisplayList candidate = display_list;
    candidate.fillRect(snapshot.bounds, background_color);
    candidate.strokeRect(snapshot.bounds, Color::default_color, snapshot.border_thickness);
    for (std::size_t i = 0; i < snapshot.rows.size(); ++i) {
        const auto& row = snapshot.rows[i];
        if (snapshot.selection == i) candidate.fillRect(row.bounds, Color::default_color);
        candidate.drawText({row.bounds.x + snapshot.padding, row.bounds.y + snapshot.padding}, row.item.text,
                           TextStyle{.inverse = snapshot.selection == i});
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
