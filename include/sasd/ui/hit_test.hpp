#pragma once

#include <sasd/ui/geometry.hpp>

namespace sasd::ui {

class Widget;

/**
 * Backend-neutral visual-tree hit testing.
 *
 * Hit testing answers only the geometric/visual question "which visible Widget is topmost at this
 * point?". It deliberately does not decide whether that Widget is enabled, focusable or interested in
 * a particular PointerEvent. Those are interaction/routing policies layered on top of target
 * selection.
 *
 * The input point is expressed in the coordinate system of root's visual parent. For an ordinary
 * top-level Window whose bounds start at (0,0), this is simply the Window's logical client coordinate
 * system. Child bounds remain parent-relative exactly as they are for layout/presentation.
 */
class HitTest final {
public:
    HitTest() = delete;

    /**
     * Returns the deepest visible Widget containing point, or nullptr when root itself is not hit.
     *
     * Siblings are examined in reverse visual-child/adoption order because presentation traverses
     * them in forward order: later siblings are therefore painted later and are considered on top.
     *
     * A child's hit area is clipped by every visible ancestor's bounds. Disabled Widgets remain valid
     * hit targets; disabled-state behavior belongs to the eventual interaction handler, not geometry.
     */
    [[nodiscard]] static Widget* deepestAt(Widget& root, Point point) noexcept;
    [[nodiscard]] static const Widget* deepestAt(const Widget& root, Point point) noexcept;
};

} // namespace sasd::ui
