#include <sasd/ui/grid_layout.hpp>

#include <sasd/ui/measurement_context.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sasd::ui {
namespace {

struct GridMetrics {
    std::vector<Coordinate> column_widths;
    std::vector<Coordinate> row_heights;
};

[[nodiscard]] Coordinate saturatingAdd(Coordinate left, Coordinate right) noexcept {
    const Coordinate maximum = std::numeric_limits<Coordinate>::max();

    if (right <= 0) {
        return left;
    }
    if (left > maximum - right) {
        return maximum;
    }
    return static_cast<Coordinate>(left + right);
}

[[nodiscard]] Coordinate remaining(Coordinate total, Coordinate cursor) noexcept {
    return cursor >= total ? 0 : static_cast<Coordinate>(total - cursor);
}

[[nodiscard]] MeasureConstraints childConstraints(
    const MeasureConstraints& parent) noexcept {
    /*
     * A grid shares the parent extent across several cells. Passing the parent's minimum to every
     * child would make each individual cell claim the full minimum. As with VBox/HBox, children get a
     * zero minimum and inherit only the parent's maximum as a conservative per-child ceiling.
     */
    return {{0, 0}, parent.maximum};
}

template <typename MeasureChild>
[[nodiscard]] GridMetrics collectMetrics(Container& grid,
                                         std::size_t columns,
                                         const MeasureConstraints& constraints,
                                         MeasureChild&& measure_child) {
    GridMetrics result;
    result.column_widths.assign(columns, 0);

    std::size_t visible_index = 0;
    for (std::size_t index = 0; index < grid.childCount(); ++index) {
        Widget& child = grid.childAt(index);
        if (!child.isVisible()) {
            continue;
        }

        const Size desired =
            measure_child(child, childConstraints(constraints));

        const std::size_t column = visible_index % columns;
        const std::size_t row = visible_index / columns;

        if (row >= result.row_heights.size()) {
            result.row_heights.push_back(0);
        }

        result.column_widths[column] =
            std::max(result.column_widths[column], desired.width);
        result.row_heights[row] =
            std::max(result.row_heights[row], desired.height);
        ++visible_index;
    }

    /*
     * If the last logical columns are empty, keeping zero-width tracks would still insert spacing
     * after real content. Trim only trailing empty columns; interior empty columns cannot occur in the
     * row-major no-span model because visible children fill cells densely.
     */
    while (!result.column_widths.empty() &&
           result.column_widths.back() == 0) {
        result.column_widths.pop_back();
    }

    return result;
}

[[nodiscard]] Coordinate sumTracks(const std::vector<Coordinate>& tracks,
                                   Coordinate spacing) noexcept {
    Coordinate total = 0;

    for (std::size_t index = 0; index < tracks.size(); ++index) {
        if (index != 0U) {
            total = saturatingAdd(total, spacing);
        }
        total = saturatingAdd(total, tracks[index]);
    }

    return total;
}

[[nodiscard]] Size desiredSize(const GridMetrics& metrics,
                               Coordinate column_spacing,
                               Coordinate row_spacing) noexcept {
    return {
        sumTracks(metrics.column_widths, column_spacing),
        sumTracks(metrics.row_heights, row_spacing)};
}

/**
 * Clips intrinsic track sizes against the final extent in deterministic track order.
 *
 * GridLayout deliberately mirrors VBox/HBox's current shortage policy: earlier tracks keep their
 * intrinsic size first; later tracks receive only the remaining extent. Extra space is not distributed
 * yet. That keeps the initial grid deterministic until M4 gains an explicit stretch/weight contract.
 */
[[nodiscard]] std::vector<Coordinate> arrangedTracks(
    const std::vector<Coordinate>& intrinsic,
    Coordinate spacing,
    Coordinate final_extent) {
    std::vector<Coordinate> result;
    result.reserve(intrinsic.size());

    Coordinate cursor = 0;
    for (std::size_t index = 0; index < intrinsic.size(); ++index) {
        const Coordinate available = remaining(final_extent, cursor);
        const Coordinate extent = std::min(intrinsic[index], available);
        result.push_back(extent);
        cursor = saturatingAdd(cursor, extent);

        if (index + 1U < intrinsic.size()) {
            cursor = saturatingAdd(
                cursor,
                std::min(spacing, remaining(final_extent, cursor)));
        }
    }

    return result;
}

[[nodiscard]] std::vector<Coordinate> trackOrigins(
    const std::vector<Coordinate>& extents,
    Coordinate spacing,
    Coordinate final_extent) {
    std::vector<Coordinate> origins;
    origins.reserve(extents.size());

    Coordinate cursor = 0;
    for (std::size_t index = 0; index < extents.size(); ++index) {
        origins.push_back(cursor);
        cursor = saturatingAdd(cursor, extents[index]);

        if (index + 1U < extents.size()) {
            cursor = saturatingAdd(
                cursor,
                std::min(spacing, remaining(final_extent, cursor)));
        }
    }

    return origins;
}

} // namespace

GridLayout::GridLayout(std::size_t columns)
    : columns_{columns} {
    if (columns == 0U) {
        throw std::invalid_argument(
            "GridLayout requires at least one column");
    }
}

void GridLayout::setColumnCount(std::size_t columns) {
    if (columns == 0U) {
        throw std::invalid_argument(
            "GridLayout::setColumnCount requires at least one column");
    }
    if (columns_ == columns) {
        return;
    }

    columns_ = columns;
    invalidateMeasure();
}

void GridLayout::setColumnSpacing(Coordinate spacing) {
    if (spacing < 0) {
        throw std::invalid_argument(
            "GridLayout::setColumnSpacing requires non-negative spacing");
    }
    if (column_spacing_ == spacing) {
        return;
    }

    column_spacing_ = spacing;
    invalidateMeasure();
}

void GridLayout::setRowSpacing(Coordinate spacing) {
    if (spacing < 0) {
        throw std::invalid_argument(
            "GridLayout::setRowSpacing requires non-negative spacing");
    }
    if (row_spacing_ == spacing) {
        return;
    }

    row_spacing_ = spacing;
    invalidateMeasure();
}

Size GridLayout::onMeasure(const MeasureConstraints& constraints) {
    const GridMetrics metrics =
        collectMetrics(
            *this,
            columns_,
            constraints,
            [](Widget& child,
               const MeasureConstraints& child_constraints) {
                return child.measure(child_constraints);
            });

    return desiredSize(metrics, column_spacing_, row_spacing_);
}

Size GridLayout::onMeasure(const MeasurementContext& context,
                           const MeasureConstraints& constraints) {
    const GridMetrics metrics =
        collectMetrics(
            *this,
            columns_,
            constraints,
            [&context](Widget& child,
                       const MeasureConstraints& child_constraints) {
                return child.measure(context, child_constraints);
            });

    return desiredSize(metrics, column_spacing_, row_spacing_);
}

void GridLayout::onArrange(Rect final_bounds) {
    /*
     * Arrangement consumes the already measured desiredSize() values, preserving the same
     * measure-before-arrange lifecycle as VBox/HBox. No MeasurementContext is retained by the layout.
     */
    GridMetrics metrics;
    metrics.column_widths.assign(columns_, 0);

    std::size_t visible_index = 0;
    for (std::size_t index = 0; index < childCount(); ++index) {
        Widget& child = childAt(index);
        if (!child.isVisible()) {
            continue;
        }

        const std::size_t column = visible_index % columns_;
        const std::size_t row = visible_index / columns_;

        if (row >= metrics.row_heights.size()) {
            metrics.row_heights.push_back(0);
        }

        const Size desired = child.desiredSize();
        metrics.column_widths[column] =
            std::max(metrics.column_widths[column], desired.width);
        metrics.row_heights[row] =
            std::max(metrics.row_heights[row], desired.height);
        ++visible_index;
    }

    while (!metrics.column_widths.empty() &&
           metrics.column_widths.back() == 0) {
        metrics.column_widths.pop_back();
    }

    const std::vector<Coordinate> column_widths =
        arrangedTracks(
            metrics.column_widths,
            column_spacing_,
            final_bounds.width);
    const std::vector<Coordinate> row_heights =
        arrangedTracks(
            metrics.row_heights,
            row_spacing_,
            final_bounds.height);

    const std::vector<Coordinate> column_origins =
        trackOrigins(
            column_widths,
            column_spacing_,
            final_bounds.width);
    const std::vector<Coordinate> row_origins =
        trackOrigins(
            row_heights,
            row_spacing_,
            final_bounds.height);

    visible_index = 0;
    for (std::size_t index = 0; index < childCount(); ++index) {
        Widget& child = childAt(index);
        if (!child.isVisible()) {
            continue;
        }

        const std::size_t column = visible_index % columns_;
        const std::size_t row = visible_index / columns_;
        ++visible_index;

        /*
         * A row/column can be completely clipped once final space is exhausted. Keep arranging later
         * visible children into deterministic zero-extent cells instead of leaving stale geometry from
         * a previous larger arrangement.
         */
        const Coordinate x =
            column < column_origins.size()
                ? column_origins[column]
                : final_bounds.width;
        const Coordinate y =
            row < row_origins.size()
                ? row_origins[row]
                : final_bounds.height;
        const Coordinate width =
            column < column_widths.size()
                ? column_widths[column]
                : 0;
        const Coordinate height =
            row < row_heights.size()
                ? row_heights[row]
                : 0;

        child.arrange({x, y, width, height});
    }
}

} // namespace sasd::ui
