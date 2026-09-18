#pragma once

#include <sasd/ui/component.hpp>
#include <sasd/ui/events/event.hpp>
#include <sasd/ui/geometry.hpp>

namespace sasd::ui {

class Container;

/**
 * Base class for components that have a visual or interactively presentable representation.
 *
 * Widget stores backend-neutral state only. Native handles, terminal cells, renderer objects and
 * other backend details belong to peers/backends rather than this public class.
 */
class Widget : public Component {
public:
    Widget() = default;
    ~Widget() override = default;

    [[nodiscard]] Container* parent() noexcept { return parent_; }
    [[nodiscard]] const Container* parent() const noexcept { return parent_; }

    [[nodiscard]] bool isVisible() const noexcept { return visible_; }
    void setVisible(bool visible) noexcept { visible_ = visible; }

    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

    [[nodiscard]] Rect bounds() const noexcept { return bounds_; }
    void setBounds(Rect bounds) noexcept { bounds_ = bounds; }

    /**
     * Delivers one already-normalized semantic event to this widget.
     *
     * This method performs single-widget delivery only; parent traversal belongs to EventDispatcher.
     * The default implementation ignores every event. Derived widgets normally override onEvent()
     * rather than this public entry point so the dispatch boundary stays consistent.
     *
     * Visibility and enabled state are intentionally not interpreted here. A future focus/target
     * selection policy decides whether a widget is eligible to become the target. Once an event is
     * explicitly delivered to a widget, handleEvent() reports only that widget's semantic response.
     */
    [[nodiscard]] EventResult handleEvent(const Event& event) {
        return onEvent(event);
    }

protected:
    /**
     * Event hook for concrete widgets.
     *
     * Returning EventResult::handled consumes the event for routing purposes. Returning ignored
     * allows EventDispatcher to offer the same event to the visual parent.
     */
    [[nodiscard]] virtual EventResult onEvent(const Event&) {
        return EventResult::ignored;
    }

private:
    friend class Container;

    void setParent(Container* parent) noexcept { parent_ = parent; }

    Container* parent_{nullptr};
    Rect bounds_{};
    bool visible_{true};
    bool enabled_{true};
};

} // namespace sasd::ui
