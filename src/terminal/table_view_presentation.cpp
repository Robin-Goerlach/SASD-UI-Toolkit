#include <sasd/ui/terminal/table_view_presentation.hpp>

#include <algorithm>
#include <limits>
#include <utility>

namespace sasd::ui::terminal {
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
    const auto outer_right = static_cast<std::int64_t>(outer.x) + outer.width;
    const auto outer_bottom = static_cast<std::int64_t>(outer.y) + outer.height;
    const auto inner_right = static_cast<std::int64_t>(inner.x) + inner.width;
    const auto inner_bottom = static_cast<std::int64_t>(inner.y) + inner.height;
    return inner.x >= outer.x && inner.y >= outer.y && inner_right <= outer_right &&
           inner_bottom <= outer_bottom;
}

[[nodiscard]] bool validSnapshot(
    const TerminalTableViewPresentationSnapshot& snapshot) noexcept {
    if (snapshot.bounds.isEmpty() || snapshot.headers.empty() ||
        snapshot.headers.size() != snapshot.column_bounds.size() ||
        snapshot.headers.size() != snapshot.column_indices.size() ||
        snapshot.rows.size() != snapshot.row_bounds.size() ||
        snapshot.rows.size() != snapshot.cell_bounds.size() ||
        !rectWithin(snapshot.bounds, snapshot.header_bounds) ||
        snapshot.header_bounds.x != snapshot.bounds.x ||
        snapshot.header_bounds.y != snapshot.bounds.y ||
        snapshot.header_bounds.width != snapshot.bounds.width ||
        snapshot.header_bounds.height != 1) {
        return false;
    }

    std::int64_t expected_x = snapshot.bounds.x;
    for (const auto& column : snapshot.column_bounds) {
        if (!rectWithin(snapshot.bounds, column) || column.x != expected_x ||
            column.y != snapshot.bounds.y || column.height != snapshot.bounds.height) {
            return false;
        }
        expected_x += column.width;
    }
    if (expected_x > static_cast<std::int64_t>(snapshot.bounds.x) + snapshot.bounds.width) {
        return false;
    }

    std::int64_t expected_y = static_cast<std::int64_t>(snapshot.bounds.y) + 1;
    for (std::size_t row = 0; row < snapshot.rows.size(); ++row) {
        const auto& row_bounds = snapshot.row_bounds[row];
        if (!rectWithin(snapshot.bounds, row_bounds) || row_bounds.x != snapshot.bounds.x ||
            row_bounds.width != snapshot.bounds.width || row_bounds.y != expected_y ||
            row_bounds.height != 1 || snapshot.rows[row].cells.size() != snapshot.headers.size() ||
            snapshot.cell_bounds[row].size() != snapshot.headers.size()) {
            return false;
        }
        for (std::size_t column = 0; column < snapshot.headers.size(); ++column) {
            const Rect expected_cell{snapshot.column_bounds[column].x,
                                     row_bounds.y,
                                     snapshot.column_bounds[column].width,
                                     1};
            if (snapshot.cell_bounds[row][column] != expected_cell ||
                !rectWithin(row_bounds, snapshot.cell_bounds[row][column])) {
                return false;
            }
        }
        if (snapshot.rows[row].selected_column.has_value() &&
            std::find(snapshot.column_indices.begin(), snapshot.column_indices.end(),
                      *snapshot.rows[row].selected_column) == snapshot.column_indices.end()) {
            return false;
        }
        expected_y += 1;
    }
    return expected_y <= static_cast<std::int64_t>(snapshot.bounds.y) + snapshot.bounds.height;
}

void writeScalar(ScreenBuffer& buffer,
                 const Rect& clip,
                 std::int64_t x,
                 std::int64_t y,
                 char32_t value,
                 int width,
                 TextStyle style) {
    const auto right = static_cast<std::int64_t>(clip.x) + clip.width;
    if (x < clip.x || x + width > right || y != clip.y || x < 0 || y < 0 ||
        x >= buffer.size().width || y >= buffer.size().height) {
        return;
    }
    if (width == 1) {
        buffer.set({static_cast<Coordinate>(x), static_cast<Coordinate>(y)},
                   Cell{value, CellRole::normal, style});
    } else if (width == 2 && x + 1 < right && x + 1 < buffer.size().width) {
        buffer.set({static_cast<Coordinate>(x), static_cast<Coordinate>(y)},
                   Cell{value, CellRole::wide_lead, style});
        buffer.set({static_cast<Coordinate>(x + 1), static_cast<Coordinate>(y)},
                   Cell{U' ', CellRole::wide_continuation, style});
    }
}

void drawText(ScreenBuffer& buffer,
              const Rect& clip,
              std::string_view text,
              TextStyle style,
              AmbiguousWidthMode ambiguous_width) {
    std::int64_t x = clip.x;
    for (std::size_t input = 0; input < text.size();) {
        const auto decoded = TextMetrics::decodeOne(text, input);
        if (decoded.consumed == 0) {
            break;
        }
        input += decoded.consumed;
        const int width = TextMetrics::codePointWidth(decoded.value, ambiguous_width);
        if (width > 0) {
            writeScalar(buffer, clip, x, clip.y, decoded.value, width, style);
            x += width;
        }
    }
}

[[nodiscard]] bool renderable(std::string_view text,
                              AmbiguousWidthMode ambiguous_width) noexcept {
    const auto measurement = TextMetrics::measureUtf8(text, ambiguous_width);
    return measurement.simpleCellRenderable() && measurement.rows == 1 && !measurement.saturated;
}

} // namespace

std::optional<TerminalTableViewPresentationSnapshot>
TerminalTableViewPresentation::snapshot(const TableView& view,
                                        Rect bounds,
                                        AmbiguousWidthMode ambiguous_width) {
    if (bounds.width <= 0 || bounds.height <= 0) {
        return std::nullopt;
    }
    const auto headers = view.visibleHeaders();
    const auto rows = view.visibleRows();
    const auto* model = view.model();
    if (model == nullptr || headers.empty() || rows.size() >= static_cast<std::size_t>(bounds.height)) {
        return std::nullopt;
    }

    std::vector<Coordinate> content_widths(headers.size(), 0);
    for (std::size_t column = 0; column < headers.size(); ++column) {
        const auto header_measurement = TextMetrics::measureUtf8(headers[column], ambiguous_width);
        if (!header_measurement.simpleCellRenderable() || header_measurement.rows != 1 ||
            header_measurement.saturated || header_measurement.columns < 0) {
            return std::nullopt;
        }
        content_widths[column] = header_measurement.columns;
        for (const auto& row : rows) {
            const auto measurement = TextMetrics::measureUtf8(row.cells[column], ambiguous_width);
            if (!measurement.simpleCellRenderable() || measurement.rows != 1 ||
                measurement.saturated || measurement.columns < 0) {
                return std::nullopt;
            }
            content_widths[column] = std::max(content_widths[column], measurement.columns);
        }
        if (content_widths[column] >= std::numeric_limits<Coordinate>::max()) {
            return std::nullopt;
        }
        ++content_widths[column];
    }

    TerminalTableViewPresentationSnapshot result;
    result.bounds = bounds;
    result.header_bounds = {bounds.x, bounds.y, bounds.width, 1};
    result.headers = headers;
    result.rows = rows;
    result.focused = view.hasFocus();
    result.ambiguous_width = ambiguous_width;
    result.model_revision = model->revision();
    result.column_bounds.reserve(content_widths.size());
    result.column_indices.reserve(content_widths.size());
    result.row_bounds.reserve(rows.size());
    result.cell_bounds.resize(rows.size());

    const auto first_column = view.firstVisibleColumn();
    std::int64_t x = bounds.x;
    for (std::size_t column = 0; column < content_widths.size(); ++column) {
        if (column > std::numeric_limits<std::size_t>::max() - first_column) {
            return std::nullopt;
        }
        const auto right = x + content_widths[column];
        if (right > static_cast<std::int64_t>(bounds.x) + bounds.width) {
            return std::nullopt;
        }
        const auto narrowed_x = narrow(x);
        if (!narrowed_x.has_value()) {
            return std::nullopt;
        }
        result.column_bounds.push_back({*narrowed_x, bounds.y, content_widths[column], bounds.height});
        result.column_indices.push_back(first_column + column);
        x = right;
    }

    for (std::size_t row = 0; row < rows.size(); ++row) {
        const auto y = static_cast<std::int64_t>(bounds.y) + 1 +
                       static_cast<std::int64_t>(row);
        if (y > static_cast<std::int64_t>(bounds.y) + bounds.height - 1) {
            return std::nullopt;
        }
        const auto narrowed_y = narrow(y);
        if (!narrowed_y.has_value()) {
            return std::nullopt;
        }
        result.row_bounds.push_back({bounds.x, *narrowed_y, bounds.width, 1});
        result.cell_bounds[row].reserve(content_widths.size());
        for (std::size_t column = 0; column < content_widths.size(); ++column) {
            result.cell_bounds[row].push_back({result.column_bounds[column].x, *narrowed_y,
                                               result.column_bounds[column].width, 1});
        }
    }
    return result;
}

bool TerminalTableViewPresentation::render(
    ScreenBuffer& buffer,
    const TerminalTableViewPresentationSnapshot& presentation,
    AmbiguousWidthMode ambiguous_width) {
    if (!validSnapshot(presentation) || presentation.ambiguous_width != ambiguous_width) {
        return false;
    }
    for (const auto& header : presentation.headers) {
        if (!renderable(header, ambiguous_width)) {
            return false;
        }
    }
    for (const auto& row : presentation.rows) {
        for (const auto& cell : row.cells) {
            if (!renderable(cell, ambiguous_width)) {
                return false;
            }
        }
    }

    // ScreenBuffer owns its vector. Rendering a complete candidate first preserves the caller's
    // previous frame when malformed text or geometry is discovered in a later lane.
    ScreenBuffer candidate = buffer;
    candidate.fill(presentation.bounds, Cell{});
    for (std::size_t column = 0; column < presentation.headers.size(); ++column) {
        candidate.fill({presentation.column_bounds[column].x,
                        presentation.header_bounds.y,
                        presentation.column_bounds[column].width,
                        1},
                       Cell{});
        drawText(candidate,
                 {presentation.column_bounds[column].x,
                  presentation.header_bounds.y,
                  presentation.column_bounds[column].width,
                  1},
                 presentation.headers[column],
                 {},
                 ambiguous_width);
    }
    for (std::size_t row = 0; row < presentation.rows.size(); ++row) {
        for (std::size_t column = 0; column < presentation.headers.size(); ++column) {
            const auto& cell_bounds = presentation.cell_bounds[row][column];
            candidate.fill(cell_bounds, Cell{});
            TextStyle style{};
            if (presentation.rows[row].selected_column.has_value() &&
                *presentation.rows[row].selected_column == presentation.column_indices[column]) {
                style.inverse = true;
                if (presentation.focused) {
                    style.bold = true;
                }
            }
            drawText(candidate, cell_bounds, presentation.rows[row].cells[column], style,
                     ambiguous_width);
        }
    }
    buffer = std::move(candidate);
    return true;
}

std::optional<TerminalTableViewHit> TerminalTableViewPresentation::hitAt(
    const TerminalTableViewPresentationSnapshot& presentation,
    Point point) noexcept {
    if (!validSnapshot(presentation) || !presentation.bounds.contains(point) ||
        presentation.header_bounds.contains(point)) {
        return std::nullopt;
    }
    for (std::size_t row = 0; row < presentation.row_bounds.size(); ++row) {
        if (!presentation.row_bounds[row].contains(point)) {
            continue;
        }
        for (std::size_t column = 0; column < presentation.column_bounds.size(); ++column) {
            if (presentation.cell_bounds[row][column].contains(point)) {
                return TerminalTableViewHit{presentation.rows[row].row,
                                            presentation.column_indices[column]};
            }
        }
    }
    return std::nullopt;
}

bool TerminalTableViewPresentation::selectAt(
    const TerminalTableViewPresentationSnapshot& presentation,
    Point point,
    TableSelectionModel& selection) {
    const auto* model = selection.model();
    const auto hit = hitAt(presentation, point);
    if (!hit.has_value() || model == nullptr || model->revision() != presentation.model_revision) {
        return false;
    }
    return selection.select(hit->row, hit->column);
}

} // namespace sasd::ui::terminal
