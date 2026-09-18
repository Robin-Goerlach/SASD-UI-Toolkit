#include <sasd/ui/presentation/presentation_coordinator.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/presentation/presentation_sink.hpp>

namespace sasd::ui {

void PresentationCoordinator::synchronizeWidget(Widget& widget,
                                                PresentationSink& sink,
                                                PresentationPassResult& result,
                                                bool force) {
    ++result.visited;

    const bool was_pending = widget.isVisualUpdatePending();
    const bool refresh_requested = widget.isSubtreeRefreshPending();
    const bool should_offer = was_pending || force;
    bool refresh_descendants = force;
    bool block_descendants = false;

    /*
     * Do not filter on visibility here. A widget that just became invisible can still require a
     * presentation update so a terminal/backend removes its previous representation.
     */
    if (should_offer) {
        ++result.requested;
        if (force && !was_pending) {
            ++result.forced;
        }

        try {
            const PresentationUpdateResult update = sink.synchronize(widget);

            if (update == PresentationUpdateResult::synchronized) {
                /*
                 * Capture the stronger refresh request before acknowledgement clears it. A successful
                 * subtree-root synchronization authorizes replay of clean descendants.
                 */
                refresh_descendants = refresh_descendants || refresh_requested;
                widget.acknowledgeVisualUpdate();
                ++result.synchronized;
            } else {
                ++result.deferred;

                /*
                 * A clean Widget can be offered only because an ancestor forced a replay. If that
                 * replay is deferred, make the Widget normally pending so a later incremental pass
                 * cannot forget it after the ancestor refresh flag is cleared.
                 */
                if (!was_pending) {
                    widget.invalidateVisual();
                }

                /*
                 * A subtree refresh is an ordered operation: the sink must first prepare/synchronize
                 * the subtree root before descendants can be replayed coherently.
                 */
                if (refresh_requested) {
                    block_descendants = true;
                }
            }
        } catch (...) {
            if (!was_pending) {
                widget.invalidateVisual();
            }
            throw;
        }
    }

    if (block_descendants) {
        return;
    }

    /*
     * Presentation follows visual parenting rather than generic Component ownership. During a forced
     * subtree refresh every descendant is offered in adoption order so previously clean siblings are
     * restored after the backend cleared/rebuilt the affected presentation surface.
     */
    if (auto* container = dynamic_cast<Container*>(&widget)) {
        const std::size_t children = container->childCount();
        for (std::size_t index = 0; index < children; ++index) {
            synchronizeWidget(container->childAt(index), sink, result, refresh_descendants);
        }
    }
}

PresentationPassResult PresentationCoordinator::synchronize(Widget& root, PresentationSink& sink) {
    PresentationPassResult result;
    synchronizeWidget(root, sink, result, false);
    return result;
}

} // namespace sasd::ui
