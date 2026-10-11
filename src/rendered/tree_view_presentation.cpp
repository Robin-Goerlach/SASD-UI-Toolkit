#include <sasd/ui/rendered/tree_view_presentation.hpp>

#include <limits>
#include <utility>

namespace sasd::ui::rendered {
namespace {

constexpr std::int64_t kIndentWidth = 16;
constexpr Coordinate kIndicatorWidth = 12;

[[nodiscard]] bool coordinateFits(std::int64_t value) noexcept {
    return value >= std::numeric_limits<Coordinate>::min() &&
           value <= std::numeric_limits<Coordinate>::max();
}

[[nodiscard]] bool validSnapshot(const RenderedTreeViewPresentationSnapshot& snapshot) noexcept {
    if (snapshot.bounds.isEmpty() || snapshot.rows.size() >
                                      static_cast<std::size_t>(snapshot.bounds.height)) {
        return false;
    }
    for (std::size_t index = 0; index < snapshot.rows.size(); ++index) {
        const auto& row = snapshot.rows[index];
        if (row.node.empty() || row.depth != row.node.depth() ||
            row.depth > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max() /
                                                 kIndentWidth) ||
            row.bounds.width <= 0 || row.bounds.height != 1 || row.bounds.x != snapshot.bounds.x ||
            row.bounds.width != snapshot.bounds.width) {
            return false;
        }
        const auto depth_offset = static_cast<std::int64_t>(row.depth) * kIndentWidth;
        if (depth_offset > std::numeric_limits<std::int64_t>::max() - kIndentWidth -
                               static_cast<std::int64_t>(row.bounds.x)) {
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

} // namespace

std::optional<RenderedTreeViewPresentationSnapshot>
RenderedTreeViewPresentation::snapshot(const TreeView& view, Rect bounds) {
    if (bounds.width <= 0 || bounds.height <= 0 || view.model() == nullptr) {
        return std::nullopt;
    }
    const auto rows = view.visibleRows();
    if (rows.size() > static_cast<std::size_t>(bounds.height)) {
        return std::nullopt;
    }

    RenderedTreeViewPresentationSnapshot result;
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
        const auto y = static_cast<std::int64_t>(bounds.y) + static_cast<std::int64_t>(index);
        const auto indicator_x = static_cast<std::int64_t>(bounds.x) +
                                 static_cast<std::int64_t>(source.depth) * kIndentWidth;
        if (!coordinateFits(y) || !coordinateFits(indicator_x)) {
            return std::nullopt;
        }
        result.rows.push_back({source.node,
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
                               source.selected});
    }
    return validSnapshot(result) ? std::optional{std::move(result)} : std::nullopt;
}

bool RenderedTreeViewPresentation::render(
    DisplayList& display_list,
    const RenderedTreeViewPresentationSnapshot& snapshot,
    Color background_color) {
    if (!validSnapshot(snapshot)) {
        return false;
    }
    DisplayList staged = display_list;
    staged.fillRect(snapshot.bounds, background_color);
    for (const auto& row : snapshot.rows) {
        TextStyle style{};
        style.inverse = row.selected;
        style.bold = snapshot.focused && row.selected;
        if (row.has_children) {
            const auto indicator = row.expanded ? "[-]" : "[+]";
            staged.drawText({row.indicator_bounds.x, row.indicator_bounds.y}, indicator, style,
                            row.indicator_bounds);
        }
        const auto text_x = static_cast<std::int64_t>(row.bounds.x) +
                            static_cast<std::int64_t>(row.depth) * kIndentWidth + kIndentWidth;
        if (!coordinateFits(text_x)) {
            return false;
        }
        staged.drawText({static_cast<Coordinate>(text_x), row.bounds.y}, row.text, style,
                        row.bounds);
    }
    display_list = std::move(staged);
    return true;
}

std::optional<RenderedTreeViewHit>
RenderedTreeViewPresentation::hitAt(const RenderedTreeViewPresentationSnapshot& snapshot,
                                    Point point) noexcept {
    if (!validSnapshot(snapshot)) {
        return std::nullopt;
    }
    for (const auto& row : snapshot.rows) {
        if (row.indicator_bounds.contains(point)) {
            return RenderedTreeViewHit{row.node, RenderedTreeViewHitKind::indicator};
        }
        if (row.bounds.contains(point)) {
            return RenderedTreeViewHit{row.node, RenderedTreeViewHitKind::row};
        }
    }
    return std::nullopt;
}

bool RenderedTreeViewPresentation::selectAt(
    const RenderedTreeViewPresentationSnapshot& snapshot,
    Point point,
    TreeSelectionModel& selection) noexcept {
    const auto hit = hitAt(snapshot, point);
    const auto* model = selection.model();
    if (!hit.has_value() || hit->kind != RenderedTreeViewHitKind::row || model == nullptr ||
        model->revision() != snapshot.model_revision) {
        return false;
    }
    return selection.select(hit->node);
}

bool RenderedTreeViewPresentation::toggleAt(
    const RenderedTreeViewPresentationSnapshot& snapshot,
    Point point,
    TreeView& view) noexcept {
    const auto hit = hitAt(snapshot, point);
    const auto* model = view.model();
    if (!hit.has_value() || hit->kind != RenderedTreeViewHitKind::indicator || model == nullptr ||
        model->revision() != snapshot.model_revision) {
        return false;
    }
    return view.toggleExpanded(hit->node);
}

} // namespace sasd::ui::rendered
