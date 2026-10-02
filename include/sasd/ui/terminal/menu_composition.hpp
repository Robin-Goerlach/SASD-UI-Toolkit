#pragma once

#include <sasd/ui/terminal/menu_interaction_presentation.hpp>

#include <optional>
#include <utility>

namespace sasd::ui::terminal {

/**
 * Complete owned terminal frame metadata after transient menu composition.
 *
 * The cell buffer and hardware-caret request intentionally travel together. Menus can visually cover
 * application content without changing the semantic focus owner, so carrying only the composed cells would
 * leave the caller with no authoritative answer about whether the application's caret should still be
 * exposed while menu interaction owns keyboard attention.
 */
struct MenuComposedPresentationFrame {
    ScreenBuffer buffer;
    std::optional<Point> caret{};
};

/**
 * Composes the current terminal menu interaction over an immutable application/base frame and caret request.
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
 * The caret follows the same current-state rule. While MenuInteractionController is active, menu navigation
 * owns keyboard attention and the application caret is suppressed in the composed metadata. The base caret
 * is not destroyed or rewritten; when menu interaction becomes inactive and composition runs again, the
 * original base caret is propagated again. This avoids mutating FocusManager/TextField state merely to hide
 * a terminal cursor during a transient menu interaction.
 *
 * The base buffer is never mutated. Its size is also the viewport used by the menu frame builder because
 * renderMenuInteractionPresentation() derives placement from the destination copy's size. This keeps base
 * geometry, viewport fitting, and the final composed frame mechanically consistent.
 *
 * Failure is transactional at the owned-frame boundary. If menu state is stale, text is not representable,
 * or placement cannot produce a complete frame, std::nullopt is returned and neither base cells nor base
 * caret metadata are modified. Allocation failures while copying the base remain ordinary exceptions rather
 * than being translated into a semantic presentation failure.
 *
 * The explicit base-frame contract is intentional. This function does not know how the application frame
 * or caret was produced, does not retain a previous composed frame, and does not present bytes to
 * TerminalSession. A later optimization may replace the full copy with damage-aware composition while
 * preserving the same observable contract: each menu frame is derived from current semantic state plus an
 * explicit base frame and base caret request.
 *
 * @returns an owned composed frame, or std::nullopt when the menu interaction cannot be presented completely
 *          under the current terminal presentation policy.
 */
[[nodiscard]] inline std::optional<MenuComposedPresentationFrame>
composeMenuInteractionFrame(
    const ScreenBuffer& base,
    std::optional<Point> base_caret,
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

    /*
     * Active menu interaction temporarily owns keyboard attention, but it does not steal semantic focus
     * from the underlying widget tree. Hiding only the presentation caret keeps those two responsibilities
     * separate and lets a later reset expose the original caret again without a focus round trip.
     */
    const std::optional<Point> composed_caret = controller.isActive() ? std::nullopt : base_caret;

    return MenuComposedPresentationFrame{
        std::move(composed),
        composed_caret,
    };
}

/**
 * Compatibility convenience for callers interested only in cell composition.
 *
 * This preserves the original ADR 0072 API and delegates to the metadata-aware composition path with no
 * base caret. There is intentionally only one implementation of transient cell lifetime semantics.
 */
[[nodiscard]] inline std::optional<ScreenBuffer>
composeMenuInteractionPresentation(
    const ScreenBuffer& base,
    const MenuBarModel& bar,
    const MenuInteractionController& controller,
    Point menu_bar_origin = {},
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    auto composed = composeMenuInteractionFrame(
        base,
        std::nullopt,
        bar,
        controller,
        menu_bar_origin,
        ambiguous_width);
    if (!composed.has_value()) {
        return std::nullopt;
    }

    return std::move(composed->buffer);
}

} // namespace sasd::ui::terminal
