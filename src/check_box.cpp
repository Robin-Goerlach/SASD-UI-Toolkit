#include <sasd/ui/check_box.hpp>

#include <sasd/ui/hit_test.hpp>
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
    const bool was_pressed = isPressed();
    pointer_armed_ = armed;
    pointer_inside_ = inside;

    // Routing-detail changes repaint only when presentation-visible pressed state actually changes.
    if (was_pressed != isPressed()) {
        invalidateVisual();
    }
}

void CheckBox::onPointerCaptureLost() noexcept {
    // Surface leave or external capture release must never leave the control visually depressed.
    setPointerGestureState(false, false);
}

EventResult CheckBox::onEvent(const Event& event) {
    if (const auto* pointer = std::get_if<PointerEvent>(&event)) {
        /*
         * Captured movement carries PointerButton::none. Non-primary press/release transitions remain
         * available to application/parent handlers.
         */
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
             * Retire gesture state before setChecked() can enter application code. Its callback may
             * release this object, so no gesture member may be needed afterwards.
             */
            setPointerGestureState(false, false);

            if (completes_inside) {
                const bool next = !checked_;
                (void)setChecked(next);
            }
            return EventResult::handled;
        }

        return EventResult::ignored;
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
