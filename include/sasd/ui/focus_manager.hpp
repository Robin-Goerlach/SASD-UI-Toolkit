#pragma once

namespace sasd::ui {

class Widget;

/**
 * Coordinates logical keyboard focus for one UI focus scope.
 *
 * FocusManager does not own widgets. It stores one non-owning pointer to the currently focused
 * Widget and cooperates with Widget destruction to keep that observation safe. A Widget can be
 * focused by at most one FocusManager at a time.
 *
 * FocusManager itself deliberately owns only focus state and explicit transitions. Deterministic
 * Tab/Shift+Tab target selection is implemented separately by FocusTraversal so event policy and
 * tree-order rules do not become hidden side effects of requestFocus(). Backend/window activation can
 * remain a further layer on top of the same small lifetime-safe contract.
 */
class FocusManager final {
public:
    FocusManager() = default;
    ~FocusManager();

    FocusManager(const FocusManager&) = delete;
    FocusManager& operator=(const FocusManager&) = delete;
    FocusManager(FocusManager&&) = delete;
    FocusManager& operator=(FocusManager&&) = delete;

    /** Returns the current focused widget, or nullptr when this focus scope has no focus. */
    [[nodiscard]] Widget* focusedWidget() noexcept { return focused_; }
    [[nodiscard]] const Widget* focusedWidget() const noexcept { return focused_; }

    /**
     * Requests logical keyboard focus for widget.
     *
     * The request is rejected without disturbing existing focus when widget is not locally eligible
     * (focusable, visible and enabled) or is already focused by another FocusManager.
     *
     * On a successful transition, the previous widget first loses focus and receives
     * FocusEvent{false}; the new widget then becomes focused and receives FocusEvent{true}. State is
     * updated before each notification. Re-requesting the already focused widget is idempotent and
     * does not emit duplicate events.
     *
     * The return value describes the state after all synchronous focus callbacks completed. A focus
     * callback may itself start another focus transition, in which case that nested transition wins.
     */
    [[nodiscard]] bool requestFocus(Widget& widget);

    /**
     * Clears the current focus, if any.
     *
     * Returns true when a widget actually lost focus. The affected widget's state is cleared before
     * FocusEvent{false} is delivered, so callbacks observe the new state.
     */
    bool clearFocus();

private:
    friend class Widget;

    /**
     * Called by Widget::~Widget() to invalidate the non-owning observation without dispatching an
     * event from an object that is already being destroyed.
     */
    void widgetDestroyed(Widget& widget) noexcept;

    Widget* focused_{nullptr};
};

} // namespace sasd::ui
