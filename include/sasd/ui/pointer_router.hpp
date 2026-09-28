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
 * - PointerRouter preserves the original press target across move/release by capture.
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
     * On the first button press, the deepest hit target is temporarily captured before dispatch.
     * Registering capture before application/widget code runs is important: if the target is destroyed
     * synchronously, Widget::~Widget() can notify this router and clear the observation safely.
     *
     * If the complete press route ignores the event, capture is released immediately. A handled
     * press keeps capture until the matching button release. Motion and other transitions are routed
     * to the captured target even when the pointer leaves its bounds.
     *
     * A captured Widget detached from root is released before the next event; capture never routes
     * input into an unrelated visual tree.
     */
    [[nodiscard]] PointerRouteResult route(Widget& root, const PointerEvent& event);

    /** Explicitly releases the current capture, if any. Safe to call repeatedly. */
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
