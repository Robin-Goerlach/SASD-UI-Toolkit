#pragma once

#include <sasd/ui/terminal/menu_interaction_presentation.hpp>

#include <optional>

namespace sasd::ui::terminal {

/**
 * Composes the current terminal menu interaction over an immutable application/base frame.
 *
 * Menus are transient overlays. Rendering directly into a long-lived ScreenBuffer is sufficient for one
 * frame, but it cannot by itself restore cells that belonged to a popup which has just closed: the current
 * menu snapshot quite correctly contains no geometry for that old popup anymore. Remembering every prior
 * popup rectangle inside menu code would introduce retained damage state and couple presentation lifetime
 * to interaction history.
 *
 * This helper chooses the simpler correctness-first model. It copies the supplied base frame, renders the
 * complete current menu interaction into that owned copy, and returns the composed value only after the
 * existing builder/renderer pipeline accepts the whole interaction. Callers can therefore rebuild every
 * visible menu frame from stable application content. When a popup closes, composing again from the same
 * base naturally restores the cells that had been covered by the old popup without any explicit erase
 * operation or stale-rectangle bookkeeping.
 *
 * The base buffer is never mutated. Its size is also the viewport used by the menu frame builder because
 * renderMenuInteractionPresentation() derives placement from the destination copy's size. This keeps base
 * geometry, viewport fitting, and the final composed frame mechanically consistent.
 *
 * Failure is transactional at the owned-frame boundary. If menu state is stale, text is not representable,
 * or placement cannot produce a complete frame, std::nullopt is returned and the caller's base buffer is
 * unchanged. Allocation failures while copying the base remain ordinary exceptions rather than being
 * translated into a semantic presentation failure.
 *
 * The explicit base-frame contract is intentional. This function does not know how the application frame
 * was produced, does not retain a previous composed frame, and does not present bytes to TerminalSession.
 * A later optimization may replace the full copy with damage-aware composition while preserving the same
 * observable contract: each menu frame is derived from current semantic state plus an explicit base frame.
 *
 * @returns an owned composed ScreenBuffer, or std::nullopt when the menu interaction cannot be presented
 *          completely under the current terminal presentation policy.
 */
[[nodiscard]] inline std::optional<ScreenBuffer>
composeMenuInteractionPresentation(
    const ScreenBuffer& base,
    const MenuBarModel& bar,
    const MenuInteractionController& controller,
    Point menu_bar_origin = {},
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    ScreenBuffer composed = base;
    if (!renderMenuInteractionPresentation(
            composed,
            bar,
            controller,
            menu_bar_origin,
            ambiguous_width)) {
        return std::nullopt;
    }

    return composed;
}

} // namespace sasd::ui::terminal
