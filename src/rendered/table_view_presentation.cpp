#include <sasd/ui/rendered/table_view_presentation.hpp>

#include <algorithm>
#include <limits>
#include <utility>

namespace sasd::ui::rendered {
namespace {

[[nodiscard]] std::optional<Coordinate> narrow(std::int64_t value) noexcept {
    if (value < std::numeric_limits<Coordinate>::min() ||
        value > std::numeric_limits<Coordinate>::max()) {
        return std::nullopt;
    }
    return static_cast<Coordinate>(value);
}

[[nodiscard]] bool validSnapshot(const RenderedTableViewPresentationSnapshot& snapshot) noexcept {
    if (snapshot.bounds.width < 0 || snapshot.bounds.height < 0 ||
        snapshot.header_bounds.width < 0 || snapshot.header_bounds.height < 0 ||
        snapshot.headers.size() != snapshot.column_bounds.size() ||
        snapshot.rows.size() != snapshot.row_bounds.size() || snapshot.headers.empty()) {
        return false;
    }
    if (!snapshot.bounds.contains({snapshot.bounds.x, snapshot.bounds.y}) &&
        !snapshot.bounds.isEmpty()) {
        return false;
    }
    for (const auto& column : snapshot.column_bounds) {
        const auto column_right = static_cast<std::int64_t>(column.x) + column.width - 1;
        const auto column_bottom = static_cast<std::int64_t>(column.y) + column.height - 1;
        if (column.width <= 0 || column.height <= 0 || !snapshot.bounds.contains({column.x, column.y}) ||
            column_right > std::numeric_limits<Coordinate>::max() ||
            column_bottom > std::numeric_limits<Coordinate>::max() ||
            !snapshot.bounds.contains({static_cast<Coordinate>(column_right),
                                       static_cast<Coordinate>(column_bottom)})) {
            return false;
        }
    }
    for (std::size_t index = 0; index < snapshot.rows.size(); ++index) {
        const auto& row = snapshot.rows[index];
        const auto& row_bounds = snapshot.row_bounds[index];
        if (row.cells.size() != snapshot.headers.size() || row_bounds.width <= 0 ||
            row_bounds.height <= 0 || !snapshot.bounds.contains({row_bounds.x, row_bounds.y})) {
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<RenderedTableViewPresentationSnapshot>
RenderedTableViewPresentation::snapshot(const TableView& view,
                                        Rect bounds,
                                        const RenderedMeasurementContext& measurement_context) {
    if (bounds.width <= 0 || bounds.height <= 0) {
        return std::nullopt;
    }

    const auto headers = view.visibleHeaders();
    const auto rows = view.visibleRows();
    const auto* model = view.model();
    if (model == nullptr || headers.empty() ||
        rows.size() >= static_cast<std::size_t>(bounds.height)) {
        return std::nullopt;
    }

    const Coordinate row_height = measurement_context.lineHeight();
    if (row_height <= 0) {
        return std::nullopt;
    }

    std::vector<Coordinate> widths(headers.size(), 0);
    for (std::size_t column = 0; column < headers.size(); ++column) {
        const auto header_size = measurement_context.measureText(headers[column]);
        if (header_size.width < 0 || header_size.height < 0) {
            return std::nullopt;
        }
        widths[column] = header_size.width;
        for (const auto& row : rows) {
            const auto cell_size = measurement_context.measureText(row.cells[column]);
            if (cell_size.width < 0 || cell_size.height < 0) {
                return std::nullopt;
            }
            widths[column] = std::max(widths[column], cell_size.width);
        }
        if (widths[column] >= std::numeric_limits<Coordinate>::max()) {
            return std::nullopt;
        }
        ++widths[column]; // One logical cell of separation is part of the fixed lane contract.
    }

    RenderedTableViewPresentationSnapshot result;
    result.bounds = bounds;
    result.headers = headers;
    result.rows = rows;
    result.model_revision = model->revision();
    result.header_bounds = {bounds.x, bounds.y, bounds.width, row_height};
    result.column_bounds.reserve(widths.size());

    std::int64_t x = bounds.x;
    for (const auto width : widths) {
        const auto right = x + width;
        if (right > static_cast<std::int64_t>(bounds.x) + bounds.width) {
            return std::nullopt;
        }
        const auto narrowed_x = narrow(x);
        if (!narrowed_x.has_value()) {
            return std::nullopt;
        }
        result.column_bounds.push_back({*narrowed_x, bounds.y, width, bounds.height});
        x = right;
    }

    result.row_bounds.reserve(rows.size());
    for (std::size_t offset = 0; offset < rows.size(); ++offset) {
        const auto y = static_cast<std::int64_t>(bounds.y) + row_height +
                       static_cast<std::int64_t>(offset) * row_height;
        const auto narrowed_y = narrow(y);
        if (!narrowed_y.has_value() || y + row_height > static_cast<std::int64_t>(bounds.y) +
                                                       bounds.height) {
            return std::nullopt;
        }
        result.row_bounds.push_back({bounds.x, *narrowed_y, bounds.width, row_height});
    }
    return result;
}

bool RenderedTableViewPresentation::render(
    DisplayList& display_list,
    const RenderedTableViewPresentationSnapshot& snapshot) {
    if (!validSnapshot(snapshot)) {
        return false;
    }

    // Render into a copy first. DisplayList has no rollback primitive, so a malformed public value
    // snapshot must not partially mutate the caller's already valid base frame.
    DisplayList candidate = display_list;
    candidate.fillRect(snapshot.bounds);
    for (std::size_t column = 0; column < snapshot.headers.size(); ++column) {
        candidate.drawText({snapshot.column_bounds[column].x, snapshot.header_bounds.y},
                           snapshot.headers[column], TextStyle{}, snapshot.column_bounds[column]);
    }
    for (std::size_t row = 0; row < snapshot.rows.size(); ++row) {
        for (std::size_t column = 0; column < snapshot.headers.size(); ++column) {
            candidate.drawText({snapshot.column_bounds[column].x, snapshot.row_bounds[row].y},
                               snapshot.rows[row].cells[column], TextStyle{},
                               snapshot.column_bounds[column]);
        }
    }
    display_list = std::move(candidate);
    return true;
}

std::optional<RenderedTableViewHit>
RenderedTableViewPresentation::hitAt(const RenderedTableViewPresentationSnapshot& snapshot,
                                     Point point) noexcept {
    if (!validSnapshot(snapshot) || !snapshot.bounds.contains(point) ||
        snapshot.header_bounds.contains(point)) {
        return std::nullopt;
    }
    for (std::size_t row = 0; row < snapshot.row_bounds.size(); ++row) {
        if (!snapshot.row_bounds[row].contains(point)) {
            continue;
        }
        for (std::size_t column = 0; column < snapshot.column_bounds.size(); ++column) {
            if (snapshot.column_bounds[column].contains(point)) {
                return RenderedTableViewHit{snapshot.rows[row].row, column};
            }
        }
    }
    return std::nullopt;
}

} // namespace sasd::ui::rendered
