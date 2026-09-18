#include <sasd/ui/widget.hpp>

#include <sasd/ui/focus_manager.hpp>

namespace sasd::ui {

Widget::~Widget() {
    /*
     * focus_manager_ is intentionally non-owning. The destruction handshake is what makes that raw
     * observation safe: a focused widget tells its still-live manager to forget the pointer before
     * the Widget storage disappears. No FocusEvent is emitted from a destructor because virtual
     * dispatch during object teardown would be surprising and unsafe.
     */
    if (focus_manager_ != nullptr) {
        focus_manager_->widgetDestroyed(*this);
    }
}

void Widget::setVisible(bool visible) {
    if (visible_ == visible) {
        return;
    }

    visible_ = visible;
    clearFocusIfIneligible();
}

void Widget::setEnabled(bool enabled) {
    if (enabled_ == enabled) {
        return;
    }

    enabled_ = enabled;
    clearFocusIfIneligible();
}

void Widget::setFocusable(bool focusable) {
    if (focusable_ == focusable) {
        return;
    }

    focusable_ = focusable;
    clearFocusIfIneligible();
}

void Widget::clearFocusIfIneligible() {
    /*
     * Property changes are allowed to synchronously trigger FocusEvent{false}. Consequently these
     * setters are deliberately not noexcept: an application-provided event handler may throw, and
     * terminating the process would be worse than propagating that exception. The focus state itself
     * is cleared before the notification is invoked by FocusManager, so invariants stay consistent.
     */
    if (focused_ && !canReceiveFocus() && focus_manager_ != nullptr) {
        (void)focus_manager_->clearFocus();
    }
}

} // namespace sasd::ui
