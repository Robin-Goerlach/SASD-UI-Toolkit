#include <sasd/ui/button.hpp>

#include "primary_pointer_gesture.hpp"
#include <sasd/ui/measurement_context.hpp>

#include <string>
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

void Button::bindCommand(Command& command) {
    /*
     * Rebinding the same live Command is treated as an explicit resynchronization request. This is
     * useful after deliberate local Button mutations and avoids replacing an otherwise healthy
     * subscription merely to copy the current semantic snapshot again.
     */
    if (command_binding_active_ && command_reference_.get() == &command) {
        setText(std::string{command.text()});
        Widget::setEnabled(command.isEnabled());
        return;
    }

    /*
     * Copy the initial values before changing this Button's current binding. std::string allocation is
     * the only meaningful throwing operation in the setup path; doing it first means an allocation
     * failure leaves the previous binding untouched rather than half-replaced.
     */
    std::string initial_text{command.text()};
    const bool initial_enabled = command.isEnabled();
    Command::Reference reference = command.reference();
    Command::StateSubscription subscription = command.observeState(
        [this](Command::StateChange change) {
            synchronizeBoundCommandState(change);
        });

    /*
     * From this point the new connection is complete. Disconnect the previous token before publishing
     * the new reference so no obsolete Command can continue mutating this Button after the rebind.
     */
    command_subscription_.reset();
    command_reference_ = std::move(reference);
    command_subscription_ = std::move(subscription);
    command_binding_active_ = true;

    setText(std::move(initial_text));
    Widget::setEnabled(initial_enabled);
}

void Button::unbindCommand() noexcept {
    command_subscription_.reset();
    command_reference_ = {};
    command_binding_active_ = false;
}

Command* Button::boundCommand() noexcept {
    return command_binding_active_ ? command_reference_.get() : nullptr;
}

const Command* Button::boundCommand() const noexcept {
    return command_binding_active_ ? command_reference_.get() : nullptr;
}

void Button::synchronizeBoundCommandState(Command::StateChange change) {
    if (!command_binding_active_) {
        return;
    }

    Command* command = command_reference_.get();
    if (command == nullptr) {
        /*
         * The Command may have been destroyed by an earlier observer in the same notification pass.
         * Command::Reference deliberately turns that case into nullptr while the notification snapshot
         * itself remains alive. Never dereference the original semantic object from this callback.
         */
        return;
    }

    switch (change) {
    case Command::StateChange::text:
        setText(std::string{command->text()});
        break;
    case Command::StateChange::enabled:
        Widget::setEnabled(command->isEnabled());
        break;
    }
}

bool Button::activate() {
    if (!isEnabled()) {
        return false;
    }

    if (command_binding_active_) {
        /*
         * Resolve the semantic target at the last responsible moment. The Reference does not extend
         * Command lifetime; an expired binding rejects activation instead of dereferencing a raw
         * pointer or silently falling back to unrelated local behavior.
         *
         * Return directly from execute(). Client code may synchronously destroy either the Command or
         * this Button, and no Button member should be touched after semantic execution begins.
         */
        Command* command = command_reference_.get();
        return command != nullptr ? command->execute() : false;
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
