#include <sasd/ui/button.hpp>

#include "primary_pointer_gesture.hpp"
#include <sasd/ui/measurement_context.hpp>

#include <variant>

namespace sasd::ui {

Button::Button() {
    // Unlike structural widgets and Label, a Button is an intentional keyboard-focus target.
    setFocusable(true);
}

Button::Button(std::string text)
    : Button() {
    text_ = std::move(text);
}

void Button::setTextStyle(TextStyle style) {
    if (text_style_ == style) {
        return;
    }

    text_style_ = style;
    invalidateVisual();
}

void Button::setText(std::string text) {
    if (text_ == text) {
        return;
    }

    text_ = std::move(text);
    invalidateMeasure();
    invalidateVisual();
}

bool Button::activate() {
    if (!isEnabled()) {
        return false;
    }

    /*
     * Copy before calling application code. The callback may synchronously remove/destroy this
     * control; invoking through a local copy avoids executing through a std::function member that was
     * replaced or detached as part of that mutation.
     */
    ActivationHandler handler = on_activated_;
    if (handler) {
        handler();
    }

    // Keep post-callback work independent of Button state; the callback may have reparented/released it.
    return true;
}

Size Button::onMeasure(const MeasurementContext& context, const MeasureConstraints&) {
    return context.measureButton(text_);
}

void Button::setPointerGestureState(bool armed, bool inside) noexcept {
    const bool was_pressed =
        detail::primaryPointerPressed(*this, pointer_armed_, pointer_inside_);

    pointer_armed_ = armed;
    pointer_inside_ = inside;

    /*
     * This setter remains for concrete-control code paths that need direct cancellation. The common
     * event transition rules live in primary_pointer_gesture.hpp; visual invalidation stays here
     * because it is protected Widget behavior and part of the concrete control's responsibility.
     */
    if (was_pressed !=
        detail::primaryPointerPressed(*this, pointer_armed_, pointer_inside_)) {
        invalidateVisual();
    }
}

void Button::onPointerCaptureLost() noexcept {
    if (detail::cancelPrimaryPointerGesture(
            *this,
            pointer_armed_,
            pointer_inside_)) {
        invalidateVisual();
    }
}

EventResult Button::onEvent(const Event& event) {
    if (const auto* pointer = std::get_if<PointerEvent>(&event)) {
        const detail::PrimaryPointerGestureUpdate update =
            detail::updatePrimaryPointerGesture(
                *this,
                *pointer,
                pointer_armed_,
                pointer_inside_);

        if (update.pressed_state_changed) {
            invalidateVisual();
        }

        /*
         * updatePrimaryPointerGesture() has already retired pressed state before reporting completion.
         * activate() may therefore invoke application code that releases this Button without leaving
         * gesture cleanup for code that runs after the callback.
         */
        if (update.completed_inside) {
            (void)activate();
        }

        return update.result;
    }

    const auto* key = std::get_if<KeyEvent>(&event);
    if (key == nullptr) {
        return EventResult::ignored;
    }

    const bool activation_key = key->key == Key::enter || key->key == Key::space;

    /*
     * Modified Enter/Space remains available for application shortcuts and parent handlers. The
     * initial Button contract claims only the ordinary unmodified activation gesture.
     */
    if (!activation_key || key->modifiers != KeyModifier::none) {
        return EventResult::ignored;
    }

    /*
     * EventDispatcher itself does not select/filter focus targets. Requiring logical focus here
     * prevents accidental activation if application code dispatches a keyboard event to the wrong
     * Widget. hasFocus() also implies that current FocusManager eligibility was established.
     */
    if (!hasFocus() || !isEnabled() || !isVisible()) {
        return EventResult::ignored;
    }

    if (key->pressed) {
        /*
         * Activate on key-down because ANSI/VT input generally reports key presses, not paired
         * press/release transitions. This also avoids inventing a transient "pressed" state that the
         * terminal backend could never reliably clear from input alone.
         */
        (void)activate();
    }

    // Consume both press and release for an activation key while focused.
    return EventResult::handled;
}

} // namespace sasd::ui
