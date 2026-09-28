#include <sasd/ui/button.hpp>

#include <sasd/ui/hit_test.hpp>
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
    const bool was_pressed = isPressed();
    pointer_armed_ = armed;
    pointer_inside_ = inside;

    /*
     * Pressed state is visual state, not intrinsic geometry. Repeated motion within the same region
     * therefore does not invalidate presentation unless the externally observable state changed.
     */
    if (was_pressed != isPressed()) {
        invalidateVisual();
    }
}

void Button::onPointerCaptureLost() noexcept {
    // Capture can disappear without a release event; never leave the Button visually depressed.
    setPointerGestureState(false, false);
}

EventResult Button::onEvent(const Event& event) {
    if (const auto* pointer = std::get_if<PointerEvent>(&event)) {
        if (pointer->button != PointerButton::primary &&
            pointer->action != PointerAction::move) {
            return EventResult::ignored;
        }

        if (pointer->action == PointerAction::press) {
            if (!isEnabled() || !isVisible() ||
                !HitTest::contains(*this, pointer->position)) {
                setPointerGestureState(false, false);
                return EventResult::ignored;
            }

            setPointerGestureState(true, true);
            return EventResult::handled;
        }

        if (pointer->action == PointerAction::move) {
            if (!pointer_armed_) {
                return EventResult::ignored;
            }

            const bool inside =
                isEnabled() && isVisible() &&
                HitTest::contains(*this, pointer->position);
            setPointerGestureState(true, inside);
            return EventResult::handled;
        }

        if (pointer->action == PointerAction::release) {
            if (!pointer_armed_) {
                return EventResult::ignored;
            }

            const bool completes_inside =
                isEnabled() && isVisible() &&
                HitTest::contains(*this, pointer->position);

            /*
             * Clear state before application code. activate() may synchronously release/reparent the
             * Button, so no Button member may be required afterwards.
             */
            setPointerGestureState(false, false);

            if (completes_inside) {
                (void)activate();
            }
            return EventResult::handled;
        }

        return EventResult::ignored;
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
