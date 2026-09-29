#include <sasd/ui/pointer_router.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/widget.hpp>

#include <algorithm>
#include <vector>

namespace sasd::ui {

PointerRouter::~PointerRouter() {
    releaseCapture();
    clearHover();
}

PointerRouteResult PointerRouter::route(Widget& root, const PointerEvent& event) {
    /*
     * Captured/hovered Widgets may be detached without being destroyed. Capture and hover have
     * different semantics, but both observations must stay inside the supplied routing root.
     */
    if (captured_ != nullptr && !belongsToRoot(root, *captured_)) {
        releaseCapture();
    }
    if (!hover_path_.empty() &&
        !belongsToRoot(root, *hover_path_.back())) {
        clearHover();
    }

    /*
     * Geometric hover follows the actual pointer position even while a gesture is captured elsewhere.
     * Capture changes event delivery; it must not make the UI claim that the pointer is still over the
     * captured control after the user drags outside it.
     */
    Widget* geometric_target = HitTest::deepestAt(root, event.position);
    updateHoverPath(root, geometric_target);

    const bool had_capture = captured_ != nullptr;
    Widget* target = captured_ != nullptr ? captured_ : geometric_target;
    if (target == nullptr) {
        return {};
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

    if (previous->pointer_capture_router_ == this) {
        previous->pointer_capture_router_ = nullptr;
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

    /*
     * A Widget in the current hover path disappearing makes the remaining geometry uncertain. Clear
     * the whole path, but never call back into the Widget that is already in destruction.
     */
    const auto dying =
        std::find(hover_path_.begin(), hover_path_.end(), &widget);
    if (dying != hover_path_.end()) {
        for (Widget* hovered : hover_path_) {
            if (hovered == &widget) {
                continue;
            }
            if (hovered->pointer_hover_router_ == this) {
                hovered->pointer_hover_router_ = nullptr;
            }
            hovered->setPointerOverState(false);
        }
        hover_path_.clear();
    }

    if (widget.pointer_capture_router_ == this) {
        widget.pointer_capture_router_ = nullptr;
    }
    if (widget.pointer_hover_router_ == this) {
        widget.pointer_hover_router_ = nullptr;
    }
}

void PointerRouter::widgetHoverInvalidated(Widget& widget) noexcept {
    if (std::find(hover_path_.begin(), hover_path_.end(), &widget) !=
        hover_path_.end()) {
        clearHover();
    } else if (widget.pointer_hover_router_ == this) {
        // Defensive repair for a stale reverse link; normal operation never reaches this branch.
        widget.pointer_hover_router_ = nullptr;
        widget.setPointerOverState(false);
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
    if (widget.pointer_capture_router_ != nullptr && widget.pointer_capture_router_ != this) {
        return false;
    }

    captured_ = &widget;
    captured_button_ = button;
    widget.pointer_capture_router_ = this;
    return true;
}

void PointerRouter::updateHoverPath(Widget& root, Widget* deepest_target) noexcept {
    std::vector<Widget*> next_path;

    if (deepest_target != nullptr) {
        for (Widget* current = deepest_target;
             current != nullptr;
             current = current->parent()) {
            next_path.push_back(current);
            if (current == &root) {
                break;
            }
        }

        if (next_path.empty() || next_path.back() != &root) {
            next_path.clear();
        } else {
            std::reverse(next_path.begin(), next_path.end());
        }
    }

    std::size_t common = 0;
    while (common < hover_path_.size() &&
           common < next_path.size() &&
           hover_path_[common] == next_path[common]) {
        ++common;
    }

    /*
     * Leave deepest old Widgets first, mirroring nested visual containment. The direct state change is
     * not an application event and cannot veto routing.
     */
    for (std::size_t index = hover_path_.size(); index > common; --index) {
        Widget* previous = hover_path_[index - 1];
        if (previous->pointer_hover_router_ == this) {
            previous->pointer_hover_router_ = nullptr;
        }
        previous->setPointerOverState(false);
    }
    hover_path_.resize(common);

    /*
     * Enter ancestors before descendants. A Widget can be hover-observed by only one PointerRouter at
     * a time; this is the current single-pointer model. On an unexpected conflicting observer, stop
     * before storing an unsafe non-owning pointer and leave the deeper suffix untracked.
     */
    for (std::size_t index = common; index < next_path.size(); ++index) {
        Widget* next = next_path[index];
        if (next->pointer_hover_router_ != nullptr &&
            next->pointer_hover_router_ != this) {
            break;
        }

        next->pointer_hover_router_ = this;
        next->setPointerOverState(true);
        hover_path_.push_back(next);
    }
}

void PointerRouter::clearHover() noexcept {
    for (std::size_t index = hover_path_.size(); index > 0; --index) {
        Widget* hovered = hover_path_[index - 1];
        if (hovered->pointer_hover_router_ == this) {
            hovered->pointer_hover_router_ = nullptr;
        }
        hovered->setPointerOverState(false);
    }
    hover_path_.clear();
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
