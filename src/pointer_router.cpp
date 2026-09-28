#include <sasd/ui/pointer_router.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/widget.hpp>

namespace sasd::ui {

PointerRouter::~PointerRouter() {
    releaseCapture();
}

PointerRouteResult PointerRouter::route(Widget& root, const PointerEvent& event) {
    /*
     * Capture is valid only while the observed Widget still belongs to this routing scope. A Widget
     * may be released/reparented without being destroyed, so the destructor handshake alone is not
     * sufficient.
     */
    if (captured_ != nullptr && !belongsToRoot(root, *captured_)) {
        releaseCapture();
    }

    Widget* target = captured_;
    bool capture_started_for_this_event = false;

    if (target == nullptr) {
        target = HitTest::deepestAt(root, event.position);
        if (target == nullptr) {
            return {};
        }

        if (event.action == PointerAction::press &&
            event.button != PointerButton::none) {
            /*
             * Capture the geometric target, not whichever ancestor eventually handles the event.
             * This preserves the original event route: if a child ignores press and its parent Button
             * handles it, the matching release again starts at the same child and bubbles to Button.
             */
            capture_started_for_this_event = beginCapture(*target, event.button);
        }
    }

    const EventDispatchResult dispatch =
        EventDispatcher::dispatch(*target, Event{event});

    /*
     * An event handler can synchronously destroy the captured target. Widget::~Widget() clears
     * captured_ in that case, so every post-dispatch decision checks current state rather than
     * assuming the pointer still exists.
     */
    if (capture_started_for_this_event && !dispatch.handled() && captured_ != nullptr) {
        releaseCapture();
    }

    if (event.action == PointerAction::release &&
        captured_ != nullptr &&
        event.button == captured_button_) {
        releaseCapture();
    }

    return {
        true,
        dispatch.handled(),
        dispatch.visited,
        captured_ != nullptr,
    };
}

void PointerRouter::releaseCapture() noexcept {
    if (captured_ == nullptr) {
        captured_button_ = PointerButton::none;
        return;
    }

    Widget* previous = captured_;
    captured_ = nullptr;
    captured_button_ = PointerButton::none;

    if (previous->pointer_router_ == this) {
        previous->pointer_router_ = nullptr;
    }
}

void PointerRouter::widgetDestroyed(Widget& widget) noexcept {
    if (captured_ == &widget) {
        captured_ = nullptr;
        captured_button_ = PointerButton::none;
    }

    if (widget.pointer_router_ == this) {
        widget.pointer_router_ = nullptr;
    }
}

bool PointerRouter::beginCapture(Widget& widget, PointerButton button) noexcept {
    if (captured_ != nullptr) {
        return false;
    }

    /*
     * A Widget can participate in only one pointer-capture scope at a time. This mirrors FocusManager
     * and prevents two top-level routing scopes from both believing they own subsequent releases.
     */
    if (widget.pointer_router_ != nullptr && widget.pointer_router_ != this) {
        return false;
    }

    captured_ = &widget;
    captured_button_ = button;
    widget.pointer_router_ = this;
    return true;
}

bool PointerRouter::belongsToRoot(const Widget& root, const Widget& widget) noexcept {
    const Widget* current = &widget;

    while (current != nullptr) {
        if (current == &root) {
            return true;
        }
        current = current->parent();
    }

    return false;
}

} // namespace sasd::ui
