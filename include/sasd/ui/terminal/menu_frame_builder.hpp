#pragma once

#include <sasd/ui/menu_interaction_view.hpp>
#include <sasd/ui/terminal/menu_placement.hpp>
#include <sasd/ui/terminal/menu_viewport_placement.hpp>

#include <cstddef>
#include <optional>
#include <utility>

namespace sasd::ui::terminal {

/**
 * Builds one complete owned terminal-menu frame from semantic interaction state and viewport policy.
 *
 * This helper is the composition boundary between backend-neutral menu interaction and terminal-specific
 * presentation geometry. It deliberately does not render, emit ANSI/VT, mutate MenuInteractionController,
 * or retain MenuModel/MenuItem pointers. Instead it snapshots the current semantic state, resolves every
 * open popup level again, applies the existing placement policies, and returns one self-contained
 * MenuFramePresentationSnapshot suitable for transactional rendering.
 *
 * The menu bar is always snapshotted, even when menu interaction is inactive. If no popup is open, the
 * returned frame contains only that persistent bar. Once popup state is open, every represented level must
 * be provable against the current model. Stale structural state therefore fails closed with std::nullopt
 * rather than returning a partially reconstructed popup chain.
 *
 * Root placement is intentionally two-stage: naturalMenuBarPopupOrigin() preserves structural relation to
 * the selected top-level title, then fitMenuPopupOriginToViewport() keeps the complete root visible. Child
 * levels use fitSubmenuPopupToViewport(), preserving a meaningful left/right relation to their parent. The
 * chosen side is written back to the parent layer as ActiveSubmenuPresentationDirection, so the resulting
 * frame contains all chrome information needed by renderMenuPresentationFrame().
 *
 * A parent level that claims to have an open child must expose a currently valid submenu selection in its
 * owned snapshot. This is stricter than merely trusting controller.popupDepth(): presentation can run after
 * application-side structural mutation but before the controller's next normalization transaction. Failing
 * closed here prevents geometry from being attached to an unprovable semantic route.
 *
 * @returns a complete owned frame, or std::nullopt when the viewport is malformed, current popup state is
 *          stale/unresolvable, menu text is not representable, or any complete popup placement is impossible.
 */
[[nodiscard]] inline std::optional<MenuFramePresentationSnapshot>
buildMenuPresentationFrame(const MenuBarModel& bar,
                           const MenuInteractionController& controller,
                           Point menu_bar_origin,
                           Size viewport,
                           AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    if (viewport.width < 0 || viewport.height < 0) {
        return std::nullopt;
    }

    MenuFramePresentationSnapshot frame;
    frame.menu_bar_origin = menu_bar_origin;
    frame.menu_bar = snapshotMenuBarPresentation(bar, controller);

    if (!controller.popupOpen()) {
        return frame;
    }

    if (!frame.menu_bar.selection.has_value() || controller.popupDepth() == 0U) {
        return std::nullopt;
    }

    auto root_snapshot = snapshotMenuPopupPresentation(bar, controller, 0U);
    if (!root_snapshot.has_value()) {
        return std::nullopt;
    }

    const auto natural_root = naturalMenuBarPopupOrigin(
        frame.menu_bar, *frame.menu_bar.selection, menu_bar_origin, ambiguous_width);
    if (!natural_root.has_value()) {
        return std::nullopt;
    }

    const auto fitted_root = fitMenuPopupOriginToViewport(
        *root_snapshot, *natural_root, viewport, ambiguous_width);
    if (!fitted_root.has_value()) {
        return std::nullopt;
    }

    frame.popups.push_back(PositionedMenuPopupPresentationSnapshot{
        *fitted_root,
        std::move(*root_snapshot),
        std::nullopt,
    });

    for (std::size_t level = 1U; level < controller.popupDepth(); ++level) {
        auto child_snapshot = snapshotMenuPopupPresentation(bar, controller, level);
        if (!child_snapshot.has_value()) {
            return std::nullopt;
        }

        PositionedMenuPopupPresentationSnapshot& parent = frame.popups.back();
        if (!parent.snapshot.selection.has_value()) {
            return std::nullopt;
        }

        const std::size_t opener_index = *parent.snapshot.selection;
        const auto child_placement = fitSubmenuPopupToViewport(
            parent, opener_index, *child_snapshot, viewport, ambiguous_width);
        if (!child_placement.has_value()) {
            return std::nullopt;
        }

        /*
         * Record the side on the parent before vector growth can invalidate the reference. The child itself
         * has no active direction until a still-deeper popup proves that it owns an open direct descendant.
         */
        parent.active_submenu_direction = ActiveSubmenuPresentationDirection{
            opener_index,
            child_placement->side,
        };

        frame.popups.push_back(PositionedMenuPopupPresentationSnapshot{
            child_placement->origin,
            std::move(*child_snapshot),
            std::nullopt,
        });
    }

    return frame;
}

} // namespace sasd::ui::terminal
