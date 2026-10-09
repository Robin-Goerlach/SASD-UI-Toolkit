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

[[nodiscard]] bool rectWithin(const Rect& outer, const Rect& inner) noexcept {
    if (outer.isEmpty() || inner.isEmpty()) {
        return false;
    }

    using WideCoordinate = std::int64_t;
    const auto outer_right = static_cast<WideCoordinate>(outer.x) + outer.width;
    const auto outer_bottom = static_cast<WideCoordinate>(outer.y) + outer.height;
    const auto inner_right = static_cast<WideCoordinate>(inner.x) + inner.width;
    const auto inner_bottom = static_cast<WideCoordinate>(inner.y) + inner.height;
    return inner.x >= outer.x && inner.y >= outer.y && inner_right <= outer_right &&
           inner_bottom <= outer_bottom;
}

[[nodiscard]] bool validSnapshot(const RenderedTableViewPresentationSnapshot& snapshot) noexcept {
    if (snapshot.bounds.isEmpty() || snapshot.headers.empty() ||
        snapshot.headers.size() != snapshot.column_bounds.size() ||
        snapshot.headers.size() != snapshot.column_indices.size() ||
        snapshot.rows.size() != snapshot.row_bounds.size() ||
        !rectWithin(snapshot.bounds, snapshot.header_bounds) ||
        snapshot.header_bounds.x != snapshot.bounds.x ||
        snapshot.header_bounds.y != snapshot.bounds.y ||
        snapshot.header_bounds.width != snapshot.bounds.width) {
        return false;
    }
    std::int64_t expected_column_x = snapshot.bounds.x;
    for (const auto& column : snapshot.column_bounds) {
        if (!rectWithin(snapshot.bounds, column) || column.x != expected_column_x ||
            column.y != snapshot.bounds.y || column.height != snapshot.bounds.height) {
            return false;
        }
        expected_column_x += column.width;
    }
    if (expected_column_x > static_cast<std::int64_t>(snapshot.bounds.x) + snapshot.bounds.width) {
        return false;
    }

    std::int64_t expected_row_y = static_cast<std::int64_t>(snapshot.header_bounds.y) +
                                  snapshot.header_bounds.height;
    for (std::size_t index = 0; index < snapshot.rows.size(); ++index) {
        const auto& row = snapshot.rows[index];
        const auto& row_bounds = snapshot.row_bounds[index];
        if (row.cells.size() != snapshot.headers.size() || !rectWithin(snapshot.bounds, row_bounds) ||
            row_bounds.x != snapshot.bounds.x || row_bounds.width != snapshot.bounds.width ||
            static_cast<std::int64_t>(row_bounds.y) != expected_row_y) {
            return false;
        }
        expected_row_y += row_bounds.height;
    }
    if (expected_row_y > static_cast<std::int64_t>(snapshot.bounds.y) + snapshot.bounds.height) {
        return false;
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
    if (row_height <= 0 || row_height > bounds.height) {
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
    result.column_indices.reserve(widths.size());

    std::int64_t x = bounds.x;
    const auto first_visible_column = view.firstVisibleColumn();
    for (std::size_t local_column = 0; local_column < widths.size(); ++local_column) {
        const auto width = widths[local_column];
        const auto right = x + width;
        if (right > static_cast<std::int64_t>(bounds.x) + bounds.width) {
            return std::nullopt;
        }
        if (local_column > std::numeric_limits<std::size_t>::max() - first_visible_column) {
            return std::nullopt;
        }
        const auto narrowed_x = narrow(x);
        if (!narrowed_x.has_value()) {
            return std::nullopt;
        }
        result.column_bounds.push_back({*narrowed_x, bounds.y, width, bounds.height});
        result.column_indices.push_back(first_visible_column + local_column);
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
        const Rect header_cell{snapshot.column_bounds[column].x,
                               snapshot.header_bounds.y,
                               snapshot.column_bounds[column].width,
                               snapshot.header_bounds.height};
        candidate.drawText({header_cell.x, header_cell.y},
                           snapshot.headers[column], TextStyle{}, header_cell);
    }
    for (std::size_t row = 0; row < snapshot.rows.size(); ++row) {
        for (std::size_t column = 0; column < snapshot.headers.size(); ++column) {
            const Rect cell{snapshot.column_bounds[column].x,
                            snapshot.row_bounds[row].y,
                            snapshot.column_bounds[column].width,
                            snapshot.row_bounds[row].height};
            candidate.drawText({cell.x, cell.y}, snapshot.rows[row].cells[column], TextStyle{}, cell);
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
                return RenderedTableViewHit{snapshot.rows[row].row,
                                            snapshot.column_indices[column]};
            }
        }
    }
    return std::nullopt;
}

} // namespace sasd::ui::rendered
