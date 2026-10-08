#include <sasd/ui/terminal/list_view_presentation.hpp>

#include <algorithm>
#include <cstdint>

namespace sasd::ui::terminal {
namespace {

[[nodiscard]] bool inside(const ScreenBuffer& buffer, std::int64_t x, std::int64_t y) noexcept {
    return x >= 0 && y >= 0 && x < buffer.size().width && y < buffer.size().height;
}

void writeScalar(ScreenBuffer& buffer,
                 std::int64_t x,
                 std::int64_t y,
                 char32_t value,
                 int width,
                 TextStyle style) {
    if (width == 1 && inside(buffer, x, y)) {
        buffer.set({static_cast<Coordinate>(x), static_cast<Coordinate>(y)},
                   Cell{value, CellRole::normal, style});
    } else if (width == 2 && inside(buffer, x, y) && inside(buffer, x + 1, y)) {
        buffer.set({static_cast<Coordinate>(x), static_cast<Coordinate>(y)},
                   Cell{value, CellRole::wide_lead, style});
        buffer.set({static_cast<Coordinate>(x + 1), static_cast<Coordinate>(y)},
                   Cell{U' ', CellRole::wide_continuation, style});
    }
}

void clear(ScreenBuffer& buffer, Rect bounds) noexcept {
    buffer.fill(bounds, Cell{});
}

} // namespace

std::optional<TerminalListViewPresentationSnapshot>
TerminalListViewPresentation::snapshot(const ListView& view, Rect bounds) {
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
    return TerminalListViewPresentationSnapshot{
        bounds, std::move(rows), view.hasFocus(), model->revision()};
}

bool TerminalListViewPresentation::render(
    ScreenBuffer& buffer,
    const TerminalListViewPresentationSnapshot& presentation,
    AmbiguousWidthMode ambiguous_width) {
    if (presentation.bounds.width < 0 || presentation.bounds.height < 0 ||
        presentation.rows.size() > static_cast<std::size_t>(presentation.bounds.height)) {
        return false;
    }

    // Preflight the complete visible transaction before touching the caller-owned buffer. A malformed
    // later row must not leave earlier rows from the same frame half-applied.
    for (const auto& row : presentation.rows) {
        const TextMeasurement measurement = TextMetrics::measureUtf8(row.text, ambiguous_width);
        if (!measurement.simpleCellRenderable() || measurement.rows != 1) {
            return false;
        }
    }

    for (const auto& row : presentation.rows) {
        const auto offset = static_cast<std::size_t>(&row - presentation.rows.data());
        const Rect row_bounds{presentation.bounds.x,
                              static_cast<Coordinate>(static_cast<std::int64_t>(presentation.bounds.y) +
                                                      static_cast<std::int64_t>(offset)),
                              presentation.bounds.width,
                              1};
        TextStyle style{};
        if (row.selected) {
            style.inverse = true;
        }
        if (presentation.focused && row.selected) {
            style.bold = true;
        }
        clear(buffer, row_bounds);

        std::int64_t x = row_bounds.x;
        for (std::size_t input = 0; input < row.text.size();) {
            const auto decoded = TextMetrics::decodeOne(row.text, input);
            if (decoded.consumed == 0) {
                break;
            }
            input += decoded.consumed;
            const int width = TextMetrics::codePointWidth(decoded.value, ambiguous_width);
            if (width <= 0) {
                continue;
            }
            if (x - row_bounds.x + width > row_bounds.width) {
                break;
            }
            writeScalar(buffer, x, row_bounds.y, decoded.value, width, style);
            x += width;
        }
    }
    return true;
}

std::optional<std::size_t>
TerminalListViewPresentation::rowAt(const TerminalListViewPresentationSnapshot& presentation,
                                    Point point) noexcept {
    if (presentation.bounds.isEmpty() || !presentation.bounds.contains(point) ||
        presentation.rows.size() > static_cast<std::size_t>(presentation.bounds.height)) {
        return std::nullopt;
    }
    const auto offset = static_cast<std::size_t>(
        static_cast<std::int64_t>(point.y) - static_cast<std::int64_t>(presentation.bounds.y));
    if (offset >= presentation.rows.size()) {
        return std::nullopt;
    }
    return presentation.rows[offset].row;
}

bool TerminalListViewPresentation::selectAt(
    const TerminalListViewPresentationSnapshot& presentation,
    Point point,
    ListSelectionModel& selection) noexcept {
    const auto row = rowAt(presentation, point);
    const auto* model = selection.model();
    if (!row.has_value() || model == nullptr || model->revision() != presentation.model_revision) {
        // The frame no longer describes the model currently bound to the selection. Rejecting the
        // click is safer than applying an old coordinate to a newly shifted row.
        return false;
    }
    return selection.select(*row);
}

} // namespace sasd::ui::terminal
