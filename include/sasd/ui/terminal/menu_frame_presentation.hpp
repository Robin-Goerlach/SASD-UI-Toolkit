#pragma once

#include <sasd/ui/terminal/menu_bar_presentation.hpp>
#include <sasd/ui/terminal/menu_directional_presentation.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace sasd::ui::terminal {

/**
 * One owned popup snapshot together with its terminal-cell origin inside a composed menu frame.
 *
 * The optional active-submenu direction is presentation state for this popup level only. Keeping it beside
 * the owned popup snapshot makes frame rendering self-contained: callers do not have to select a different
 * renderer out-of-band after placement has already decided that an open child belongs on the left or right.
 * A popup without an open direct child leaves the optional empty and renders with the ordinary popup path.
 *
 * The origin and direction belong to presentation, not to MenuModel or MenuInteractionController. This
 * keeps terminal geometry/chrome out of the semantic Core model while still giving the composed frame all
 * data required for deterministic rendering.
 */
struct PositionedMenuPopupPresentationSnapshot {
    Point origin{};
    MenuPopupPresentationSnapshot snapshot{};
    std::optional<ActiveSubmenuPresentationDirection> active_submenu_direction{};
};

/**
 * Owned presentation transaction for the complete terminal menu surface.
 *
 * The frame deliberately contains only presentation values: a top-level menu-bar snapshot and zero or
 * more positioned popup snapshots. Popup layers are stored in paint order; later entries are painted
 * after earlier entries and therefore appear on top when their rectangles overlap. This small rule is
 * sufficient for nested popup composition without introducing a terminal-specific visual tree.
 */
struct MenuFramePresentationSnapshot {
    Point menu_bar_origin{};
    MenuBarPresentationSnapshot menu_bar{};
    std::vector<PositionedMenuPopupPresentationSnapshot> popups{};
};

/**
 * Renders a complete owned menu frame into ScreenBuffer.
 *
 * Every layer, including optional direction metadata, is preflighted before the first ScreenBuffer
 * mutation. This extends the fail-closed policy of the individual renderers to the whole menu surface: an
 * unsupported popup label or a stale direction descriptor cannot erase/restyle a valid menu bar that was
 * already present in the previous frame. Only after all snapshots and descriptors have proven consistent
 * do we paint the bar followed by popup layers in vector order.
 *
 * Popup dispatch is intentionally data-driven. A layer without active_submenu_direction uses the ordinary
 * popup renderer; a layer carrying one uses the directional wrapper. The caller therefore builds one owned
 * frame transaction and does not need an out-of-band parallel list saying which rendering function applies
 * to which popup level.
 *
 * The individual renderers intentionally run their own validation again. That duplicated work is small
 * and keeps their standalone contracts intact; a later optimization pass may reuse measurements inside
 * one frame transaction if profiling shows a material cost. Because the snapshots are owned and passed
 * as const values, representability cannot change between frame preflight and painting unless execution
 * is interrupted by an exception such as allocation failure.
 *
 * This function still stops at ScreenBuffer. It does not emit ANSI/VT, own terminal-session state, mutate
 * menu interaction, calculate popup placement, or introduce Widget ownership for transient menus.
 *
 * @returns true when every layer and optional directional descriptor was valid and painting completed;
 *          false when preflight rejected at least one layer, leaving the buffer unchanged.
 */
[[nodiscard]] inline bool
renderMenuPresentationFrame(ScreenBuffer& buffer,
                            const MenuFramePresentationSnapshot& frame,
                            AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    if (!measureMenuBarPresentation(frame.menu_bar, ambiguous_width).has_value()) {
        return false;
    }

    for (const PositionedMenuPopupPresentationSnapshot& popup : frame.popups) {
        if (!measureMenuPopupPresentation(popup.snapshot, ambiguous_width).has_value()) {
            return false;
        }

        if (popup.active_submenu_direction.has_value() &&
            !isValidActiveSubmenuPresentationDirection(
                popup.snapshot,
                *popup.active_submenu_direction)) {
            return false;
        }
    }

    /*
     * Preflight above makes false returns from the individual renderers unreachable for these immutable
     * snapshots under normal execution. Keep the checks nevertheless: they preserve fail-closed behavior
     * if a future renderer gains an additional representability condition not yet specialized here.
     */
    if (!renderMenuBarPresentation(buffer,
                                   frame.menu_bar_origin,
                                   frame.menu_bar,
                                   ambiguous_width)) {
        return false;
    }

    for (const PositionedMenuPopupPresentationSnapshot& popup : frame.popups) {
        if (popup.active_submenu_direction.has_value()) {
            if (!renderDirectionalMenuPopupPresentation(buffer,
                                                        popup.origin,
                                                        popup.snapshot,
                                                        *popup.active_submenu_direction,
                                                        ambiguous_width)) {
                return false;
            }
            continue;
        }

        if (!renderMenuPopupPresentation(buffer,
                                         popup.origin,
                                         popup.snapshot,
                                         ambiguous_width)) {
            return false;
        }
    }

    return true;
}

} // namespace sasd::ui::terminal
