#include <sasd/ui/radio_button.hpp>

#include "primary_pointer_gesture.hpp"
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

void RadioButton::onPointerCaptureLost() noexcept {
    if (detail::cancelPrimaryPointerGesture(
            *this,
            pointer_armed_,
            pointer_inside_)) {
        invalidateVisual();
    }
}

EventResult RadioButton::onEvent(const Event& event) {
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

        if (update.completed_inside && !selected_) {
            /*
             * User activation selects but never toggles off an already selected radio. The shared
             * helper owns only gesture mechanics; this selection rule remains RadioButton semantics.
             */
            (void)setSelected(true);
        }

        return update.result;
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
