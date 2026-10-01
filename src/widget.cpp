#include <sasd/ui/widget.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/pointer_router.hpp>

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

    /*
     * Pointer capture and hover are separate non-owning runtime observations. Either router must be
     * told before Widget storage disappears. widgetDestroyed() clears both roles owned by that router,
     * so re-reading the second reverse link avoids duplicate notification when both roles share it.
     */
    if (pointer_capture_router_ != nullptr) {
        pointer_capture_router_->widgetDestroyed(*this);
    }
    if (pointer_hover_router_ != nullptr) {
        pointer_hover_router_->widgetDestroyed(*this);
    }
}

void Widget::setVisible(bool visible) {
    if (visible_ == visible) {
        return;
    }

    visible_ = visible;

    /*
     * A hidden Widget immediately stops participating in HitTest. If it currently belongs to a
     * router's hover path, invalidate that complete path now rather than waiting for another native
     * motion event and exposing stale hover presentation in the meantime.
     */
    if (!visible && pointer_hover_router_ != nullptr) {
        pointer_hover_router_->widgetHoverInvalidated(*this);
    }

    /*
     * Visibility is stronger than an ordinary visual-state change. Hiding a widget can uncover a
     * previously occluded sibling (StackLayout is the obvious case, but arbitrary overlapping
     * geometry can do the same), while showing one can change which content should be considered
     * topmost. Repainting only this widget is therefore not sufficient in the general case.
     *
     * Request the same conservative subtree refresh used for geometry damage. The presentation root
     * can clear/rebuild from a known-clean surface and PresentationCoordinator will replay otherwise
     * clean descendants in visual order. This deliberately favors correctness over incremental
     * efficiency; dirty-region/occlusion-aware replay can optimize the policy later without changing
     * Widget visibility semantics.
     *
     * Visibility also affects layout participation, so measurement invalidation remains independent
     * from presentation invalidation and propagates through the visual-parent chain as before.
     */
    invalidateMeasure();
    invalidatePresentationSubtree();
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
        /*
         * The router stores geometry-derived hover, not the last pointer position. Once this Widget
         * moves or resizes that cached path is no longer provably correct. Drop it conservatively;
         * the next pointer event will rebuild the exact root-to-hit-target path.
         */
        if (pointer_hover_router_ != nullptr) {
            pointer_hover_router_->widgetHoverInvalidated(*this);
        }

        /*
         * Moving/resizing can leave old presentation content behind and can uncover siblings that
         * were previously occluded. A normal visual invalidation is therefore insufficient: request
         * a conservative subtree rebuild at the presentation root.
         */
        invalidatePresentationSubtree();
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

void Widget::invalidatePresentationSubtree() noexcept {
    subtree_refresh_pending_ = true;
    visual_update_pending_ = true;

    /*
     * Every ancestor must know that something below changed presentation geometry. The top-level
     * sink can then rebuild from a known-clean surface while PresentationCoordinator replays the
     * complete visual subtree, including otherwise-clean siblings.
     */
    if (parent_ != nullptr) {
        parent_->invalidatePresentationSubtree();
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

void Widget::setPointerOverState(bool pointer_over) noexcept {
    if (pointer_over_ == pointer_over) {
        return;
    }

    pointer_over_ = pointer_over;

    /*
     * Hover can alter backend presentation, but never intrinsic size. The state is router-owned and
     * carries no application callback, so a noexcept direct transition is sufficient and avoids
     * inventing enter/leave bubbling semantics before a concrete application-event use case exists.
     */
    invalidateVisual();
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
