#pragma once

#include <sasd/ui/events/event.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/widget.hpp>

namespace sasd::ui::detail {

/**
 * Result of applying one PointerEvent to a control's primary press/release gesture.
 *
 * This type deliberately carries only interaction mechanics. It does not know whether the owning
 * control is a Button, CheckBox, RadioButton, or a future control, and it never invokes application
 * callbacks. The concrete control remains responsible for deciding what a completed gesture means.
 *
 * The helper lives under src/ and is therefore not part of the installed/public API. Three controls
 * now provide enough evidence for shared mechanics, but not enough evidence to justify freezing a
 * new public/protected control base class before 1.0.
 */
struct PrimaryPointerGestureUpdate {
    EventResult result{EventResult::ignored};

    /**
     * True when externally observable isPressed() state changed.
     *
     * The concrete Widget owns invalidation because invalidateVisual() is protected semantic state,
     * not a responsibility that should be exposed to a private helper merely for convenience.
     */
    bool pressed_state_changed{false};

    /**
     * True only for a matching primary release that completed inside the visible/enabled control.
     *
     * Gesture state has already been retired before this flag is returned. A concrete control may
     * therefore enter application callbacks immediately without leaving stale pressed state behind.
     */
    bool completed_inside{false};
};

/**
 * Computes the presentation-visible pressed state from raw gesture mechanics.
 *
 * Keeping this tiny rule here ensures the transition comparison used by the helper is identical to
 * the public isPressed() contracts of Button/CheckBox/RadioButton.
 */
[[nodiscard]] inline bool primaryPointerPressed(const Widget& widget,
                                                bool armed,
                                                bool inside) noexcept {
    return armed && inside && widget.isVisible() && widget.isEnabled();
}

/**
 * Applies the common primary-pointer armed gesture used by simple clickable controls.
 *
 * Contract:
 * - only primary press/release transitions participate;
 * - captured movement is accepted with PointerButton::none;
 * - press arms only inside visible/enabled clipped geometry;
 * - captured movement updates only the geometric-inside part of pressed state;
 * - release always retires an existing gesture and reports whether it completed inside;
 * - ignored input never creates a new gesture;
 * - all state retirement occurs before the caller is told to perform semantic activation.
 *
 * armed and inside are intentionally supplied by reference instead of hidden in a public base class.
 * They remain private state of each concrete control, preserving today's class layout/API boundary
 * while centralizing the behavior that has now proven identical across three controls.
 */
[[nodiscard]] inline PrimaryPointerGestureUpdate updatePrimaryPointerGesture(
    const Widget& widget,
    const PointerEvent& event,
    bool& armed,
    bool& inside) noexcept {
    const bool was_pressed =
        primaryPointerPressed(widget, armed, inside);

    auto finish = [&](EventResult result,
                      bool completed_inside = false) noexcept {
        return PrimaryPointerGestureUpdate{
            result,
            was_pressed != primaryPointerPressed(widget, armed, inside),
            completed_inside};
    };

    /*
     * PointerRouter delivers captured movement with PointerButton::none. Other non-primary
     * transitions must remain available for parent/application handling and must not disturb a
     * primary gesture that may already be active.
     */
    if (event.button != PointerButton::primary &&
        event.action != PointerAction::move) {
        return finish(EventResult::ignored);
    }

    if (event.action == PointerAction::press) {
        if (!widget.isEnabled() ||
            !widget.isVisible() ||
            !HitTest::contains(widget, event.position)) {
            /*
             * Defensive cleanup mirrors the original per-control behavior. In ordinary routing a new
             * press should not arrive while this control still owns capture, but stale local gesture
             * state must never survive an invalid press if a custom dispatcher violates that norm.
             */
            armed = false;
            inside = false;
            return finish(EventResult::ignored);
        }

        armed = true;
        inside = true;
        return finish(EventResult::handled);
    }

    if (event.action == PointerAction::move) {
        if (!armed) {
            return finish(EventResult::ignored);
        }

        inside =
            widget.isEnabled() &&
            widget.isVisible() &&
            HitTest::contains(widget, event.position);
        return finish(EventResult::handled);
    }

    if (event.action == PointerAction::release) {
        if (!armed) {
            return finish(EventResult::ignored);
        }

        const bool completes_inside =
            widget.isEnabled() &&
            widget.isVisible() &&
            HitTest::contains(widget, event.position);

        /*
         * Retire the gesture before the concrete control observes completed_inside. Its semantic
         * action may synchronously invoke application code that releases or destroys that control.
         */
        armed = false;
        inside = false;

        return finish(EventResult::handled, completes_inside);
    }

    return finish(EventResult::ignored);
}

/**
 * Cancels local gesture state after PointerRouter reports capture loss.
 *
 * @returns true when presentation-visible pressed state changed.
 */
[[nodiscard]] inline bool cancelPrimaryPointerGesture(
    const Widget& widget,
    bool& armed,
    bool& inside) noexcept {
    const bool was_pressed =
        primaryPointerPressed(widget, armed, inside);

    armed = false;
    inside = false;

    return was_pressed != primaryPointerPressed(widget, armed, inside);
}

} // namespace sasd::ui::detail
