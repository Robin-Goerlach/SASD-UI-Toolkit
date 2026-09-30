#pragma once

#include <sasd/ui/events/event.hpp>

#include <SDL3/SDL_events.h>

#include <optional>

namespace sasd::ui::rendered::sdl3::detail {

/**
 * Converts one SDL mouse event whose x/y fields are already expressed in SASD logical coordinates.
 *
 * Window ownership/filtering and SDL_RenderCoordinatesFromWindow() remain responsibilities of the
 * window backend. This small private seam isolates the deterministic semantic mapping from SDL's
 * platform mouse-state machinery, which is especially important for headless/offscreen tests.
 *
 * Unsupported SDL event kinds, non-finite coordinates and values outside Coordinate range return
 * nullopt instead of manufacturing a misleading semantic PointerEvent.
 */
[[nodiscard]] std::optional<PointerEvent> translateLogicalPointerEvent(
    const SDL_Event& event) noexcept;

/**
 * Converts an SDL top-level mouse enter/leave notification into the backend-neutral surface event.
 *
 * This helper deliberately does not inspect SDL's live mouse state and does not decide whether the
 * event belongs to a particular window. Sdl3WindowBackend performs native window filtering first,
 * then delegates only the deterministic SDL-event-type -> SASD-semantic mapping here.
 *
 * Keeping that mapping behind a tiny private seam lets headless/offscreen tests verify the exact
 * production translation without depending on a synthetic enter/leave sequence surviving SDL's
 * platform event-pump state reconciliation.
 */
[[nodiscard]] std::optional<PointerSurfaceEvent> translatePointerSurfaceEvent(
    const SDL_Event& event) noexcept;

} // namespace sasd::ui::rendered::sdl3::detail
