#include <sasd/ui/form_layout.hpp>

#include <sasd/ui/measurement_context.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace sasd::ui {
namespace {

/*
 * FormLayout intentionally owns its private measurement machinery instead of sharing an internal
 * helper layer with GridLayout. The two layouts currently have different structural semantics:
 * GridLayout packs visible children densely, while FormLayout preserves stable label/field pairs.
 * Keeping these helpers local avoids turning incidental arithmetic similarities into a premature
 * abstraction that later span/alignment work would have to preserve.
 */
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
     * A form row shares the parent's available extent between multiple cells. Forwarding the parent
     * minimum to each individual cell would make both label and field claim the complete minimum.
     * Children therefore receive a zero minimum while retaining the parent's maximum as a safe
     * per-child ceiling.
     */
    return {{0, 0}, parent.maximum};
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

} // namespace

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
