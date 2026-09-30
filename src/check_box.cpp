#include <sasd/ui/check_box.hpp>

#include "primary_pointer_gesture.hpp"
#include <sasd/ui/measurement_context.hpp>

#include <variant>

namespace sasd::ui {

CheckBox::CheckBox() {
    // CheckBox is an intentional keyboard target, like Button and TextField.
    setFocusable(true);
}

CheckBox::CheckBox(std::string text)
    : CheckBox() {
    text_ = std::move(text);
}

CheckBox::CheckBox(std::string text, bool checked)
    : CheckBox(std::move(text)) {
    // Construction establishes initial model state without manufacturing a change notification.
    checked_ = checked;
}

void CheckBox::setTextStyle(TextStyle style) {
    if (text_style_ == style) {
        return;
    }

    text_style_ = style;
    invalidateVisual();
}

void CheckBox::setText(std::string text) {
    if (text_ == text) {
        return;
    }

    text_ = std::move(text);
    invalidateMeasure();
    invalidateVisual();
}

bool CheckBox::setChecked(bool checked) {
    if (checked_ == checked) {
        return false;
    }

    checked_ = checked;

    /*
     * Indicator state changes appearance, not intrinsic geometry. Invalidate before application code
     * runs so callbacks observe a coherent dirty state. There is deliberately no measure invalidation.
     */
    invalidateVisual();

    /*
     * Copy before invoking application code. The callback may replace itself or release/reparent this
     * CheckBox. Do not access members after invocation; this mirrors Button::activate()'s lifetime rule.
     */
    CheckedChangedHandler handler = on_checked_changed_;
    if (handler) {
        handler(checked);
    }

    return true;
}

Size CheckBox::onMeasure(const MeasurementContext& context,
                         const MeasureConstraints&) {
    return context.measureCheckBox(text_);
}

void CheckBox::setPointerGestureState(bool armed, bool inside) noexcept {
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

void CheckBox::onPointerCaptureLost() noexcept {
    if (detail::cancelPrimaryPointerGesture(
            *this,
            pointer_armed_,
            pointer_inside_)) {
        invalidateVisual();
    }
}

EventResult CheckBox::onEvent(const Event& event) {
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

        if (update.completed_inside) {
            /*
             * Read the next semantic value before setChecked() enters application callbacks. Gesture
             * state is already retired by the helper, and no member is required after setChecked().
             */
            const bool next = !checked_;
            (void)setChecked(next);
        }

        return update.result;
    }

    const auto* key = std::get_if<KeyEvent>(&event);
    if (key == nullptr) {
        return EventResult::ignored;
    }

    /*
     * Space is the portable checkbox gesture. Enter is reserved for future/default form actions;
     * modified Space remains available for application shortcuts.
     */
    if (key->key != Key::space || key->modifiers != KeyModifier::none) {
        return EventResult::ignored;
    }

    if (!hasFocus() || !isEnabled() || !isVisible()) {
        return EventResult::ignored;
    }

    if (key->pressed) {
        /*
         * Toggle on key-down because terminal input cannot reliably promise key-up. Desktop key-up is
         * consumed too, preventing one logical gesture from bubbling into an unrelated parent action.
         */
        const bool next = !checked_;
        (void)setChecked(next);
    }

    return EventResult::handled;
}

} // namespace sasd::ui
