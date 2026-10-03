#pragma once

#include <sasd/ui/terminal/menu_interaction_presentation.hpp>
#include <sasd/ui/terminal/presentation_frame.hpp>

#include <optional>
#include <utility>

namespace sasd::ui::terminal {

/**
 * Compatibility name retained for the menu-composition frame introduced by ADR 0073.
 *
 * The payload is not inherently menu-specific: it is the terminal backend's general owned frame value.
 * Keeping this alias avoids needless source churn for code written against the immediately preceding API
 * while allowing later terminal presentation/transport layers to depend on TerminalPresentationFrame
 * without importing menu-specific vocabulary.
 */
using MenuComposedPresentationFrame = TerminalPresentationFrame;

/**
 * Composes the current terminal menu interaction over an immutable application/base buffer and caret.
 *
 * This is the lowest-level frame-composition primitive. It intentionally keeps the original explicit
 * buffer/caret form so compatibility callers can continue to use it and so all menu-overlay semantics live
 * in exactly one implementation. The TerminalPresentationFrame overload below delegates here rather than
 * duplicating overlay lifetime, caret-suppression, placement, or failure behavior.
 *
 * Menus are transient overlays. The supplied buffer is copied before any menu chrome is applied. Closing a
 * popup and composing again from the same application base therefore restores the cells that were previously
 * covered without retaining old popup rectangles or coupling presentation lifetime to interaction history.
 *
 * While MenuInteractionController is active, menu navigation owns keyboard attention at the presentation
 * level. The semantic application focus remains untouched, but the application hardware caret is suppressed
 * in the composed frame. When the controller becomes inactive, the supplied base caret is propagated again.
 *
 * The base buffer is never mutated. Its size is the viewport used by the menu frame builder, keeping source
 * geometry, popup fitting, and the final composed frame mechanically consistent.
 *
 * Failure is transactional at the owned-frame boundary. If current menu state is stale, text is not safely
 * representable, or placement cannot produce a complete frame, std::nullopt is returned and neither base
 * cells nor base caret metadata are changed. Allocation failure while copying the base remains an exception.
 *
 * @returns an owned terminal presentation frame, or std::nullopt when the complete interaction cannot be
 *          represented under the current terminal presentation policy.
 */
[[nodiscard]] inline std::optional<TerminalPresentationFrame>
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
    const std::optional<Point> composed_caret =
        controller.isActive() ? std::nullopt : base_caret;

    return TerminalPresentationFrame{
        std::move(composed),
        composed_caret,
    };
}

/**
 * Composes menus directly from a complete immutable TerminalPresentationFrame.
 *
 * ADR 0076 made TerminalPresentationSink::captureFrame() the explicit ownership boundary between mutable
 * widget presentation and downstream frame processing. Accepting that value directly here means callers no
 * longer need to split a captured frame back into `buffer` and `caret` merely to feed the next stage.
 *
 * The input frame remains immutable and independent. This overload delegates to the established buffer/caret
 * primitive, which performs the one required buffer copy and preserves exactly the same fail-closed and
 * caret-suppression semantics. No second menu-rendering implementation is introduced.
 */
[[nodiscard]] inline std::optional<TerminalPresentationFrame>
composeMenuInteractionFrame(
    const TerminalPresentationFrame& base,
    const MenuBarModel& bar,
    const MenuInteractionController& controller,
    Point menu_bar_origin = {},
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    return composeMenuInteractionFrame(
        base.buffer,
        base.caret,
        bar,
        controller,
        menu_bar_origin,
        ambiguous_width);
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
