#pragma once

#include <sasd/ui/events/event.hpp>

#include <cstddef>

namespace sasd::ui {

class Widget;

/**
 * Summary of one synchronous event-routing operation.
 *
 * handler is a non-owning pointer to the widget that returned EventResult::handled, or nullptr when
 * the event reached the root without being consumed. visited counts how many widgets were offered
 * the event and is useful for deterministic tests and diagnostics.
 */
struct EventDispatchResult {
    Widget* handler{nullptr};
    std::size_t visited{0};

    [[nodiscard]] bool handled() const noexcept {
        return handler != nullptr;
    }
};

/**
 * Routes an already-targeted semantic event through the visual widget hierarchy.
 *
 * M1 deliberately keeps target selection separate from propagation:
 *
 *   target -> visual parent -> ... -> root
 *
 * The target receives the event first. Routing stops immediately when a widget returns
 * EventResult::handled; otherwise the exact same Event object is offered to each visual parent.
 * Ownership-only relationships to non-visual Components are not part of this route.
 *
 * EventDispatcher does not choose keyboard focus, perform hit testing, filter disabled/hidden
 * widgets or translate platform input. Those are separate responsibilities that can evolve without
 * changing the basic propagation contract.
 */
class EventDispatcher final {
public:
    EventDispatcher() = delete;

    /**
     * Dispatches event synchronously starting at target.
     *
     * The current widget's parent is captured before invoking its handler. Consequently, detaching
     * or re-parenting the current widget does not unexpectedly redirect an event halfway through that
     * handler. Event handlers must nevertheless not destroy ancestors that are still participating in
     * the active route; a future deferred-mutation mechanism can relax that restriction if needed.
     */
    [[nodiscard]] static EventDispatchResult dispatch(Widget& target, const Event& event);
};

} // namespace sasd::ui
