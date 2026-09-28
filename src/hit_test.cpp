#include <sasd/ui/hit_test.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/widget.hpp>

#include <cstddef>
#include <cstdint>

namespace sasd::ui {
namespace {

/**
 * Tests one Widget using point coordinates expressed in the Widget's parent coordinate system.
 *
 * Recursing with a translated point instead of accumulating an absolute origin has two advantages:
 * it mirrors the parent-relative layout model directly, and all arithmetic is widened to int64_t
 * before subtraction so extreme int32_t coordinates cannot overflow.
 */
[[nodiscard]] const Widget* deepestAtImpl(const Widget& widget,
                                          std::int64_t point_x,
                                          std::int64_t point_y) noexcept {
    if (!widget.isVisible()) {
        return nullptr;
    }

    const Rect bounds = widget.bounds();
    if (bounds.isEmpty()) {
        return nullptr;
    }

    const auto left = static_cast<std::int64_t>(bounds.x);
    const auto top = static_cast<std::int64_t>(bounds.y);
    const auto right = left + static_cast<std::int64_t>(bounds.width);
    const auto bottom = top + static_cast<std::int64_t>(bounds.height);

    if (point_x < left || point_x >= right ||
        point_y < top || point_y >= bottom) {
        return nullptr;
    }

    /*
     * Once the point is known to lie inside this Widget, translate it into this Widget's local
     * coordinate system. Descendant bounds are relative to this origin.
     */
    const std::int64_t local_x = point_x - left;
    const std::int64_t local_y = point_y - top;

    if (const auto* container = dynamic_cast<const Container*>(&widget)) {
        /*
         * Presentation paints children in forward adoption order. Reverse iteration therefore finds
         * the visually topmost sibling first when bounds overlap.
         */
        for (std::size_t index = container->childCount(); index > 0; --index) {
            const Widget& child = container->childAt(index - 1);
            if (const Widget* hit = deepestAtImpl(child, local_x, local_y)) {
                return hit;
            }
        }
    }

    return &widget;
}

/**
 * Walks from the topmost ancestor down to widget by recursion.
 *
 * Input coordinates remain in the top-level parent's space. Each successful recursion level returns
 * the point translated into that ancestor's local space for the child check. This avoids allocating
 * an ancestor vector on every captured release while still honoring clipping at every level.
 */
[[nodiscard]] bool containsPath(const Widget& widget,
                                std::int64_t top_x,
                                std::int64_t top_y,
                                std::int64_t& local_x,
                                std::int64_t& local_y) noexcept {
    std::int64_t parent_local_x = top_x;
    std::int64_t parent_local_y = top_y;

    if (const Container* parent = widget.parent()) {
        if (!containsPath(*parent, top_x, top_y, parent_local_x, parent_local_y)) {
            return false;
        }
    }

    if (!widget.isVisible()) {
        return false;
    }

    const Rect bounds = widget.bounds();
    if (bounds.isEmpty()) {
        return false;
    }

    const auto left = static_cast<std::int64_t>(bounds.x);
    const auto top = static_cast<std::int64_t>(bounds.y);
    const auto right = left + static_cast<std::int64_t>(bounds.width);
    const auto bottom = top + static_cast<std::int64_t>(bounds.height);

    if (parent_local_x < left || parent_local_x >= right ||
        parent_local_y < top || parent_local_y >= bottom) {
        return false;
    }

    local_x = parent_local_x - left;
    local_y = parent_local_y - top;
    return true;
}

} // namespace

const Widget* HitTest::deepestAt(const Widget& root, Point point) noexcept {
    return deepestAtImpl(
        root,
        static_cast<std::int64_t>(point.x),
        static_cast<std::int64_t>(point.y));
}

Widget* HitTest::deepestAt(Widget& root, Point point) noexcept {
    /*
     * The const implementation performs no mutation and returns an object already owned by the same
     * non-const tree supplied by the caller. Reusing it avoids two subtly different hit-test rules.
     */
    return const_cast<Widget*>(
        deepestAt(static_cast<const Widget&>(root), point));
}

bool HitTest::contains(const Widget& widget, Point point) noexcept {
    std::int64_t local_x = 0;
    std::int64_t local_y = 0;
    return containsPath(
        widget,
        static_cast<std::int64_t>(point.x),
        static_cast<std::int64_t>(point.y),
        local_x,
        local_y);
}

} // namespace sasd::ui
