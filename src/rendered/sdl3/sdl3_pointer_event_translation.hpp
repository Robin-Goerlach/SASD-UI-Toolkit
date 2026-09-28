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

} // namespace sasd::ui::rendered::sdl3::detail
