#include <sasd/ui/radio_group.hpp>

#include <sasd/ui/radio_button.hpp>

#include <algorithm>
#include <stdexcept>

namespace sasd::ui {

RadioGroup::~RadioGroup() {
    /*
     * Group membership is non-owning. If the semantic group dies first, leave each RadioButton alive
     * and selected state untouched, but sever its back-pointer before this object ceases to exist.
     *
     * No invalidation is required: losing grouping does not change how any individual RadioButton is
     * drawn; it changes only the exclusivity rule applied to future state transitions.
     */
    for (RadioButton* button : members_) {
        if (button != nullptr && button->group_ == this) {
            button->group_ = nullptr;
        }
    }

    members_.clear();
    selected_ = nullptr;
}

void RadioGroup::attach(RadioButton& button) {
    if (button.group_ != nullptr) {
        throw std::invalid_argument{
            "RadioGroup::attach requires a RadioButton that is not already grouped"};
    }

    /*
     * vector growth may throw. Commit the reverse pointer only after storage succeeds, giving the
     * constructor a strong relationship guarantee: failure leaves both objects completely detached.
     */
    members_.push_back(&button);
    button.group_ = this;

    /*
     * Grouped construction starts unselected in the public API, so this branch is primarily
     * defensive for future internal evolution. If a preselected button is ever attached, it becomes
     * the current choice and the old member is deselected atomically.
     */
    if (button.selected_) {
        if (selected_ != nullptr && selected_ != &button) {
            selected_->applySelectedState(false);
        }
        selected_ = &button;
    }
}

void RadioGroup::detach(RadioButton& button) noexcept {
    if (button.group_ != this) {
        return;
    }

    const auto it = std::find(members_.begin(), members_.end(), &button);
    if (it != members_.end()) {
        members_.erase(it);
    }

    if (selected_ == &button) {
        selected_ = nullptr;
    }

    button.group_ = nullptr;
}

bool RadioGroup::select(RadioButton& button) noexcept {
    if (button.group_ != this || selected_ == &button) {
        return false;
    }

    /*
     * Establish the complete group invariant before RadioButton emits any callback. Application code
     * observing selectedButton() from onSelected therefore never sees two selected members or the old
     * choice still active.
     */
    RadioButton* previous = selected_;
    selected_ = &button;

    if (previous != nullptr) {
        previous->applySelectedState(false);
    }
    button.applySelectedState(true);
    return true;
}

bool RadioGroup::clear(RadioButton& button) noexcept {
    if (button.group_ != this || selected_ != &button) {
        return false;
    }

    selected_ = nullptr;
    button.applySelectedState(false);
    return true;
}

} // namespace sasd::ui
