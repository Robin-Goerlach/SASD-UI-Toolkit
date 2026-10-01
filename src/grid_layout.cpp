#include <sasd/ui/form_layout.hpp>
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

struct FormRowMetric {
    Coordinate height{0};
    bool active{false};
};

struct FormMetrics {
    Coordinate label_width{0};
    Coordinate field_width{0};
    bool has_visible_label{false};
    bool has_visible_field{false};
    std::vector<FormRowMetric> rows;
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
     * Dense row-major placement cannot create an interior empty column. Trailing empty columns can
     * occur in the final partial row and must not manufacture spacing after the final occupied track.
     */
    while (!metrics.column_widths.empty() &&
           metrics.column_widths.back() == 0) {
        metrics.column_widths.pop_back();
    }

    return metrics;
}

template <typename MeasureChild>
[[nodiscard]] FormMetrics collectFormMetrics(
    FormLayout& form,
    const MeasureConstraints& constraints,
    MeasureChild&& measure_child) {
    FormMetrics metrics;
    metrics.rows.reserve(form.rowCount());

    for (std::size_t row = 0; row < form.rowCount(); ++row) {
        const std::size_t label_index = row * 2U;
        const std::size_t field_index = label_index + 1U;

        Widget& label = form.childAt(label_index);
        Widget* field =
            field_index < form.childCount()
                ? &form.childAt(field_index)
                : nullptr;

        FormRowMetric row_metric;

        if (label.isVisible()) {
            const Size desired =
                measure_child(label, childConstraints(constraints));
            metrics.label_width =
                std::max(metrics.label_width, desired.width);
            row_metric.height =
                std::max(row_metric.height, desired.height);
            row_metric.active = true;
            metrics.has_visible_label = true;
        }

        if (field != nullptr && field->isVisible()) {
            const Size desired =
                measure_child(*field, childConstraints(constraints));
            metrics.field_width =
                std::max(metrics.field_width, desired.width);
            row_metric.height =
                std::max(row_metric.height, desired.height);
            row_metric.active = true;
            metrics.has_visible_field = true;
        }

        /*
         * Preserve one metric entry per structural pair even when the complete row is hidden. Pair
         * identity must never be repacked from the currently visible children.
         */
        metrics.rows.push_back(row_metric);
    }

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

[[nodiscard]] Size formDesiredSize(
    const FormMetrics& metrics,
    Coordinate column_spacing,
    Coordinate row_spacing) noexcept {
    Coordinate width = metrics.label_width;
    if (metrics.has_visible_label && metrics.has_visible_field) {
        width = saturatingAdd(width, column_spacing);
    }
    if (metrics.has_visible_field) {
        width = saturatingAdd(width, metrics.field_width);
    }

    Coordinate height = 0;
    bool first_active = true;
    for (const FormRowMetric& row : metrics.rows) {
        if (!row.active) {
            continue;
        }
        if (!first_active) {
            height = saturatingAdd(height, row_spacing);
        }
        height = saturatingAdd(height, row.height);
        first_active = false;
    }

    return {width, height};
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

void FormLayout::setColumnSpacing(Coordinate spacing) {
    if (spacing < 0) {
        throw std::invalid_argument(
            "FormLayout::setColumnSpacing requires non-negative spacing");
    }
    if (column_spacing_ == spacing) {
        return;
    }

    column_spacing_ = spacing;
    invalidateMeasure();
}

void FormLayout::setRowSpacing(Coordinate spacing) {
    if (spacing < 0) {
        throw std::invalid_argument(
            "FormLayout::setRowSpacing requires non-negative spacing");
    }
    if (row_spacing_ == spacing) {
        return;
    }

    row_spacing_ = spacing;
    invalidateMeasure();
}

Size FormLayout::onMeasure(const MeasureConstraints& constraints) {
    const FormMetrics metrics =
        collectFormMetrics(
            *this,
            constraints,
            [](Widget& child,
               const MeasureConstraints& child_constraints) {
                return child.measure(child_constraints);
            });

    return formDesiredSize(metrics, column_spacing_, row_spacing_);
}

Size FormLayout::onMeasure(
    const MeasurementContext& context,
    const MeasureConstraints& constraints) {
    const FormMetrics metrics =
        collectFormMetrics(
            *this,
            constraints,
            [&context](Widget& child,
                       const MeasureConstraints& child_constraints) {
                return child.measure(context, child_constraints);
            });

    return formDesiredSize(metrics, column_spacing_, row_spacing_);
}

void FormLayout::onArrange(Rect final_bounds) {
    /*
     * Reuse the measurement collector with each child's cached desired size. Structural row pairing is
     * therefore identical in measure and arrange, including partially hidden and label-only rows.
     */
    const FormMetrics metrics =
        collectFormMetrics(
            *this,
            {},
            [](Widget& child, const MeasureConstraints&) {
                return child.desiredSize();
            });

    const Coordinate label_width =
        metrics.has_visible_label
            ? std::min(metrics.label_width, final_bounds.width)
            : 0;

    Coordinate field_x = label_width;
    if (metrics.has_visible_label && metrics.has_visible_field) {
        field_x = saturatingAdd(
            field_x,
            std::min(
                column_spacing_,
                remaining(final_bounds.width, field_x)));
    }

    /*
     * This is FormLayout's intentional specialization over GridLayout: the label track remains
     * intrinsic while the field track consumes all remaining horizontal space.
     */
    const Coordinate field_width =
        metrics.has_visible_field
            ? remaining(final_bounds.width, field_x)
            : 0;

    std::size_t active_rows = 0;
    for (const FormRowMetric& row : metrics.rows) {
        if (row.active) {
            ++active_rows;
        }
    }

    Coordinate y = 0;
    std::size_t processed_active = 0;

    for (std::size_t row = 0; row < metrics.rows.size(); ++row) {
        const FormRowMetric& row_metric = metrics.rows[row];
        if (!row_metric.active) {
            continue;
        }

        const Coordinate row_height =
            std::min(
                row_metric.height,
                remaining(final_bounds.height, y));

        const std::size_t label_index = row * 2U;
        const std::size_t field_index = label_index + 1U;
        Widget& label = childAt(label_index);

        if (label.isVisible()) {
            label.arrange({0, y, label_width, row_height});
        }

        if (field_index < childCount()) {
            Widget& field = childAt(field_index);
            if (field.isVisible()) {
                field.arrange({field_x, y, field_width, row_height});
            }
        }

        y = saturatingAdd(y, row_height);
        ++processed_active;

        if (processed_active < active_rows) {
            y = saturatingAdd(
                y,
                std::min(
                    row_spacing_,
                    remaining(final_bounds.height, y)));
        }
    }
}

} // namespace sasd::ui
