#pragma once

#include <sasd/ui/component.hpp>
#include <sasd/ui/events/event.hpp>
#include <sasd/ui/geometry.hpp>

namespace sasd::ui {

class Container;
class FocusManager;

/**
 * Base class for components that have a visual or interactively presentable representation.
 *
 * Widget stores backend-neutral state only. Native handles, terminal cells, renderer objects and
 * other backend details belong to peers/backends rather than this public class.
 */
class Widget : public Component {
public:
    Widget() = default;
    ~Widget() override;

    [[nodiscard]] Container* parent() noexcept { return parent_; }
    [[nodiscard]] const Container* parent() const noexcept { return parent_; }

    [[nodiscard]] bool isVisible() const noexcept { return visible_; }

    /**
     * Changes this widget's local visibility state.
     *
     * Hiding a currently focused widget makes it locally ineligible for focus and therefore clears
     * that focus synchronously. Visibility can also affect a parent's future layout policy, so the
     * measure cache is invalidated whenever visibility changes.
     */
    void setVisible(bool visible);

    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }

    /**
     * Changes this widget's local enabled state.
     *
     * Disabling a currently focused widget clears focus synchronously because disabled controls must
     * not remain keyboard-input targets.
     */
    void setEnabled(bool enabled);

    /**
     * Returns whether this widget type/instance is allowed to receive logical keyboard focus.
     *
     * Widgets are deliberately non-focusable by default. Concrete interactive controls such as a
     * future Button or TextField can opt in explicitly instead of making containers and labels
     * accidental keyboard targets.
     */
    [[nodiscard]] bool isFocusable() const noexcept { return focusable_; }

    /**
     * Enables or disables this widget's ability to receive focus.
     *
     * Making the currently focused widget non-focusable clears focus synchronously.
     */
    void setFocusable(bool focusable);

    /** Returns whether a FocusManager currently designates this widget as its focused widget. */
    [[nodiscard]] bool hasFocus() const noexcept { return focused_; }

    /**
     * Returns whether this widget is locally eligible to become the focus target.
     *
     * M1 intentionally evaluates the widget's own focusable/visible/enabled state only. Effective
     * inherited state from hidden/disabled ancestors belongs to later focus-scope/navigation rules,
     * once Window and real container behavior exist.
     */
    [[nodiscard]] bool canReceiveFocus() const noexcept {
        return focusable_ && visible_ && enabled_;
    }

    /** Returns this widget's minimum/preferred/maximum intrinsic size hints. */
    [[nodiscard]] const SizeConstraints& sizeConstraints() const noexcept {
        return size_constraints_;
    }

    /**
     * Replaces this widget's intrinsic size hints and invalidates cached measurement.
     *
     * minimum > maximum is rejected because a widget should never retain contradictory layout state.
     * preferred may lie outside the interval; measure() clamps it just like any custom intrinsic
     * measurement returned by a concrete widget.
     */
    void setSizeConstraints(SizeConstraints constraints);

    /** Returns the most recently measured desired size. */
    [[nodiscard]] Size desiredSize() const noexcept { return desired_size_; }

    /** Returns whether desiredSize() belongs to the current widget/layout state. */
    [[nodiscard]] bool isMeasureValid() const noexcept { return measure_valid_; }

    /**
     * Measures this widget inside parent-provided constraints.
     *
     * Measurement is backend-neutral and expressed only in logical Size values. The concrete widget's
     * onMeasure() result is first constrained by the widget's own SizeConstraints and then by the
     * parent's MeasureConstraints. The parent therefore has final authority over available space.
     *
     * Repeating measure() with identical constraints reuses the cached desired size until
     * invalidateMeasure() is called.
     *
     * @throws std::invalid_argument when the supplied MeasureConstraints are malformed.
     */
    [[nodiscard]] Size measure(const MeasureConstraints& constraints = {});

    /**
     * Assigns the final rectangle produced by layout and invokes the arrangement hook.
     *
     * The bounds are stored before onArrange() runs so container implementations can query their own
     * final rectangle while arranging children. Zero extents are valid; negative extents are rejected.
     *
     * @throws std::invalid_argument when width or height is negative.
     */
    void arrange(Rect final_bounds);

    /**
     * Convenience alias for direct/manual positioning.
     *
     * All geometry assignment intentionally goes through arrange() so manual code and future layout
     * managers cannot bypass onArrange() invariants.
     */
    void setBounds(Rect bounds) { arrange(bounds); }

    [[nodiscard]] Rect bounds() const noexcept { return bounds_; }

    /**
     * Delivers one already-normalized semantic event to this widget.
     *
     * This method performs single-widget delivery only; parent traversal belongs to EventDispatcher.
     * Derived widgets override onEvent() rather than this public entry point so routing, focus and
     * future dispatch instrumentation all share one stable delivery boundary.
     */
    [[nodiscard]] EventResult handleEvent(const Event& event) {
        return onEvent(event);
    }

protected:
    /**
     * Computes this widget's intrinsic desired size before framework clamping.
     *
     * The base implementation returns sizeConstraints().preferred. Concrete widgets override this to
     * measure content such as text while remaining independent of pixels or a particular backend.
     */
    [[nodiscard]] virtual Size onMeasure(const MeasureConstraints& constraints);

    /**
     * Called after final bounds have been stored.
     *
     * Leaf widgets normally need no implementation. Containers/layout-aware widgets can override this
     * hook to arrange visual children inside the assigned rectangle.
     */
    virtual void onArrange(Rect final_bounds);

    /**
     * Marks cached measurement stale and propagates that invalidation to the visual parent.
     *
     * A future Label can call this after text/font-metric changes; a parent VBox/Window then becomes
     * stale as well without any backend-specific update mechanism leaking into the component model.
     */
    void invalidateMeasure() noexcept;

    /**
     * Event hook for concrete widgets.
     *
     * Returning EventResult::handled consumes an ordinarily routed event. FocusManager-generated
     * FocusEvent notifications are direct state notifications; their return value is intentionally
     * ignored because focus changes are not vetoable through event propagation.
     */
    [[nodiscard]] virtual EventResult onEvent(const Event&) {
        return EventResult::ignored;
    }

private:
    friend class Container;
    friend class FocusManager;

    void setParent(Container* parent) noexcept { parent_ = parent; }

    /**
     * Updates the two sides of the Widget/FocusManager invariant together.
     *
     * focus_manager_ is non-owning and is non-null only while focused_ is true. FocusManager and the
     * Widget destructor cooperate so neither side retains a pointer to an already-destroyed object.
     */
    void setFocusState(bool focused, FocusManager* focus_manager) noexcept {
        focused_ = focused;
        focus_manager_ = focus_manager;
    }

    /** Clears current focus after a local property change made this widget ineligible. */
    void clearFocusIfIneligible();

    Container* parent_{nullptr};
    FocusManager* focus_manager_{nullptr};
    Rect bounds_{};
    SizeConstraints size_constraints_{};
    Size desired_size_{};
    MeasureConstraints last_measure_constraints_{};
    bool visible_{true};
    bool enabled_{true};
    bool focusable_{false};
    bool focused_{false};
    bool measure_valid_{false};
};

} // namespace sasd::ui
