#pragma once

#include <sasd/ui/events/event.hpp>

#include <cstddef>
#include <vector>

namespace sasd::ui {

class Widget;

/**
 * Summary of one pointer-routing operation.
 *
 * No Widget pointer is returned deliberately. Event handlers may synchronously detach or destroy
 * Widgets, so returning a non-owning target after dispatch would make the result easy to misuse.
 * Callers normally need only whether a target existed, whether it handled the event, how far bubbling
 * travelled, and whether a capture remains active for the next pointer event.
 */
struct PointerRouteResult {
    bool targeted{false};
    bool handled{false};
    std::size_t visited{0};
    bool capture_active{false};
};

/**
 * Backend-neutral pointer target selection, geometric hover and single-pointer capture.
 *
 * Responsibilities are deliberately narrow:
 *
 * - HitTest chooses the current deepest visual target from logical coordinates.
 * - PointerRouter mirrors the complete root-to-target geometry as direct Widget hover state.
 * - EventDispatcher performs normal target-to-parent bubbling for the actual PointerEvent.
 * - PointerRouter captures the Widget that handled a button press and preserves that interaction
 *   target across move/release without freezing geometric hover.
 *
 * Hover state is deliberately not synthesized as routed enter/leave application events yet. This
 * avoids choosing bubbling semantics before a real application use case requires them. The router
 * does not own Widgets, choose keyboard focus, activate controls or translate native mouse/touch
 * APIs. Concrete controls decide what routed PointerEvent means to them.
 */
class PointerRouter final {
public:
    PointerRouter() = default;
    ~PointerRouter();

    PointerRouter(const PointerRouter&) = delete;
    PointerRouter& operator=(const PointerRouter&) = delete;
    PointerRouter(PointerRouter&&) = delete;
    PointerRouter& operator=(PointerRouter&&) = delete;

    [[nodiscard]] bool hasCapture() const noexcept { return captured_ != nullptr; }
    [[nodiscard]] Widget* capturedWidget() noexcept { return captured_; }
    [[nodiscard]] const Widget* capturedWidget() const noexcept { return captured_; }

    /** Returns whether any Widget currently belongs to the geometric hover path. */
    [[nodiscard]] bool hasHover() const noexcept { return !hover_path_.empty(); }

    /** Returns the deepest currently hovered Widget, or nullptr when the pointer is outside root. */
    [[nodiscard]] Widget* hoveredWidget() noexcept {
        return hover_path_.empty() ? nullptr : hover_path_.back();
    }
    [[nodiscard]] const Widget* hoveredWidget() const noexcept {
        return hover_path_.empty() ? nullptr : hover_path_.back();
    }

    /**
     * Routes one semantic PointerEvent inside root's visual subtree.
     *
     * On an uncaptured button press, HitTest chooses the deepest geometric target and normal
     * EventDispatcher bubbling runs first. The Widget that actually handles that press owns capture.
     * This matters for future composite controls whose child may ignore input handled by the parent.
     *
     * A handled press keeps capture until the matching button release. Motion and other transitions
     * are routed directly to the captured handler even after leaving its bounds. An ignored press
     * never creates capture.
     *
     * Detachment from root releases capture before the next event. Releasing capture invokes the
     * Widget's noexcept capture-lost hook so transient pressed/dragging state cannot remain stale.
     *
     * Independently, every pointer event refreshes the geometric root-to-hit-target hover path.
     * Capture affects event delivery only; dragging outside a captured control therefore clears its
     * hover even while move/release continue to reach that control.
     */
    [[nodiscard]] PointerRouteResult route(Widget& root, const PointerEvent& event);

    /** Explicitly releases capture and performs noexcept control-state cleanup. Safe repeatedly. */
    void releaseCapture() noexcept;

private:
    friend class Widget;

    /** Widget destruction handshake for capture and geometric hover observations. */
    void widgetDestroyed(Widget& widget) noexcept;

    /** Widget-side invalidation after hide/geometry change makes the cached hover path stale. */
    void widgetHoverInvalidated(Widget& widget) noexcept;

    [[nodiscard]] bool beginCapture(Widget& widget, PointerButton button) noexcept;
    void updateHoverPath(Widget& root, Widget* deepest_target) noexcept;
    void clearHover() noexcept;
    [[nodiscard]] static bool belongsToRoot(const Widget& root,
                                            const Widget& widget) noexcept;

    Widget* captured_{nullptr};
    PointerButton captured_button_{PointerButton::none};

    /*
     * Root-to-deepest-target path from the most recent pointer coordinates. Every entry is observed
     * through Widget::pointer_hover_router_, making destruction lifetime-safe without ownership.
     */
    std::vector<Widget*> hover_path_;
};

} // namespace sasd::ui
