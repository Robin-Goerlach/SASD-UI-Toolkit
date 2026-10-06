#pragma once

#include <sasd/ui/events/event.hpp>
#include <sasd/ui/menu_interaction_controller.hpp>
#include <sasd/ui/terminal/menu_hit_test.hpp>
#include <sasd/ui/terminal/menu_presentation.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>

namespace sasd::ui::terminal {

/**
 * Classification produced by terminal pointer-intent observation.
 *
 * `toward_open_submenu` is intentionally advisory. It does not select an item, delay an event,
 * mutate MenuInteractionController, or change presentation. A host may later use this information
 * to decide whether an ancestor-row motion should be deferred briefly while the pointer is moving
 * through the geometric corridor toward an already-open child popup.
 */
enum class TerminalMenuPointerIntentKind {
    none,
    toward_open_submenu,
};

/**
 * Host-owned value-state classifier for pointer motion toward the deepest open terminal submenu.
 *
 * Classic desktop menus often avoid immediately replacing an open submenu when the pointer crosses
 * a neighbouring parent row on a diagonal path toward the child. This behavior is commonly described
 * as a safe triangle or menu-aim policy. The timing and mutation parts of that policy are deliberately
 * not implemented here. This class provides only the deterministic geometric/structural foundation.
 *
 * The intended future host order is different from TerminalMenuHoverInteraction and
 * TerminalMenuCloseInteraction: pointer intent must be observed BEFORE ordinary pointer selection.
 * Otherwise selecting a sibling ancestor row may already have closed the child whose transfer corridor
 * we are trying to classify.
 *
 * A transfer anchor is established by the most recent motion report over the exact parent row that owns
 * the deepest currently-open child popup. Subsequent motion is classified as `toward_open_submenu` only
 * while all of the following remain true:
 *
 * - the selected top-level menu is unchanged;
 * - the complete open MenuPath is unchanged;
 * - frame popup count still agrees with semantic popup depth;
 * - every popup snapshot remains representable under the supplied terminal width policy;
 * - the current point lies inside the triangle from the anchor to the nearer vertical edge of the child.
 *
 * The nearer child edge is derived from final presentation geometry. This automatically works for both
 * normal right-opening submenus and viewport-fitted left-opening submenus without leaking a left/right
 * policy into Core.
 *
 * The class retains only value state: top-level index, MenuPath, parent/child levels, parent item index,
 * and one Point anchor. It retains no MenuModel/MenuItem pointer, presentation-frame pointer, Widget,
 * terminal handle, timer, callback, or thread. Every call revalidates current semantic/presentation scope.
 */
class TerminalMenuPointerIntent final {
public:
    TerminalMenuPointerIntent() = default;

    /** Clears any retained transfer anchor without changing semantic menu state. */
    void reset() noexcept { anchor_.reset(); }

    /** Returns true when a parent-row transfer anchor is currently retained. */
    [[nodiscard]] bool hasTransferAnchor() const noexcept { return anchor_.has_value(); }

    /**
     * Observes one pointer event against the PRE-interaction menu state.
     *
     * Only PointerAction::move participates. Press/release and any state that does not prove an open
     * child-popup route clear the anchor and return `none`.
     *
     * Motion over the owning parent row refreshes the anchor to the latest cell in that row. Using the
     * last point inside the parent is important: the transfer triangle should begin where the pointer
     * actually leaves the row, not at some older entry point from the opposite side of the popup.
     *
     * Reaching the child popup completes the transfer and clears the anchor. Motion elsewhere is advisory:
     * if it still lies inside the safe triangle, `toward_open_submenu` is returned and the anchor remains;
     * otherwise the anchor is retired and `none` is returned. No semantic mutation occurs in any branch.
     */
    [[nodiscard]] TerminalMenuPointerIntentKind
    observe(const MenuInteractionController& controller,
            const MenuFramePresentationSnapshot& frame,
            const PointerEvent& event,
            AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        if (event.action != PointerAction::move) {
            reset();
            return TerminalMenuPointerIntentKind::none;
        }

        const auto scope = currentTransferScope(controller, frame, ambiguous_width);
        if (!scope.has_value()) {
            reset();
            return TerminalMenuPointerIntentKind::none;
        }

        const auto hit = TerminalMenuHitTest::popupItemAt(frame, event.position, ambiguous_width);
        if (hit.has_value() && *hit == scope->parent_item) {
            /*
             * Refreshing on every motion cell inside the owning row gives the corridor a stable exit point.
             * The stored scope is copied as values, so later model mutation cannot reinterpret this anchor.
             */
            anchor_ = Anchor{
                scope->top_level_index,
                scope->popup_path,
                scope->parent_item,
                scope->child_level,
                event.position,
            };
            return TerminalMenuPointerIntentKind::none;
        }

        if (hit.has_value() && hit->level == scope->child_level) {
            /*
             * The pointer has reached the destination popup. There is no longer a parent-to-child transfer
             * to protect, and retaining the old anchor could contaminate a later unrelated motion sequence.
             */
            reset();
            return TerminalMenuPointerIntentKind::none;
        }

        if (!anchor_.has_value() || !sameScope(*anchor_, *scope)) {
            reset();
            return TerminalMenuPointerIntentKind::none;
        }

        if (pointInsideChildCorridor(frame,
                                     anchor_->anchor_point,
                                     scope->child_level,
                                     event.position,
                                     ambiguous_width)) {
            /*
             * This remains classification only. In particular, a sibling parent row can geometrically lie
             * inside the triangle; a future host policy may use this result to defer that sibling selection,
             * but this layer never suppresses or rewrites the event itself.
             */
            return TerminalMenuPointerIntentKind::toward_open_submenu;
        }

        reset();
        return TerminalMenuPointerIntentKind::none;
    }

private:
    struct TransferScope {
        std::size_t top_level_index{0};
        MenuPath popup_path{};
        TerminalMenuPopupHit parent_item{};
        std::size_t child_level{0};
    };

    struct Anchor {
        std::size_t top_level_index{0};
        MenuPath popup_path{};
        TerminalMenuPopupHit parent_item{};
        std::size_t child_level{0};
        Point anchor_point{};
    };

    struct WidePoint {
        std::int64_t x{0};
        std::int64_t y{0};
    };

    [[nodiscard]] static std::optional<TransferScope>
    currentTransferScope(const MenuInteractionController& controller,
                         const MenuFramePresentationSnapshot& frame,
                         AmbiguousWidthMode ambiguous_width) {
        if (!controller.isActive() || !controller.popupOpen() || controller.popupDepth() <= 1U) {
            return std::nullopt;
        }

        const auto top_level = controller.menuBarSelection();
        const auto& popup_path = controller.popupPath();
        const std::size_t depth = controller.popupDepth();

        if (!top_level.has_value() || !popup_path.has_value() || popup_path->empty() ||
            popup_path->size() + 1U != depth || frame.popups.size() != depth) {
            /*
             * Intent is meaningful only when semantic and presented popup depth describe the same route.
             * A stale frame therefore fails closed rather than supplying geometry for a different MenuPath.
             */
            return std::nullopt;
        }

        for (const PositionedMenuPopupPresentationSnapshot& popup : frame.popups) {
            if (!measureMenuPopupPresentation(popup.snapshot, ambiguous_width).has_value()) {
                /*
                 * Match TerminalMenuHitTest's transactional geometry rule: never derive an intent corridor
                 * from a frame that the terminal menu presentation layer cannot represent as a whole.
                 */
                return std::nullopt;
            }
        }

        const std::size_t child_level = depth - 1U;
        const std::size_t parent_level = child_level - 1U;
        const std::size_t parent_item_index = popup_path->back();

        if (parent_item_index >= frame.popups[parent_level].snapshot.items.size()) {
            return std::nullopt;
        }

        return TransferScope{
            *top_level,
            *popup_path,
            TerminalMenuPopupHit{parent_level, parent_item_index},
            child_level,
        };
    }

    [[nodiscard]] static bool sameScope(const Anchor& anchor,
                                        const TransferScope& scope) noexcept {
        return anchor.top_level_index == scope.top_level_index &&
               anchor.popup_path == scope.popup_path &&
               anchor.parent_item == scope.parent_item &&
               anchor.child_level == scope.child_level;
    }

    [[nodiscard]] static long double cross(WidePoint a,
                                           WidePoint b,
                                           WidePoint c) noexcept {
        const long double ab_x = static_cast<long double>(b.x) - static_cast<long double>(a.x);
        const long double ab_y = static_cast<long double>(b.y) - static_cast<long double>(a.y);
        const long double ac_x = static_cast<long double>(c.x) - static_cast<long double>(a.x);
        const long double ac_y = static_cast<long double>(c.y) - static_cast<long double>(a.y);
        return ab_x * ac_y - ab_y * ac_x;
    }

    [[nodiscard]] static bool pointOnSegment(WidePoint a,
                                             WidePoint b,
                                             WidePoint point) noexcept {
        if (cross(a, b, point) != 0.0L) {
            return false;
        }

        const std::int64_t min_x = a.x < b.x ? a.x : b.x;
        const std::int64_t max_x = a.x < b.x ? b.x : a.x;
        const std::int64_t min_y = a.y < b.y ? a.y : b.y;
        const std::int64_t max_y = a.y < b.y ? b.y : a.y;
        return point.x >= min_x && point.x <= max_x &&
               point.y >= min_y && point.y <= max_y;
    }

    [[nodiscard]] static bool pointInsideTriangle(WidePoint a,
                                                  WidePoint b,
                                                  WidePoint c,
                                                  WidePoint point) noexcept {
        const long double area = cross(a, b, c);
        if (area == 0.0L) {
            /*
             * A one-row child can collapse the target edge to one cell, producing a degenerate triangle.
             * Treat that case as the finite transfer segment rather than accidentally accepting every point
             * on the same infinite line.
             */
            return pointOnSegment(a, b, point) || pointOnSegment(a, c, point);
        }

        const long double first = cross(a, b, point);
        const long double second = cross(b, c, point);
        const long double third = cross(c, a, point);
        const bool has_negative = first < 0.0L || second < 0.0L || third < 0.0L;
        const bool has_positive = first > 0.0L || second > 0.0L || third > 0.0L;
        return !(has_negative && has_positive);
    }

    [[nodiscard]] static bool pointInsideChildCorridor(
        const MenuFramePresentationSnapshot& frame,
        Point anchor,
        std::size_t child_level,
        Point point,
        AmbiguousWidthMode ambiguous_width) {
        if (child_level >= frame.popups.size()) {
            return false;
        }

        const PositionedMenuPopupPresentationSnapshot& child = frame.popups[child_level];
        const auto measured = measureMenuPopupPresentation(child.snapshot, ambiguous_width);
        if (!measured.has_value() || measured->size.isEmpty()) {
            return false;
        }

        const std::int64_t left = static_cast<std::int64_t>(child.origin.x);
        const std::int64_t top = static_cast<std::int64_t>(child.origin.y);
        const std::int64_t right =
            left + static_cast<std::int64_t>(measured->size.width) - 1;
        const std::int64_t bottom =
            top + static_cast<std::int64_t>(measured->size.height) - 1;
        const std::int64_t anchor_x = static_cast<std::int64_t>(anchor.x);

        /*
         * Choose the visible vertical edge closest to the anchor rather than assuming that submenus always
         * open to the right. Viewport fitting can legitimately flip the child to the left, and final frame
         * geometry is the authoritative source for that decision.
         */
        const long double distance_to_left =
            static_cast<long double>(anchor_x >= left ? anchor_x - left : left - anchor_x);
        const long double distance_to_right =
            static_cast<long double>(anchor_x >= right ? anchor_x - right : right - anchor_x);
        const std::int64_t target_x =
            distance_to_left <= distance_to_right ? left : right;

        const WidePoint triangle_anchor{
            static_cast<std::int64_t>(anchor.x),
            static_cast<std::int64_t>(anchor.y),
        };
        const WidePoint target_top{target_x, top};
        const WidePoint target_bottom{target_x, bottom};
        const WidePoint current{
            static_cast<std::int64_t>(point.x),
            static_cast<std::int64_t>(point.y),
        };

        return pointInsideTriangle(triangle_anchor, target_top, target_bottom, current);
    }

    std::optional<Anchor> anchor_{};
};

} // namespace sasd::ui::terminal
