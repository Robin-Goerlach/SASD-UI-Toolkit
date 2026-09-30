#include "rendered/sdl3/sdl3_pointer_event_translation.hpp"

#include <SDL3/SDL_mouse.h>

#include <cmath>
#include <limits>

namespace sasd::ui::rendered::sdl3::detail {
namespace {

[[nodiscard]] std::optional<Point> logicalPoint(float x, float y) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return std::nullopt;
    }

    /*
     * Pointer coordinates describe a continuous location while Core geometry uses integer logical
     * units. Floor chooses the containing unit on both sides of zero; truncation would incorrectly
     * map -0.25 to 0 and turn a point just outside the top/left edge into an inside hit.
     */
    const double logical_x = std::floor(static_cast<double>(x));
    const double logical_y = std::floor(static_cast<double>(y));
    const double minimum = static_cast<double>(std::numeric_limits<Coordinate>::min());
    const double maximum = static_cast<double>(std::numeric_limits<Coordinate>::max());

    if (logical_x < minimum || logical_x > maximum ||
        logical_y < minimum || logical_y > maximum) {
        return std::nullopt;
    }

    return Point{
        static_cast<Coordinate>(logical_x),
        static_cast<Coordinate>(logical_y)};
}

[[nodiscard]] PointerButton pointerButton(Uint8 button) noexcept {
    switch (button) {
    case SDL_BUTTON_LEFT:   return PointerButton::primary;
    case SDL_BUTTON_MIDDLE: return PointerButton::middle;
    case SDL_BUTTON_RIGHT:  return PointerButton::secondary;
    case SDL_BUTTON_X1:     return PointerButton::auxiliary1;
    case SDL_BUTTON_X2:     return PointerButton::auxiliary2;
    default:                return PointerButton::other;
    }
}

} // namespace

std::optional<PointerEvent> translateLogicalPointerEvent(const SDL_Event& event) noexcept {
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        const auto position = logicalPoint(event.motion.x, event.motion.y);
        if (!position.has_value()) {
            return std::nullopt;
        }

        return PointerEvent{
            *position,
            PointerAction::move,
            PointerButton::none,
            0};
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        const auto position = logicalPoint(event.button.x, event.button.y);
        if (!position.has_value()) {
            return std::nullopt;
        }

        return PointerEvent{
            *position,
            event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                ? PointerAction::press
                : PointerAction::release,
            pointerButton(event.button.button),
            event.button.clicks};
    }

    return std::nullopt;
}

std::optional<PointerSurfaceEvent> translatePointerSurfaceEvent(
    const SDL_Event& event) noexcept {
    switch (event.type) {
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
        /*
         * Surface lifecycle intentionally carries no coordinates. Core hover is rebuilt by the next
         * real PointerEvent; inventing a position here would couple semantics to backend state.
         */
        return PointerSurfaceEvent{PointerSurfaceAction::entered};

    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        /*
         * A leave notification must remain useful even when no subsequent motion event exists.
         * Hosts can therefore retire hover/capture without fabricating an out-of-range coordinate.
         */
        return PointerSurfaceEvent{PointerSurfaceAction::left};

    default:
        return std::nullopt;
    }
}

} // namespace sasd::ui::rendered::sdl3::detail
