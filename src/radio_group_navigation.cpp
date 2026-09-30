#include <sasd/ui/radio_group_navigation.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/widget.hpp>

#include <algorithm>
#include <cstddef>
#include <variant>

namespace sasd::ui {
namespace {

/**
 * Returns the top-most visual ancestor for one Widget.
 *
 * RadioGroup membership is intentionally independent from visual parenting. Keyboard focus,
 * however, belongs to one visible surface/scope at a time, so group navigation must not cross from a
 * RadioButton in one top-level tree into a grouped button in another.
 */
[[nodiscard]] const Widget* visualRoot(const Widget& widget) noexcept {
    const Widget* current = &widget;
    while (current->parent() != nullptr) {
        current = current->parent();
    }
    return current;
}

/**
 * Applies effective keyboard reachability rather than Widget::canReceiveFocus()'s local-only check.
 *
 * A locally enabled RadioButton under a disabled/hidden panel must not become reachable through arrow
 * navigation merely because RadioGroup is semantic and independent from the visual tree.
 */
[[nodiscard]] bool effectivelyReachable(const RadioButton& button) noexcept {
    if (!button.canReceiveFocus()) {
        return false;
    }

    const Widget* current = button.parent();
    while (current != nullptr) {
        if (!current->isVisible() || !current->isEnabled()) {
            return false;
        }
        current = current->parent();
    }

    return true;
}

[[nodiscard]] bool navigationKey(const KeyEvent& key,
                                 RadioGroupNavigationDirection& direction) noexcept {
    if (key.modifiers != KeyModifier::none) {
        return false;
    }

    if (key.key == Key::left || key.key == Key::up) {
        direction = RadioGroupNavigationDirection::previous;
        return true;
    }

    if (key.key == Key::right || key.key == Key::down) {
        direction = RadioGroupNavigationDirection::next;
        return true;
    }

    return false;
}

} // namespace

RadioButton* RadioGroupNavigation::targetFor(
    RadioButton& source,
    RadioGroupNavigationDirection direction) noexcept {
    RadioGroup* const group = source.group();
    if (group == nullptr || group->members_.size() < 2U) {
        return nullptr;
    }

    const auto source_it =
        std::find(group->members_.begin(), group->members_.end(), &source);
    if (source_it == group->members_.end()) {
        return nullptr;
    }

    const std::size_t count = group->members_.size();
    const std::size_t source_index =
        static_cast<std::size_t>(std::distance(group->members_.begin(), source_it));
    const Widget* const source_root = visualRoot(source);

    /*
     * Examine at most count-1 peers. Skipping source itself matters when every other member is
     * disabled/hidden: Arrow should be a no-op rather than pretending navigation succeeded by
     * wrapping back onto the current control.
     */
    for (std::size_t step = 1U; step < count; ++step) {
        const std::size_t index =
            direction == RadioGroupNavigationDirection::next
                ? (source_index + step) % count
                : (source_index + count - (step % count)) % count;

        RadioButton* const candidate = group->members_[index];
        if (candidate == nullptr ||
            visualRoot(*candidate) != source_root ||
            !effectivelyReachable(*candidate)) {
            continue;
        }

        return candidate;
    }

    return nullptr;
}

bool RadioGroupNavigation::move(FocusManager& focus,
                                RadioButton& source,
                                RadioGroupNavigationDirection direction) {
    /*
     * Only the currently focused group member may initiate keyboard navigation. Requiring this here,
     * instead of trusting the caller, prevents a stale application target from moving unrelated focus.
     */
    if (!source.hasFocus() || focus.focusedWidget() != &source) {
        return false;
    }

    RadioButton* const target = targetFor(source, direction);
    if (target == nullptr) {
        return false;
    }

    /*
     * Focus first. FocusManager owns re-entrant focus semantics: a focus-lost/gained callback may
     * redirect the transition. In that case requestFocus() returns false and selection is untouched.
     *
     * When requestFocus() returns true, synchronous callbacks have completed and target is still the
     * manager's current focus. Commit group selection next. setSelected() establishes exclusivity
     * before invoking onSelected and this function intentionally does not dereference target again.
     */
    if (!focus.requestFocus(*target)) {
        return false;
    }

    (void)target->setSelected(true);
    return true;
}

EventResult RadioGroupNavigation::handleEvent(FocusManager& focus,
                                              RadioButton& source,
                                              const Event& event) {
    const auto* key = std::get_if<KeyEvent>(&event);
    if (key == nullptr) {
        return EventResult::ignored;
    }

    RadioGroupNavigationDirection direction =
        RadioGroupNavigationDirection::next;
    if (!navigationKey(*key, direction)) {
        return EventResult::ignored;
    }

    /*
     * Compute applicability on release as well. This means a single-member/effectively isolated radio
     * leaves Arrow available to its parent/application, while a real navigable group consumes the
     * desktop key-up corresponding to a handled key-down without repeating the move.
     */
    if (!key->pressed) {
        return source.hasFocus() &&
                       focus.focusedWidget() == &source &&
                       targetFor(source, direction) != nullptr
                   ? EventResult::handled
                   : EventResult::ignored;
    }

    return move(focus, source, direction)
               ? EventResult::handled
               : EventResult::ignored;
}

} // namespace sasd::ui
