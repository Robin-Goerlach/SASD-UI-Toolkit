#include <sasd/ui/grid_layout.hpp>

#include <sasd/ui/measurement_context.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
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
     * Multi-child layouts share the parent extent. Passing the parent's minimum to each child would
     * make every cell independently claim the complete parent minimum. Children therefore receive a
     * zero minimum and only inherit the parent's maximum as a conservative per-child ceiling.
     */
    return {{0, 0}, parent.maximum};
}

template <typename MeasureChild>
[[nodiscard]] GridMetrics collectGridMetrics(
    Container& grid,
    std::size_t columns,
    const MeasureConstraints& constraints,
    MeasureChild&& measure_child) {
    GridMetrics metrics;
    metrics.column_widths.assign(columns, 0);

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

        if (row >= metrics.row_heights.size()) {
            metrics.row_heights.push_back(0);
        }

        metrics.column_widths[column] =
            std::max(metrics.column_widths[column], desired.width);
        metrics.row_heights[row] =
            std::max(metrics.row_heights[row], desired.height);
        ++visible_index;
    }

    /*
     * Occupancy is defined by row-major cell assignment, not by measured extent. A visible child
     * whose desired width is zero still occupies its column and therefore still establishes the
     * spacing boundary to the previous occupied column. Trimming by trailing zero values would
     * incorrectly conflate "occupied zero-width track" with "no track at all".
     *
     * Dense placement guarantees that the occupied columns are always a prefix of the configured
     * column set, so the number of columns to retain is determined solely by how many visible cells
     * were assigned. Empty columns after a partial final row are the only columns removed here.
     */
    metrics.column_widths.resize(std::min(columns, visible_index));

    return metrics;
}

[[nodiscard]] Coordinate sumTracks(
    const std::vector<Coordinate>& tracks,
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

/**
 * Computes GridLayout's intrinsic result.
 *
 * The explicit grid prefix is intentional. Widget already exposes desiredSize(), and an unqualified
 * helper named desiredSize() inside a GridLayout member is hidden by that inherited member during C++
 * name lookup. A distinct implementation-helper name avoids relying on subtle lookup rules and keeps
 * warnings-as-errors builds identical across GCC, Clang and MSVC.
 */
[[nodiscard]] Size gridDesiredSize(
    const GridMetrics& metrics,
    Coordinate column_spacing,
    Coordinate row_spacing) noexcept {
    return {
        sumTracks(metrics.column_widths, column_spacing),
        sumTracks(metrics.row_heights, row_spacing)};
}

/**
 * Clips intrinsic tracks against final space in deterministic track order.
 *
 * This intentionally mirrors the first VBox/HBox shortage policy: earlier tracks keep their desired
 * extent first, while later tracks receive only what remains. Extra space is not distributed yet.
 */
[[nodiscard]] std::vector<Coordinate> arrangedTracks(
    const std::vector<Coordinate>& intrinsic,
    Coordinate spacing,
    Coordinate final_extent) {
    std::vector<Coordinate> result;
    result.reserve(intrinsic.size());

    Coordinate cursor = 0;
    for (std::size_t index = 0; index < intrinsic.size(); ++index) {
        const Coordinate extent =
            std::min(intrinsic[index], remaining(final_extent, cursor));
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
        collectGridMetrics(
            *this,
            columns_,
            constraints,
            [](Widget& child,
               const MeasureConstraints& child_constraints) {
                return child.measure(child_constraints);
            });

    return gridDesiredSize(metrics, column_spacing_, row_spacing_);
}

Size GridLayout::onMeasure(
    const MeasurementContext& context,
    const MeasureConstraints& constraints) {
    const GridMetrics metrics =
        collectGridMetrics(
            *this,
            columns_,
            constraints,
            [&context](Widget& child,
                       const MeasureConstraints& child_constraints) {
                return child.measure(context, child_constraints);
            });

    return gridDesiredSize(metrics, column_spacing_, row_spacing_);
}

void GridLayout::onArrange(Rect final_bounds) {
    /*
     * Rebuild track maxima from the children's last desiredSize() values. Layout therefore follows
     * the normal measure-before-arrange lifecycle and does not retain a MeasurementContext.
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

    /*
     * Keep exactly the columns that received at least one visible cell. A zero-width child still
     * occupies a logical column; preserving that track keeps column spacing and child x origins
     * identical between measurement and arrangement.
     */
    metrics.column_widths.resize(std::min(columns_, visible_index));

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
         * Later tracks can be fully exhausted. Arrange their children into deterministic zero-extent
         * cells rather than leaving stale geometry from a previous larger layout pass.
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
