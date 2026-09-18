#pragma once

#include <cstddef>

namespace sasd::ui {

class PresentationSink;
class Widget;

/**
 * Statistics for one deterministic presentation synchronization pass.
 */
struct PresentationPassResult {
    /** Number of visual Widgets traversed, including clean widgets. */
    std::size_t visited{0};

    /** Number of pending widgets offered to PresentationSink. */
    std::size_t requested{0};

    /** Number of offered widgets successfully synchronized and acknowledged. */
    std::size_t synchronized{0};

    /** Number of offered widgets deliberately left pending by the sink. */
    std::size_t deferred{0};

    /** Returns true when no requested update was deferred. */
    [[nodiscard]] bool complete() const noexcept {
        return deferred == 0;
    }
};

/**
 * Bridges the semantic Widget tree and a presentation-specific sink.
 *
 * The coordinator owns neither the tree nor the sink. A pass traverses the visual hierarchy in
 * deterministic preorder (parent before children, children in Container adoption order). Clean
 * widgets are still traversed so a dirty descendant remains discoverable even when an ancestor was
 * acknowledged independently.
 *
 * The coordinator deliberately knows nothing about drawing primitives, terminal cells, native
 * handles, clipping or frame scheduling. Its only responsibility is to consume the backend-neutral
 * "visual update pending" contract established by Widget.
 */
class PresentationCoordinator final {
public:
    PresentationCoordinator() = delete;

    /**
     * Runs one synchronous presentation pass rooted at root.
     *
     * Pending widgets are offered to sink. A synchronized result clears only that widget's pending
     * flag; deferred widgets remain pending. The full visual subtree is traversed regardless of
     * whether the root itself is clean or dirty.
     *
     * If sink throws, the exception propagates immediately. Widgets synchronized earlier in the
     * pass remain acknowledged, while the throwing widget and all not-yet-visited widgets retain
     * their prior state. Structural mutation of the visual tree from sink callbacks is unsupported.
     */
    [[nodiscard]] static PresentationPassResult synchronize(Widget& root, PresentationSink& sink);
};

} // namespace sasd::ui
