#pragma once

#include <sasd/ui/component.hpp>
#include <sasd/ui/events/event.hpp>
#include <sasd/ui/geometry.hpp>

namespace sasd::ui {

class Container;
class FocusManager;

/**
 * Base class for components that have a visual or interactively presentable representation.
 *
 * Widget stores backend-neutral state only. Native handles, terminal cells, renderer objects and
 * other backend details belong to peers/backends rather than this public class.
 */
class Widget : public Component {
public:
    Widget() = default;
    ~Widget() override;

    [[nodiscard]] Container* parent() noexcept { return parent_; }
    [[nodiscard]] const Container* parent() const noexcept { return parent_; }

    [[nodiscard]] bool isVisible() const noexcept { return visible_; }

    /**
     * Changes this widget's local visibility state.
     *
     * Hiding a currently focused widget makes it locally ineligible for focus and therefore clears
     * that focus synchronously. The resulting FocusEvent may execute user code.
     */
    void setVisible(bool visible);

    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }

    /**
     * Changes this widget's local enabled state.
     *
     * Disabling a currently focused widget clears focus synchronously because disabled controls must
     * not remain keyboard-input targets.
     */
    void setEnabled(bool enabled);

    /**
     * Returns whether this widget type/instance is allowed to receive logical keyboard focus.
     *
     * Widgets are deliberately non-focusable by default. Concrete interactive controls such as a
     * future Button or TextField can opt in explicitly instead of making containers and labels
     * accidental keyboard targets.
     */
    [[nodiscard]] bool isFocusable() const noexcept { return focusable_; }

    /**
     * Enables or disables this widget's ability to receive focus.
     *
     * Making the currently focused widget non-focusable clears focus synchronously.
     */
    void setFocusable(bool focusable);

    /** Returns whether a FocusManager currently designates this widget as its focused widget. */
    [[nodiscard]] bool hasFocus() const noexcept { return focused_; }

    /**
     * Returns whether this widget is locally eligible to become the focus target.
     *
     * M1 intentionally evaluates the widget's own focusable/visible/enabled state only. Effective
     * inherited state from hidden/disabled ancestors belongs to the later focus-scope/navigation
     * rules, once Window and real container behavior exist. Keeping that distinction explicit avoids
     * inventing parent-state semantics before the first real backend can validate them.
     */
    [[nodiscard]] bool canReceiveFocus() const noexcept {
        return focusable_ && visible_ && enabled_;
    }

    [[nodiscard]] Rect bounds() const noexcept { return bounds_; }
    void setBounds(Rect bounds) noexcept { bounds_ = bounds; }

    /**
     * Delivers one already-normalized semantic event to this widget.
     *
     * This method performs single-widget delivery only; parent traversal belongs to EventDispatcher.
     * The default implementation ignores every event. Derived widgets normally override onEvent()
     * rather than this public entry point so the dispatch boundary stays consistent.
     *
     * Visibility and enabled state are intentionally not interpreted here. Target-selection policy
     * decides whether a widget is eligible to receive ordinary input. Once an event is explicitly
     * delivered, handleEvent() reports only that widget's semantic response.
     */
    [[nodiscard]] EventResult handleEvent(const Event& event) {
        return onEvent(event);
    }

protected:
    /**
     * Event hook for concrete widgets.
     *
     * Returning EventResult::handled consumes an ordinarily routed event. FocusManager-generated
     * FocusEvent notifications are direct state notifications; their return value is intentionally
     * ignored because focus changes are not vetoable through event propagation.
     */
    [[nodiscard]] virtual EventResult onEvent(const Event&) {
        return EventResult::ignored;
    }

private:
    friend class Container;
    friend class FocusManager;

    void setParent(Container* parent) noexcept { parent_ = parent; }

    /**
     * Updates the two sides of the Widget/FocusManager invariant together.
     *
     * focus_manager_ is non-owning and is non-null only while focused_ is true. FocusManager and the
     * Widget destructor cooperate so neither side retains a pointer to an already-destroyed object.
     */
    void setFocusState(bool focused, FocusManager* focus_manager) noexcept {
        focused_ = focused;
        focus_manager_ = focus_manager;
    }

    /** Clears current focus after a local property change made this widget ineligible. */
    void clearFocusIfIneligible();

    Container* parent_{nullptr};
    FocusManager* focus_manager_{nullptr};
    Rect bounds_{};
    bool visible_{true};
    bool enabled_{true};
    bool focusable_{false};
    bool focused_{false};
};

} // namespace sasd::ui
