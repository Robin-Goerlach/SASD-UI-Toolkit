#pragma once

#include <sasd/ui/style.hpp>
#include <sasd/ui/widget.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace sasd::ui {

/**
 * Backend-neutral two-state check control.
 *
 * CheckBox owns semantic state only: caption text, checked/unchecked value, focusability, optional
 * styling, and a synchronous checked-state callback. Terminal markers, rendered indicator geometry,
 * native peers, and theme-specific visuals remain presentation concerns.
 *
 * The first M4 contract is intentionally two-state. An indeterminate/tri-state model can be added
 * later when a concrete application requires the extra public-state and presentation complexity.
 *
 * Keyboard interaction uses unmodified Space while the CheckBox owns logical focus. Enter remains
 * unclaimed so forms can later use it for default actions. Pointer interaction mirrors Button's
 * armed press/release model: PointerRouter owns capture, visual pressed state follows pointer
 * position, and the value toggles only when the matching primary release completes inside.
 */
class CheckBox final : public Widget {
public:
    using CheckedChangedHandler = std::function<void(bool)>;

    CheckBox();
    explicit CheckBox(std::string text);
    CheckBox(std::string text, bool checked);

    [[nodiscard]] std::string_view text() const noexcept { return text_; }
    [[nodiscard]] const TextStyle& textStyle() const noexcept { return text_style_; }
    [[nodiscard]] bool isChecked() const noexcept { return checked_; }

    /**
     * Returns transient pointer-pressed presentation state, independent from checked state.
     */
    [[nodiscard]] bool isPressed() const noexcept {
        return pointer_armed_ && pointer_inside_ && isVisible() && isEnabled();
    }

    void setTextStyle(TextStyle style);

    /**
     * Replaces the UTF-8 caption. A real change invalidates both measurement and presentation.
     */
    void setText(std::string text);

    /**
     * Replaces semantic checked state.
     *
     * Programmatic assignment remains legal while disabled or hidden; enabled/visible gates apply to
     * user input, not application model updates. Checked state changes presentation but not intrinsic
     * size. The callback is copied before invocation so application code may replace it or release
     * the control without invalidating the std::function currently executing.
     *
     * @returns true when the value changed.
     */
    bool setChecked(bool checked);

    /** Toggles the current semantic value. */
    bool toggle() { return setChecked(!checked_); }

    void setOnCheckedChanged(CheckedChangedHandler handler) {
        on_checked_changed_ = std::move(handler);
    }

protected:
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;
    [[nodiscard]] EventResult onEvent(const Event& event) override;
    void onPointerCaptureLost() noexcept override;

private:
    void setPointerGestureState(bool armed, bool inside) noexcept;

    std::string text_;
    TextStyle text_style_{};
    CheckedChangedHandler on_checked_changed_;
    bool checked_{false};

    /*
     * Gesture ownership and geometric inside-state are separate from persistent checked state. This
     * prevents routing/presentation code from conflating "currently pressed" with "currently checked".
     */
    bool pointer_armed_{false};
    bool pointer_inside_{false};
};

} // namespace sasd::ui
