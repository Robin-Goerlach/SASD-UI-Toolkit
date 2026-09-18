#include <sasd/ui/events/event_dispatcher.hpp>

#include <sasd/ui/container.hpp>

namespace sasd::ui {

EventDispatchResult EventDispatcher::dispatch(Widget& target, const Event& event) {
    std::size_t visited = 0;
    Widget* current = &target;

    while (current != nullptr) {
        /*
         * Capture the visual parent before calling application/widget code. A handler may legitimately
         * detach or re-parent the current widget. In that case this event continues along the route
         * that existed when the current delivery step began instead of jumping to a newly assigned
         * parent in the middle of propagation.
         */
        Container* parent = current->parent();
        ++visited;

        if (current->handleEvent(event) == EventResult::handled) {
            return {current, visited};
        }

        current = parent;
    }

    return {nullptr, visited};
}

} // namespace sasd::ui
