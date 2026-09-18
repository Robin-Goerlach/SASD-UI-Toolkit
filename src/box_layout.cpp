#include <sasd/ui/hbox.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/vbox.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace sasd::ui {
namespace {

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

[[nodiscard]] std::size_t visibleChildCount(const Container& container) noexcept {
    std::size_t visible = 0;
    for (std::size_t index = 0; index < container.childCount(); ++index) {
        if (container.childAt(index).isVisible()) {
            ++visible;
        }
    }
    return visible;
}

[[nodiscard]] MeasureConstraints childConstraints(const MeasureConstraints& parent) noexcept {
    /*
     * A box does not impose the parent's minimum on every child; doing so would make two children each
     * demand the full parent minimum. The parent's maximum is still a useful per-child ceiling, while
     * the Box's own final result is clamped by Widget::measure() afterwards.
     */
    return {{0, 0}, parent.maximum};
}

template <typename MeasureChild>
[[nodiscard]] Size measureVBox(Container& box,
                               Coordinate spacing,
                               const MeasureConstraints& constraints,
                               MeasureChild&& measure_child) {
    Coordinate width = 0;
    Coordinate height = 0;
    bool first_visible = true;

    for (std::size_t index = 0; index < box.childCount(); ++index) {
        Widget& child = box.childAt(index);
        if (!child.isVisible()) {
            continue;
        }

        const Size desired = measure_child(child, childConstraints(constraints));

        width = std::max(width, desired.width);
        if (!first_visible) {
            height = saturatingAdd(height, spacing);
        }
        height = saturatingAdd(height, desired.height);
        first_visible = false;
    }

    return {width, height};
}

template <typename MeasureChild>
[[nodiscard]] Size measureHBox(Container& box,
                               Coordinate spacing,
                               const MeasureConstraints& constraints,
                               MeasureChild&& measure_child) {
    Coordinate width = 0;
    Coordinate height = 0;
    bool first_visible = true;

    for (std::size_t index = 0; index < box.childCount(); ++index) {
        Widget& child = box.childAt(index);
        if (!child.isVisible()) {
            continue;
        }

        const Size desired = measure_child(child, childConstraints(constraints));

        if (!first_visible) {
            width = saturatingAdd(width, spacing);
        }
        width = saturatingAdd(width, desired.width);
        height = std::max(height, desired.height);
        first_visible = false;
    }

    return {width, height};
}

[[nodiscard]] Coordinate remaining(Coordinate total, Coordinate cursor) noexcept {
    return cursor >= total ? 0 : static_cast<Coordinate>(total - cursor);
}

} // namespace

void VBox::setSpacing(Coordinate spacing) {
    if (spacing < 0) {
        throw std::invalid_argument("VBox::setSpacing requires non-negative spacing");
    }
    if (spacing_ == spacing) {
        return;
    }

    spacing_ = spacing;
    invalidateMeasure();
}

Size VBox::onMeasure(const MeasureConstraints& constraints) {
    return measureVBox(*this, spacing_, constraints,
                       [](Widget& child, const MeasureConstraints& child_constraints) {
                           return child.measure(child_constraints);
                       });
}

Size VBox::onMeasure(const MeasurementContext& context,
                     const MeasureConstraints& constraints) {
    return measureVBox(*this, spacing_, constraints,
                       [&context](Widget& child, const MeasureConstraints& child_constraints) {
                           return child.measure(context, child_constraints);
                       });
}

void VBox::onArrange(Rect final_bounds) {
    Coordinate cursor = 0;
    std::size_t processed_visible = 0;
    const std::size_t visible_count = visibleChildCount(*this);

    for (std::size_t index = 0; index < childCount(); ++index) {
        Widget& child = childAt(index);
        if (!child.isVisible()) {
            continue;
        }

        const Coordinate available = remaining(final_bounds.height, cursor);
        const Coordinate height = std::min(child.desiredSize().height, available);

        /*
         * Child coordinates are relative to this Container, not to final_bounds.x/y. The terminal and
         * future desktop presentation layers resolve the complete visual-parent offset chain.
         */
        child.arrange({0, cursor, final_bounds.width, height});
        cursor = saturatingAdd(cursor, height);
        ++processed_visible;

        if (processed_visible < visible_count) {
            cursor = saturatingAdd(cursor, std::min(spacing_, remaining(final_bounds.height, cursor)));
        }
    }
}

void HBox::setSpacing(Coordinate spacing) {
    if (spacing < 0) {
        throw std::invalid_argument("HBox::setSpacing requires non-negative spacing");
    }
    if (spacing_ == spacing) {
        return;
    }

    spacing_ = spacing;
    invalidateMeasure();
}

Size HBox::onMeasure(const MeasureConstraints& constraints) {
    return measureHBox(*this, spacing_, constraints,
                       [](Widget& child, const MeasureConstraints& child_constraints) {
                           return child.measure(child_constraints);
                       });
}

Size HBox::onMeasure(const MeasurementContext& context,
                     const MeasureConstraints& constraints) {
    return measureHBox(*this, spacing_, constraints,
                       [&context](Widget& child, const MeasureConstraints& child_constraints) {
                           return child.measure(context, child_constraints);
                       });
}

void HBox::onArrange(Rect final_bounds) {
    Coordinate cursor = 0;
    std::size_t processed_visible = 0;
    const std::size_t visible_count = visibleChildCount(*this);

    for (std::size_t index = 0; index < childCount(); ++index) {
        Widget& child = childAt(index);
        if (!child.isVisible()) {
            continue;
        }

        const Coordinate available = remaining(final_bounds.width, cursor);
        const Coordinate width = std::min(child.desiredSize().width, available);

        child.arrange({cursor, 0, width, final_bounds.height});
        cursor = saturatingAdd(cursor, width);
        ++processed_visible;

        if (processed_visible < visible_count) {
            cursor = saturatingAdd(cursor, std::min(spacing_, remaining(final_bounds.width, cursor)));
        }
    }
}

} // namespace sasd::ui
