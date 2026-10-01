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

    /*
     * A forced replay means an ancestor has already synchronized the presentation surface that owns
     * this descendant. An invisible descendant therefore has nothing to draw or erase: offering it to
     * an opaque leaf renderer after visible lower layers were replayed could erase those layers again
     * (StackLayout exposes this ordering hazard directly).
     *
     * A pending invisible Widget is safe to acknowledge here because the ancestor's successful
     * subtree refresh established the clean presentation baseline first. We deliberately do not walk
     * descendants of an invisible container. They remain semantically present but have no visible
     * representation; when the container becomes visible again its visibility transition requests a
     * new subtree refresh and those descendants are replayed from their then-current state.
     *
     * This shortcut is only valid under force. An ordinary non-forced hide must still be offered to
     * the sink so an incremental backend can remove the Widget's previous representation when no
     * ancestor surface rebuild occurred.
     */
    if (force && !widget.isVisible()) {
        if (was_pending || refresh_requested) {
            widget.acknowledgeVisualUpdate();
        }
        return;
    }

    const bool should_offer = was_pending || force;
    bool refresh_descendants = force;
    bool block_descendants = false;

    /*
     * Do not filter an ordinary dirty invisible Widget here. A Widget that just became invisible can
     * still require a presentation update so an incremental terminal/rendered backend removes its
     * previous representation. The forced-replay case above is different because the owning surface
     * has already been rebuilt by an ancestor.
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

    if (block_descendants || !widget.isVisible()) {
        /*
         * Descendants of an invisible container are not part of the current visual surface. Pruning
         * here also prevents a visible child from painting through a hidden parent. Dirty descendant
         * state is intentionally retained and will be consumed when a later visible subtree replay
         * makes that branch reachable again.
         */
        return;
    }

    /*
     * Presentation follows visual parenting rather than generic Component ownership. During a forced
     * subtree refresh every visible descendant is offered in adoption order so previously clean
     * siblings are restored after the backend cleared/rebuilt the affected presentation surface.
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

PresentationPassResult PresentationCoordinator::replay(Widget& root, PresentationSink& sink) {
    PresentationPassResult result;

    /*
     * force=true is deliberately a traversal policy, not a mutation of Widget invalidation state.
     * The same helper already uses this mode for clean descendants beneath an acknowledged subtree
     * refresh. Starting at root extends that proven behavior to presentation-surface recovery.
     */
    synchronizeWidget(root, sink, result, true);
    return result;
}

} // namespace sasd::ui
