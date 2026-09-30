#pragma once

#include <sasd/ui/style.hpp>
#include <sasd/ui/widget.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace sasd::ui {

class RadioGroup;

/**
 * Backend-neutral single-choice option control.
 *
 * A RadioButton may stand alone or belong to an explicit RadioGroup. Grouping is intentionally a
 * semantic relationship rather than an inference from sibling/visual-parent structure: layout and
 * ownership may change without silently changing which options are mutually exclusive.
 *
 * User activation selects the control but never toggles an already selected RadioButton off. This is
 * the defining difference from CheckBox. Application code may still call setSelected(false) to place
 * a group into its permitted "no selection" state.
 *
 * The initial M4 slice uses unmodified Space for keyboard selection. Arrow-key navigation inside a
 * group is deferred until the project has an explicit group-navigation/focus contract rather than
 * smuggling focus policy into this one control.
 */
class RadioButton final : public Widget {
public:
    using SelectedHandler = std::function<void()>;

    RadioButton();
    explicit RadioButton(std::string text);
    RadioButton(RadioGroup& group, std::string text);
    ~RadioButton() override;

    [[nodiscard]] std::string_view text() const noexcept { return text_; }
    [[nodiscard]] const TextStyle& textStyle() const noexcept { return text_style_; }
    [[nodiscard]] bool isSelected() const noexcept { return selected_; }

    [[nodiscard]] RadioGroup* group() noexcept { return group_; }
    [[nodiscard]] const RadioGroup* group() const noexcept { return group_; }

    /**
     * Returns transient primary-pointer pressed presentation state.
     *
     * Pressed state is independent from persistent selection, exactly as CheckBox keeps interaction
     * gesture state separate from its checked value.
     */
    [[nodiscard]] bool isPressed() const noexcept {
        return pointer_armed_ && pointer_inside_ && isVisible() && isEnabled();
    }

    void setTextStyle(TextStyle style);
    void setText(std::string text);

    /**
     * Changes semantic selected state.
     *
     * Selecting a grouped button first deselects the group's previous member, then marks this button
     * selected. Only a false->true transition invokes onSelected; deselection is intentionally silent
     * in the first contract so one group transaction has one application-level "new choice" signal.
     *
     * Programmatic false is permitted even for a grouped button, leaving the group with no selection.
     *
     * @returns true when this button's selected state changed.
     */
    bool setSelected(bool selected);

    /**
     * Installs a callback emitted after this button becomes the selected choice.
     *
     * The handler is copied before invocation and no member is touched afterwards, matching the
     * existing Button/CheckBox lifetime rule that allows a callback to release this Widget safely.
     */
    void setOnSelected(SelectedHandler handler) {
        on_selected_ = std::move(handler);
    }

protected:
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;
    [[nodiscard]] EventResult onEvent(const Event& event) override;
    void onPointerCaptureLost() noexcept override;

private:
    friend class RadioGroup;

    void applySelectedState(bool selected) noexcept;
    void setPointerGestureState(bool armed, bool inside) noexcept;

    std::string text_;
    TextStyle text_style_{};
    SelectedHandler on_selected_;
    RadioGroup* group_{nullptr};
    bool selected_{false};
    bool pointer_armed_{false};
    bool pointer_inside_{false};
};

} // namespace sasd::ui
