#include <sasd/ui/widget.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/measurement_context.hpp>

#include <stdexcept>
#include <typeinfo>

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
    invalidateVisual();
    clearFocusIfIneligible();
}

void Widget::setEnabled(bool enabled) {
    if (enabled_ == enabled) {
        return;
    }

    enabled_ = enabled;
    invalidateVisual();
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
     * Context-free and context-aware measurements intentionally occupy different cache domains. A
     * Label measured without presentation metrics must never be reused as if it had been measured by
     * a terminal/font context later.
     */
    if (measure_valid_ && !last_measure_used_context_ &&
        last_measure_constraints_ == constraints) {
        return desired_size_;
    }

    const Size intrinsic = onMeasure(constraints);
    const Size widget_constrained = size_constraints_.clamp(intrinsic);
    desired_size_ = constraints.clamp(widget_constrained);
    last_measure_constraints_ = constraints;
    last_measure_context_type_ = nullptr;
    last_measure_context_revision_ = 0;
    last_measure_used_context_ = false;
    measure_valid_ = true;

    return desired_size_;
}

Size Widget::measure(const MeasurementContext& context,
                     const MeasureConstraints& constraints) {
    if (!constraints.hasValidRange()) {
        throw std::invalid_argument(
            "Widget::measure requires non-negative, ordered MeasureConstraints");
    }

    const std::type_info* context_type = &typeid(context);
    const std::uint64_t context_revision = context.revision();

    /*
     * Do not retain &context. Measurement contexts can be short-lived stack objects. Dynamic
     * std::type_info has static lifetime, while revision identifies the concrete context's complete
     * metric policy. This makes the cache lifetime-safe without forcing ownership into Widget.
     */
    if (measure_valid_ && last_measure_used_context_ &&
        last_measure_context_type_ == context_type &&
        last_measure_context_revision_ == context_revision &&
        last_measure_constraints_ == constraints) {
        return desired_size_;
    }

    const Size intrinsic = onMeasure(context, constraints);
    const Size widget_constrained = size_constraints_.clamp(intrinsic);
    desired_size_ = constraints.clamp(widget_constrained);
    last_measure_constraints_ = constraints;
    last_measure_context_type_ = context_type;
    last_measure_context_revision_ = context_revision;
    last_measure_used_context_ = true;
    measure_valid_ = true;

    return desired_size_;
}

void Widget::arrange(Rect final_bounds) {
    if (final_bounds.width < 0 || final_bounds.height < 0) {
        throw std::invalid_argument("Widget::arrange requires non-negative extents");
    }

    /*
     * A geometry change affects presentation even when desired size remains valid. Remember the
     * comparison before storing the new rectangle, then mark the visual state stale after the new
     * geometry is visible through bounds().
     */
    const bool geometry_changed = bounds_ != final_bounds;
    bounds_ = final_bounds;

    if (geometry_changed) {
        invalidateVisual();
    }

    onArrange(final_bounds);
}

Size Widget::onMeasure(const MeasureConstraints&) {
    return size_constraints_.preferred;
}

Size Widget::onMeasure(const MeasurementContext&, const MeasureConstraints& constraints) {
    return onMeasure(constraints);
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

void Widget::invalidateVisual() noexcept {
    visual_update_pending_ = true;

    /*
     * A child visual change can require an ancestor surface/container to be redrawn as well. This is
     * deliberately the visual parent relation, not Component ownership: non-visual owned components
     * do not form a presentation path.
     *
     * Do not stop propagation merely because this widget was already dirty. A renderer may have
     * acknowledged the parent while leaving this child pending; a later child change must be able to
     * mark that parent dirty again.
     */
    if (parent_ != nullptr) {
        parent_->invalidateVisual();
    }
}

void Widget::setFocusState(bool focused, FocusManager* focus_manager) noexcept {
    const bool state_changed = focused_ != focused;

    focused_ = focused;
    focus_manager_ = focus_manager;

    // Focus commonly affects visual state (focus ring, inverse terminal style, caret, etc.) but never
    // requires re-measurement by itself.
    if (state_changed) {
        invalidateVisual();
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
