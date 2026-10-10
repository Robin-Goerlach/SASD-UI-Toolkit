#include <sasd/ui/terminal/tree_view_presentation.hpp>

#include <algorithm>
#include <limits>

namespace sasd::ui::terminal {
namespace {

constexpr std::int64_t kIndentWidth = 4;
constexpr Coordinate kIndicatorWidth = 3;

[[nodiscard]] bool coordinateFits(std::int64_t value) noexcept {
    return value >= std::numeric_limits<Coordinate>::min() &&
           value <= std::numeric_limits<Coordinate>::max();
}

[[nodiscard]] bool validSnapshot(const TerminalTreeViewPresentationSnapshot& snapshot) noexcept {
    if (snapshot.bounds.isEmpty() || snapshot.rows.size() >
                                      static_cast<std::size_t>(snapshot.bounds.height)) {
        return false;
    }
    for (std::size_t index = 0; index < snapshot.rows.size(); ++index) {
        const auto& row = snapshot.rows[index];
        if (row.node.empty() || row.depth != row.node.depth() ||
            row.depth > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max() /
                                                 kIndentWidth) ||
            row.bounds.width <= 0 ||
            row.bounds.height != 1 || row.bounds.x != snapshot.bounds.x ||
            row.bounds.width != snapshot.bounds.width) {
            return false;
        }
        const auto expected_y = static_cast<std::int64_t>(snapshot.bounds.y) +
                                static_cast<std::int64_t>(index);
        if (!coordinateFits(expected_y) || row.bounds.y != static_cast<Coordinate>(expected_y) ||
            !snapshot.bounds.contains({row.bounds.x, row.bounds.y})) {
            return false;
        }
        if (!row.has_children && !row.indicator_bounds.isEmpty()) {
            return false;
        }
        if (row.has_children &&
            (row.indicator_bounds.width != kIndicatorWidth || row.indicator_bounds.height != 1 ||
             !row.bounds.contains({row.indicator_bounds.x, row.indicator_bounds.y}) ||
             !row.bounds.contains({static_cast<Coordinate>(
                                       static_cast<std::int64_t>(row.indicator_bounds.x) +
                                       row.indicator_bounds.width - 1),
                                   row.indicator_bounds.y}))) {
            return false;
        }
    }
    return true;
}

void writeScalar(ScreenBuffer& buffer,
                 std::int64_t x,
                 Coordinate y,
                 char32_t value,
                 int width,
                 TextStyle style) {
    const auto inside = [&](std::int64_t candidate_x) {
        return candidate_x >= std::numeric_limits<Coordinate>::min() &&
               candidate_x <= std::numeric_limits<Coordinate>::max() &&
               buffer.contains({static_cast<Coordinate>(candidate_x), y});
    };
    if (width == 1 && inside(x)) {
        buffer.set({static_cast<Coordinate>(x), y}, Cell{value, CellRole::normal, style});
    } else if (width == 2 && inside(x) && inside(x + 1)) {
        buffer.set({static_cast<Coordinate>(x), y}, Cell{value, CellRole::wide_lead, style});
        buffer.set({static_cast<Coordinate>(x + 1), y},
                   Cell{U' ', CellRole::wide_continuation, style});
    }
}

void drawText(ScreenBuffer& buffer,
              const TerminalTreeViewRow& row,
              TextStyle style,
              AmbiguousWidthMode ambiguous_width) {
    std::int64_t x = static_cast<std::int64_t>(row.bounds.x) +
                     static_cast<std::int64_t>(row.depth) * kIndentWidth +
                     kIndentWidth;
    const auto right = static_cast<std::int64_t>(row.bounds.x) + row.bounds.width;
    for (std::size_t offset = 0; offset < row.text.size();) {
        const auto decoded = TextMetrics::decodeOne(row.text, offset);
        if (decoded.consumed == 0) {
            break;
        }
        offset += decoded.consumed;
        const auto width = TextMetrics::codePointWidth(decoded.value, ambiguous_width);
        if (width <= 0 || x > right - width) {
            break;
        }
        writeScalar(buffer, x, row.bounds.y, decoded.value, width, style);
        x += width;
    }
}

} // namespace

std::optional<TerminalTreeViewPresentationSnapshot>
TerminalTreeViewPresentation::snapshot(const TreeView& view, Rect bounds) {
    if (bounds.width <= 0 || bounds.height <= 0 || view.model() == nullptr) {
        return std::nullopt;
    }
    const auto rows = view.visibleRows();
    if (rows.size() > static_cast<std::size_t>(bounds.height)) {
        return std::nullopt;
    }

    TerminalTreeViewPresentationSnapshot result;
    result.bounds = bounds;
    result.focused = view.hasFocus();
    result.model_revision = view.model()->revision();
    result.rows.reserve(rows.size());
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto& source = rows[index];
        if (source.depth > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max() /
                                                     kIndentWidth)) {
            return std::nullopt;
        }
        const auto indent = static_cast<std::int64_t>(source.depth) * kIndentWidth;
        const auto y = static_cast<std::int64_t>(bounds.y) + static_cast<std::int64_t>(index);
        const auto indicator_x = static_cast<std::int64_t>(bounds.x) + indent;
        if (!coordinateFits(y) || !coordinateFits(indicator_x)) {
            return std::nullopt;
        }
        TerminalTreeViewRow row{source.node,
                                source.depth,
                                source.text,
                                {bounds.x, static_cast<Coordinate>(y), bounds.width, 1},
                                source.has_children
                                    ? Rect{static_cast<Coordinate>(indicator_x),
                                           static_cast<Coordinate>(y),
                                           kIndicatorWidth,
                                           1}
                                    : Rect{},
                                source.has_children,
                                source.expanded,
                                source.selected};
        result.rows.push_back(std::move(row));
    }
    return validSnapshot(result) ? std::optional{std::move(result)} : std::nullopt;
}

bool TerminalTreeViewPresentation::render(
    ScreenBuffer& buffer,
    const TerminalTreeViewPresentationSnapshot& snapshot,
    AmbiguousWidthMode ambiguous_width) {
    if (!validSnapshot(snapshot)) {
        return false;
    }
    for (const auto& row : snapshot.rows) {
        const auto measurement = TextMetrics::measureUtf8(row.text, ambiguous_width);
        if (!measurement.simpleCellRenderable() || measurement.rows != 1) {
            return false;
        }
    }

    ScreenBuffer staged = buffer;
    for (const auto& row : snapshot.rows) {
        staged.fill(row.bounds, Cell{});
        if (row.has_children) {
            TextStyle indicator_style{};
            indicator_style.inverse = row.selected;
            indicator_style.bold = snapshot.focused && row.selected;
            const auto indicator = row.expanded ? "[-]" : "[+]";
            for (std::size_t index = 0; index < 3; ++index) {
                staged.set({static_cast<Coordinate>(
                                static_cast<std::int64_t>(row.indicator_bounds.x) + index),
                            row.bounds.y},
                           Cell{static_cast<char32_t>(indicator[index]), CellRole::normal,
                                indicator_style});
            }
        }
        TextStyle text_style{};
        text_style.inverse = row.selected;
        text_style.bold = snapshot.focused && row.selected;
        drawText(staged, row, text_style, ambiguous_width);
    }
    buffer = std::move(staged);
    return true;
}

std::optional<TerminalTreeViewHit>
TerminalTreeViewPresentation::hitAt(const TerminalTreeViewPresentationSnapshot& snapshot,
                                    Point point) noexcept {
    if (!validSnapshot(snapshot)) {
        return std::nullopt;
    }
    for (const auto& row : snapshot.rows) {
        if (row.indicator_bounds.contains(point)) {
            return TerminalTreeViewHit{row.node, TerminalTreeViewHitKind::indicator};
        }
        if (row.bounds.contains(point)) {
            return TerminalTreeViewHit{row.node, TerminalTreeViewHitKind::row};
        }
    }
    return std::nullopt;
}

bool TerminalTreeViewPresentation::selectAt(
    const TerminalTreeViewPresentationSnapshot& snapshot,
    Point point,
    TreeSelectionModel& selection) noexcept {
    const auto hit = hitAt(snapshot, point);
    const auto* model = selection.model();
    if (!hit.has_value() || hit->kind != TerminalTreeViewHitKind::row || model == nullptr ||
        model->revision() != snapshot.model_revision) {
        return false;
    }
    return selection.select(hit->node);
}

bool TerminalTreeViewPresentation::toggleAt(
    const TerminalTreeViewPresentationSnapshot& snapshot,
    Point point,
    TreeView& view) noexcept {
    const auto hit = hitAt(snapshot, point);
    const auto* model = view.model();
    if (!hit.has_value() || hit->kind != TerminalTreeViewHitKind::indicator || model == nullptr ||
        model->revision() != snapshot.model_revision) {
        return false;
    }
    return view.toggleExpanded(hit->node);
}

} // namespace sasd::ui::terminal
