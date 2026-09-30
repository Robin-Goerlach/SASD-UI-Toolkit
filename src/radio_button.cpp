#include <sasd/ui/radio_button.hpp>

#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/radio_group.hpp>

#include <variant>

namespace sasd::ui {

RadioButton::RadioButton() {
    setFocusable(true);
}

RadioButton::RadioButton(std::string text)
    : RadioButton() {
    text_ = std::move(text);
}

RadioButton::RadioButton(RadioGroup& group, std::string text)
    : RadioButton(std::move(text)) {
    group.attach(*this);
}

RadioButton::~RadioButton() {
    if (group_ != nullptr) {
        group_->detach(*this);
    }
}

void RadioButton::setTextStyle(TextStyle style) {
    if (text_style_ == style) {
        return;
    }

    text_style_ = style;
    invalidateVisual();
}

void RadioButton::setText(std::string text) {
    if (text_ == text) {
        return;
    }

    text_ = std::move(text);
    invalidateMeasure();
    invalidateVisual();
}

void RadioButton::applySelectedState(bool selected) noexcept {
    if (selected_ == selected) {
        return;
    }

    selected_ = selected;

    /*
     * Selection changes only the indicator's visual state. Its footprint remains fixed across
     * selected/unselected presentation, so invalidating measurement would create unnecessary layout
     * work and would contradict the stable-chrome contract planned for both Terminal and Rendered.
     */
    invalidateVisual();
}

bool RadioButton::setSelected(bool selected) {
    if (selected_ == selected) {
        return false;
    }

    bool changed = false;

    if (group_ != nullptr) {
        changed = selected
                      ? group_->select(*this)
                      : group_->clear(*this);
    } else {
        applySelectedState(selected);
        changed = true;
    }

    if (!changed) {
        return false;
    }

    if (selected) {
        /*
         * Copy before entering application code. All group/member state is already coherent, and this
         * method intentionally performs no member access after invocation, so a handler may release
         * or destroy this RadioButton using the same lifetime pattern supported by Button/CheckBox.
         */
        SelectedHandler handler = on_selected_;
        if (handler) {
            handler();
        }
    }

    return true;
}

Size RadioButton::onMeasure(const MeasurementContext& context,
                            const MeasureConstraints&) {
    return context.measureRadioButton(text_);
}

void RadioButton::setPointerGestureState(bool armed, bool inside) noexcept {
    const bool was_pressed = isPressed();

    pointer_armed_ = armed;
    pointer_inside_ = inside;

    if (was_pressed != isPressed()) {
        invalidateVisual();
    }
}

void RadioButton::onPointerCaptureLost() noexcept {
    setPointerGestureState(false, false);
}

EventResult RadioButton::onEvent(const Event& event) {
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
             * Retire transient gesture state before selection can enter application callbacks. A
             * selected handler may release this object, so nothing below may depend on gesture fields.
             */
            setPointerGestureState(false, false);

            if (completes_inside && !selected_) {
                (void)setSelected(true);
            }

            return EventResult::handled;
        }

        return EventResult::ignored;
    }

    const auto* key = std::get_if<KeyEvent>(&event);
    if (key == nullptr ||
        key->key != Key::space ||
        key->modifiers != KeyModifier::none) {
        return EventResult::ignored;
    }

    if (!hasFocus() || !isEnabled() || !isVisible()) {
        return EventResult::ignored;
    }

    if (key->pressed && !selected_) {
        /*
         * Select on key-down because terminal input does not reliably provide key-up. Repeated Space
         * on an already selected radio is consumed but deliberately does not deselect it.
         */
        (void)setSelected(true);
    }

    return EventResult::handled;
}

} // namespace sasd::ui
