#pragma once

#include <sasd/ui/terminal/menu_bar_presentation.hpp>
#include <sasd/ui/terminal/menu_presentation.hpp>

#include <cstddef>
#include <vector>

namespace sasd::ui::terminal {

/**
 * One owned popup snapshot together with its terminal-cell origin inside a composed menu frame.
 *
 * The origin belongs to presentation, not to MenuModel or MenuInteractionController. Keeping it next to
 * the owned popup snapshot lets callers build a complete immutable render transaction without retaining
 * semantic menu pointers or smuggling terminal geometry back into the Core menu model.
 */
struct PositionedMenuPopupPresentationSnapshot {
    Point origin{};
    MenuPopupPresentationSnapshot snapshot{};
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
 * Every layer is preflighted before the first ScreenBuffer mutation. This extends the fail-closed policy
 * of the individual bar/popup renderers to the whole menu surface: an unsupported popup label cannot
 * erase or restyle a valid menu bar that was already present in the previous frame. Only after all
 * snapshots have proven representable do we paint the bar followed by popup layers in vector order.
 *
 * The individual renderers intentionally run their own validation again. That duplicated work is small
 * and keeps their standalone contracts intact; a later optimization pass may reuse measurements inside
 * one frame transaction if profiling shows a material cost. Because the snapshots are owned and passed
 * as const values, representability cannot change between frame preflight and painting unless execution
 * is interrupted by an exception such as allocation failure.
 *
 * This function still stops at ScreenBuffer. It does not emit ANSI/VT, own terminal-session state,
 * mutate menu interaction, calculate popup placement, or introduce Widget ownership for transient menus.
 *
 * @returns true when every layer was representable and painting completed; false when preflight rejected
 *          at least one layer, in which case the buffer is left unchanged.
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
    }

    /*
     * Preflight above makes false returns from the individual renderers unreachable for these immutable
     * snapshots under normal execution. Keep the checks nevertheless: they preserve fail-closed behavior
     * if a future renderer gains an additional representability condition that its measurement helper
     * also exposes but this composition layer has not yet specialized for.
     */
    if (!renderMenuBarPresentation(buffer,
                                   frame.menu_bar_origin,
                                   frame.menu_bar,
                                   ambiguous_width)) {
        return false;
    }

    for (const PositionedMenuPopupPresentationSnapshot& popup : frame.popups) {
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
