#include <sasd/ui/rendered/list_view_presentation.hpp>

#include <limits>
#include <utility>

namespace sasd::ui::rendered {

std::optional<RenderedListViewPresentationSnapshot>
RenderedListViewPresentation::snapshot(const ListView& view, Rect bounds) {
    if (bounds.width < 0 || bounds.height < 0) {
        return std::nullopt;
    }
    auto rows = view.visibleRows();
    if (rows.size() > static_cast<std::size_t>(bounds.height)) {
        return std::nullopt;
    }
    const auto* model = view.model();
    if (model == nullptr) {
        return std::nullopt;
    }
    return RenderedListViewPresentationSnapshot{
        bounds, std::move(rows), view.hasFocus(), model->revision()};
}

bool RenderedListViewPresentation::render(
    DisplayList& display_list,
    const RenderedListViewPresentationSnapshot& presentation,
    Color background_color) {
    if (presentation.bounds.width < 0 || presentation.bounds.height < 0 ||
        presentation.rows.size() > static_cast<std::size_t>(presentation.bounds.height)) {
        return false;
    }

    // Resolve every coordinate before appending anything. DisplayList has no rollback operation, so
    // a malformed later row must leave the previous frame untouched.
    for (std::size_t offset = 0; offset < presentation.rows.size(); ++offset) {
        const auto row_y = static_cast<std::int64_t>(presentation.bounds.y) +
                           static_cast<std::int64_t>(offset);
        if (row_y < std::numeric_limits<Coordinate>::min() ||
            row_y > std::numeric_limits<Coordinate>::max()) {
            return false;
        }
    }

    display_list.fillRect(presentation.bounds, background_color);
    for (std::size_t offset = 0; offset < presentation.rows.size(); ++offset) {
        const auto& row = presentation.rows[offset];
        const auto row_y = static_cast<std::int64_t>(presentation.bounds.y) +
                           static_cast<std::int64_t>(offset);
        const Rect row_bounds{presentation.bounds.x,
                              static_cast<Coordinate>(row_y),
                              presentation.bounds.width,
                              1};
        TextStyle style{};
        if (row.selected) {
            style.inverse = true;
            style.bold = presentation.focused;
        }
        display_list.drawText({row_bounds.x, row_bounds.y}, row.text, style, row_bounds);
    }
    return true;
}

std::optional<std::size_t>
RenderedListViewPresentation::rowAt(const RenderedListViewPresentationSnapshot& presentation,
                                    Point point) noexcept {
    if (presentation.bounds.isEmpty() || !presentation.bounds.contains(point) ||
        presentation.rows.size() > static_cast<std::size_t>(presentation.bounds.height)) {
        return std::nullopt;
    }
    const auto offset = static_cast<std::size_t>(
        static_cast<std::int64_t>(point.y) - static_cast<std::int64_t>(presentation.bounds.y));
    return offset < presentation.rows.size() ? std::optional{presentation.rows[offset].row}
                                             : std::nullopt;
}

bool RenderedListViewPresentation::selectAt(
    const RenderedListViewPresentationSnapshot& presentation,
    Point point,
    ListSelectionModel& selection) noexcept {
    const auto row = rowAt(presentation, point);
    const auto* model = selection.model();
    if (!row.has_value() || model == nullptr || model->revision() != presentation.model_revision) {
        return false;
    }
    return selection.select(*row);
}

} // namespace sasd::ui::rendered
