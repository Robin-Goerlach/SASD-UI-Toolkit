#pragma once

#include <sasd/ui/terminal/menu_frame_builder.hpp>

namespace sasd::ui::terminal {

/**
 * Builds and renders the current terminal menu interaction as one transactional presentation step.
 *
 * This helper is intentionally a thin orchestration boundary over buildMenuPresentationFrame() and
 * renderMenuPresentationFrame(). It exists for application/backend call sites that want the normal
 * "semantic menu state -> terminal cells" path without manually plumbing the intermediate owned frame,
 * while preserving the lower-level builder and renderer as independently testable primitives.
 *
 * The ScreenBuffer supplies the concrete viewport through buffer.size(). No terminal-session object,
 * ANSI encoder, native handle, or Widget ownership enters this layer. menu_bar_origin is expressed in
 * terminal cells, and ambiguous_width is forwarded unchanged to both frame construction and rendering so
 * placement and painting use one coherent Unicode width policy.
 *
 * Failure remains fail-closed across the complete operation:
 *
 * - if the current interaction state cannot be reconstructed into a complete owned frame, the function
 *   returns false before the buffer is touched;
 * - if frame rendering rejects representability or directional metadata, the renderer's own transactional
 *   preflight leaves the previous buffer contents unchanged.
 *
 * This function paints only the menu geometry that exists in the current interaction snapshot. It does not
 * remember or erase geometry from an older popup that has since closed. Callers that repeatedly compose
 * transient menus over stable application content should rebuild from an explicit base frame through
 * composeMenuInteractionPresentation() rather than treating this in-place primitive as a retained overlay
 * manager. Keeping old-frame damage outside this function prevents hidden presentation history from entering
 * the semantic menu pipeline.
 *
 * The function does not mutate MenuInteractionController, normalize stale semantic state, execute commands,
 * present the ScreenBuffer to a terminal device, or retain pointers into MenuBarModel. Allocation failures
 * are not converted to false; they follow the surrounding library's ordinary exception behavior.
 *
 * Keeping this convenience boundary small is deliberate. It composes existing contracts rather than
 * introducing a second menu lifecycle abstraction, and a later optimization pass may reuse measurements or
 * storage inside the same public semantics without changing callers.
 *
 * @returns true when a complete frame was built and rendered; false when construction or rendering failed
 *          closed, with the buffer left unchanged for every rejected input path.
 */
[[nodiscard]] inline bool
renderMenuInteractionPresentation(ScreenBuffer& buffer,
                                  const MenuBarModel& bar,
                                  const MenuInteractionController& controller,
                                  Point menu_bar_origin = {},
                                  AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    auto frame = buildMenuPresentationFrame(
        bar,
        controller,
        menu_bar_origin,
        buffer.size(),
        ambiguous_width);
    if (!frame.has_value()) {
        return false;
    }

    return renderMenuPresentationFrame(buffer, *frame, ambiguous_width);
}

} // namespace sasd::ui::terminal
