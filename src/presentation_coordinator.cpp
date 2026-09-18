#include <sasd/ui/presentation/presentation_coordinator.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/presentation/presentation_sink.hpp>

namespace sasd::ui {
namespace {

void synchronizeWidget(Widget& widget,
                       PresentationSink& sink,
                       PresentationPassResult& result) {
    ++result.visited;

    /*
     * Do not filter on visibility here. A widget that just became invisible can still require a
     * presentation update so a terminal/backend removes its previous representation.
     */
    if (widget.isVisualUpdatePending()) {
        ++result.requested;

        /*
         * Acknowledge only after the sink reports success. If synchronize() throws, or explicitly
         * defers the update, the pending flag remains intact and a later pass can retry it.
         */
        if (sink.synchronize(widget) == PresentationUpdateResult::synchronized) {
            widget.acknowledgeVisualUpdate();
            ++result.synchronized;
        } else {
            ++result.deferred;
        }
    }

    /*
     * Presentation follows visual parenting rather than generic Component ownership. Container's
     * child API already filters out non-visual Components and preserves visual adoption order.
     *
     * Sink callbacks must not mutate this tree during a pass; doing so could invalidate references
     * that participate in traversal.
     */
    if (auto* container = dynamic_cast<Container*>(&widget)) {
        const std::size_t children = container->childCount();
        for (std::size_t index = 0; index < children; ++index) {
            synchronizeWidget(container->childAt(index), sink, result);
        }
    }
}

} // namespace

PresentationPassResult PresentationCoordinator::synchronize(Widget& root, PresentationSink& sink) {
    PresentationPassResult result;
    synchronizeWidget(root, sink, result);
    return result;
}

} // namespace sasd::ui
