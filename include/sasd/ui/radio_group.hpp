#pragma once

#include <sasd/ui/component.hpp>

#include <cstddef>
#include <vector>

namespace sasd::ui {

class RadioButton;
class RadioGroupNavigation;

/**
 * Non-visual semantic exclusivity group for RadioButton controls.
 *
 * RadioGroup deliberately does not depend on visual parenting or Container ownership. Radio buttons
 * may therefore remain grouped while being laid out in different nested containers, and moving a
 * Widget between visual owners does not silently change selection semantics.
 *
 * The initial contract guarantees "at most one selected member". It does not require that one member
 * is always selected: application code may explicitly clear the current selection.
 *
 * Membership is established by RadioButton construction and retired automatically from either side.
 * The group may die before its buttons or a button may die before the group; both paths detach the
 * non-owning relationship so neither object keeps a dangling pointer.
 */
class RadioGroup final : public Component {
public:
    RadioGroup() = default;
    ~RadioGroup() override;

    RadioGroup(const RadioGroup&) = delete;
    RadioGroup& operator=(const RadioGroup&) = delete;
    RadioGroup(RadioGroup&&) = delete;
    RadioGroup& operator=(RadioGroup&&) = delete;

    [[nodiscard]] std::size_t memberCount() const noexcept {
        return members_.size();
    }

    [[nodiscard]] RadioButton* selectedButton() noexcept {
        return selected_;
    }

    [[nodiscard]] const RadioButton* selectedButton() const noexcept {
        return selected_;
    }

private:
    friend class RadioButton;
    friend class RadioGroupNavigation;

    void attach(RadioButton& button);
    void detach(RadioButton& button) noexcept;

    /**
     * Selects one member and atomically retires the previous member before returning.
     *
     * User callbacks are intentionally not invoked here. RadioButton fires its selection callback
     * only after group state is internally coherent, so application code can safely inspect the group
     * from that callback.
     */
    [[nodiscard]] bool select(RadioButton& button) noexcept;

    /** Clears selection only when button is the currently selected member. */
    [[nodiscard]] bool clear(RadioButton& button) noexcept;

    std::vector<RadioButton*> members_;
    RadioButton* selected_{nullptr};
};

} // namespace sasd::ui
