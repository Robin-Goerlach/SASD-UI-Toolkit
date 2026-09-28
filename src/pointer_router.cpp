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
     * A captured Widget may be detached without being destroyed. Do not route into an unrelated
     * visual tree; releaseCapture() also clears the control's transient gesture state.
     */
    if (captured_ != nullptr && !belongsToRoot(root, *captured_)) {
        releaseCapture();
    }

    const bool had_capture = captured_ != nullptr;
    Widget* target = captured_;

    if (target == nullptr) {
        target = HitTest::deepestAt(root, event.position);
        if (target == nullptr) {
            return {};
        }
    }

    const EventDispatchResult dispatch =
        EventDispatcher::dispatch(*target, Event{event});

    /*
     * Capture belongs to the semantic handler, not necessarily the deepest geometric target. The
     * current EventDispatcher contract requires the returned handler to remain alive for synchronous
     * dispatch. If it detached itself or capture cannot be established, immediately deliver the
     * noexcept cleanup hook so a control cannot remain armed without a matching future release.
     */
    if (!had_capture &&
        event.action == PointerAction::press &&
        event.button != PointerButton::none &&
        dispatch.handled() &&
        dispatch.handler != nullptr) {
        Widget& handler = *dispatch.handler;
        if (!belongsToRoot(root, handler) ||
            !beginCapture(handler, event.button)) {
            handler.onPointerCaptureLost();
        }
    }

    /*
     * Only the button that established capture terminates it. Other simultaneous button transitions
     * may be routed to the active control without canceling the original gesture.
     */
    if (event.action == PointerAction::release &&
        captured_ != nullptr &&
        event.button == captured_button_) {
        releaseCapture();
    }

    return {true, dispatch.handled(), dispatch.visited, captured_ != nullptr};
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

    /*
     * Clear the relation before notification. The virtual hook is noexcept and restricted to
     * transient-state cleanup/presentation invalidation, so router destruction remains safe.
     */
    previous->onPointerCaptureLost();
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
