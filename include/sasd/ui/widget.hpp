#pragma once

#include <sasd/ui/component.hpp>
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

private:
    friend class Container;

    void setParent(Container* parent) noexcept { parent_ = parent; }

    Container* parent_{nullptr};
    Rect bounds_{};
    bool visible_{true};
    bool enabled_{true};
};

} // namespace sasd::ui
