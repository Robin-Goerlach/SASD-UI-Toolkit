#include <sasd/ui/focus_manager.hpp>

#include <sasd/ui/events/event.hpp>
#include <sasd/ui/widget.hpp>

namespace sasd::ui {

FocusManager::~FocusManager() {
    /*
     * Destruction must sever the reverse Widget -> FocusManager observation as well. We intentionally
     * do not deliver FocusEvent{false} here: a manager destructor is lifetime cleanup, not a normal UI
     * focus transition, and it must not invoke arbitrary application code while objects are tearing
     * down.
     */
    if (focused_ != nullptr) {
        focused_->setFocusState(false, nullptr);
        focused_ = nullptr;
    }
}

bool FocusManager::requestFocus(Widget& widget) {
    if (focused_ == &widget) {
        // Focus requests are idempotent. Re-emitting gained events would make controls observe
        // duplicate transitions that never actually happened.
        return true;
    }

    if (!widget.canReceiveFocus()) {
        return false;
    }

    if (widget.focus_manager_ != nullptr && widget.focus_manager_ != this) {
        // One Widget cannot simultaneously belong to two logical focus scopes.
        return false;
    }

    /*
     * Complete focus loss before assigning the new target. Besides giving the transition a simple
     * and observable order, this handles re-entrant focus requests predictably: if the old widget's
     * FocusEvent{false} callback focuses some third widget, that nested request wins and the outer
     * request stops instead of overwriting it.
     */
    if (focused_ != nullptr) {
        Widget* previous = focused_;
        focused_ = nullptr;
        previous->setFocusState(false, nullptr);

        (void)previous->handleEvent(FocusEvent{false});

        if (focused_ != nullptr) {
            return focused_ == &widget;
        }
    }

    /*
     * The previous focus-loss callback may have changed the requested widget's state even when it did
     * not focus another widget. Re-check eligibility before committing the new focus.
     */
    if (!widget.canReceiveFocus()) {
        return false;
    }

    focused_ = &widget;
    widget.setFocusState(true, this);

    /*
     * FocusEvent is a notification of a transition that has already happened, not a cancellable
     * request. Deliver it directly to the affected widget and ignore EventResult. Ordinary key/text
     * input still uses EventDispatcher and its target-to-parent bubbling rules.
     */
    (void)widget.handleEvent(FocusEvent{true});

    // A gained-focus callback may synchronously move/clear focus again. Report final state rather than
    // the intermediate assignment performed above.
    return focused_ == &widget;
}

bool FocusManager::clearFocus() {
    if (focused_ == nullptr) {
        return false;
    }

    /*
     * Clear both sides of the relation before application code runs. If the focus-lost handler starts
     * a new focus request, it sees an internally consistent "no current focus" state and can establish
     * the next focus normally.
     */
    Widget* previous = focused_;
    focused_ = nullptr;
    previous->setFocusState(false, nullptr);

    (void)previous->handleEvent(FocusEvent{false});
    return true;
}

void FocusManager::widgetDestroyed(Widget& widget) noexcept {
    if (focused_ == &widget) {
        focused_ = nullptr;
    }

    /*
     * This also makes Widget destruction safe if a future invariant bug ever calls us for a widget
     * that is no longer the manager's current pointer: teardown still removes the reverse reference.
     */
    widget.setFocusState(false, nullptr);
}

} // namespace sasd::ui
