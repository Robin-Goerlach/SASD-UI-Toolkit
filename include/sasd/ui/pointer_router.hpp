#pragma once

#include <sasd/ui/events/event.hpp>

#include <cstddef>

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
 * Backend-neutral pointer target selection plus single-pointer capture.
 *
 * Responsibilities are deliberately narrow:
 *
 * - HitTest chooses the initial visual target from logical coordinates.
 * - EventDispatcher performs normal target-to-parent bubbling.
 * - PointerRouter captures the Widget that actually handled a button press and preserves that
 *   interaction target across move/release.
 *
 * The router does not own Widgets, choose keyboard focus, activate controls or translate native
 * mouse/touch APIs. Concrete controls decide what PointerEvent means to them.
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
     */
    [[nodiscard]] PointerRouteResult route(Widget& root, const PointerEvent& event);

    /** Explicitly releases capture and performs noexcept control-state cleanup. Safe repeatedly. */
    void releaseCapture() noexcept;

private:
    friend class Widget;

    /** Widget destruction handshake for the non-owning captured_ pointer. */
    void widgetDestroyed(Widget& widget) noexcept;

    [[nodiscard]] bool beginCapture(Widget& widget, PointerButton button) noexcept;
    [[nodiscard]] static bool belongsToRoot(const Widget& root,
                                            const Widget& widget) noexcept;

    Widget* captured_{nullptr};
    PointerButton captured_button_{PointerButton::none};
};

} // namespace sasd::ui
