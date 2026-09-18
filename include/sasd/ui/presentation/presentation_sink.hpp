#pragma once

#include <cstdint>

namespace sasd::ui {

class Widget;

/**
 * Outcome of synchronizing one widget's current semantic visual state.
 */
enum class PresentationUpdateResult : std::uint8_t {
    /**
     * The sink successfully synchronized the widget state that was presented to it.
     *
     * PresentationCoordinator may acknowledge the widget immediately after this result.
     */
    synchronized,

    /**
     * The sink intentionally leaves the widget pending for a later presentation pass.
     *
     * Typical reasons may include a temporarily unavailable native peer, a renderer that has not
     * acquired a surface yet, or a terminal update that is deliberately postponed.
     */
    deferred,
};

/**
 * Backend-neutral consumer of visual widget updates.
 *
 * The sink receives a const Widget because presentation is expected to observe semantic state rather
 * than mutate the component tree. Terminal, rendered and native-peer implementations may consume
 * the same contract differently.
 *
 * Implementations must not structurally mutate the visual Widget tree while a
 * PresentationCoordinator pass is in progress. The coordinator traverses non-owning Widget
 * references, so insertion/removal during the pass would make traversal semantics ambiguous.
 */
class PresentationSink {
public:
    virtual ~PresentationSink() = default;

    /**
     * Synchronizes the current visual state of one pending widget.
     *
     * The coordinator calls this only when Widget::isVisualUpdatePending() is true. Returning
     * synchronized causes the coordinator to acknowledge that widget; returning deferred keeps it
     * pending. Exceptions propagate to the caller and leave the current widget unacknowledged.
     */
    [[nodiscard]] virtual PresentationUpdateResult synchronize(const Widget& widget) = 0;
};

} // namespace sasd::ui
