#include <sasd/ui/widget.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>

#include <stdexcept>

namespace sasd::ui {

Widget::~Widget() {
    /*
     * focus_manager_ is intentionally non-owning. The destruction handshake is what makes that raw
     * observation safe: a focused widget tells its still-live manager to forget the pointer before
     * the Widget storage disappears. No FocusEvent is emitted from a destructor because virtual
     * dispatch during object teardown would be surprising and unsafe.
     */
    if (focus_manager_ != nullptr) {
        focus_manager_->widgetDestroyed(*this);
    }
}

void Widget::setVisible(bool visible) {
    if (visible_ == visible) {
        return;
    }

    visible_ = visible;

    /*
     * M1 does not yet decide whether hidden widgets consume layout space. Nevertheless a visibility
     * change can affect whichever policy a future container uses, so invalidating here is the safe
     * backend-neutral behavior.
     */
    invalidateMeasure();
    clearFocusIfIneligible();
}

void Widget::setEnabled(bool enabled) {
    if (enabled_ == enabled) {
        return;
    }

    enabled_ = enabled;
    clearFocusIfIneligible();
}

void Widget::setFocusable(bool focusable) {
    if (focusable_ == focusable) {
        return;
    }

    focusable_ = focusable;
    clearFocusIfIneligible();
}

void Widget::setSizeConstraints(SizeConstraints constraints) {
    if (!constraints.hasValidRange()) {
        throw std::invalid_argument("Widget::setSizeConstraints requires minimum <= maximum");
    }

    if (size_constraints_ == constraints) {
        return;
    }

    size_constraints_ = constraints;
    invalidateMeasure();
}

Size Widget::measure(const MeasureConstraints& constraints) {
    if (!constraints.hasValidRange()) {
        throw std::invalid_argument(
            "Widget::measure requires non-negative, ordered MeasureConstraints");
    }

    /*
     * Measurement may be expensive once text shaping and native/backend metrics are involved. The
     * cache key is therefore the complete parent constraint interval. Widget-specific state changes
     * must call invalidateMeasure(), which also propagates staleness to the visual parent.
     */
    if (measure_valid_ && last_measure_constraints_ == constraints) {
        return desired_size_;
    }

    /*
     * A concrete widget reports its intrinsic/content desire. Its own size hints shape that result,
     * then the parent's offered range wins last. This order is intentional: a child may prefer or
     * even declare a minimum wider than a narrow terminal/window can actually provide.
     */
    const Size intrinsic = onMeasure(constraints);
    const Size widget_constrained = size_constraints_.clamp(intrinsic);
    desired_size_ = constraints.clamp(widget_constrained);
    last_measure_constraints_ = constraints;
    measure_valid_ = true;

    return desired_size_;
}

void Widget::arrange(Rect final_bounds) {
    if (final_bounds.width < 0 || final_bounds.height < 0) {
        throw std::invalid_argument("Widget::arrange requires non-negative extents");
    }

    /*
     * Store the final rectangle first. Container::onArrange implementations can then query bounds()
     * while assigning child rectangles. If a custom onArrange() throws, the widget intentionally
     * retains the requested final bounds; arrangement is an applied state transition, not a preview.
     */
    bounds_ = final_bounds;
    onArrange(final_bounds);
}

Size Widget::onMeasure(const MeasureConstraints&) {
    return size_constraints_.preferred;
}

void Widget::onArrange(Rect) {
    // Leaf/base widgets have no children to arrange.
}

void Widget::invalidateMeasure() noexcept {
    measure_valid_ = false;

    /*
     * Desired size of a descendant can affect every ancestor layout. Propagating through the visual
     * parent chain here keeps that dependency in the semantic tree rather than making VBox/Window or
     * a backend remember to perform manual ancestor invalidation.
     */
    if (parent_ != nullptr) {
        parent_->invalidateMeasure();
    }
}

void Widget::clearFocusIfIneligible() {
    /*
     * Property changes are allowed to synchronously trigger FocusEvent{false}. Consequently these
     * setters are deliberately not noexcept: an application-provided event handler may throw, and
     * terminating the process would be worse than propagating that exception. The focus state itself
     * is cleared before the notification is invoked by FocusManager, so invariants stay consistent.
     */
    if (focused_ && !canReceiveFocus() && focus_manager_ != nullptr) {
        (void)focus_manager_->clearFocus();
    }
}

} // namespace sasd::ui
