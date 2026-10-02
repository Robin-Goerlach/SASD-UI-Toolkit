#pragma once

#include <sasd/ui/terminal/menu_presentation.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>

namespace sasd::ui::terminal {

/**
 * Fits one already-natural popup origin into a concrete terminal viewport.
 *
 * This helper is deliberately a second-stage presentation policy. Structural placement is computed by
 * naturalMenuBarPopupOrigin()/naturalSubmenuPopupOrigin(); viewport fitting then translates that origin
 * only as far as necessary to keep the complete measured popup rectangle visible. The semantic menu
 * model and interaction controller therefore remain unaware of terminal dimensions.
 *
 * The policy is conservative and intentionally simple:
 *
 * - if the popup already fits at the natural origin, that origin is preserved;
 * - negative origins are shifted to zero;
 * - right/bottom overflow is shifted left/up to the last origin that keeps the complete popup visible;
 * - if the popup itself is wider or taller than the viewport, no fully visible placement exists and the
 *   function returns std::nullopt rather than silently introducing clipping/scrolling semantics.
 *
 * Empty-height popups are valid presentation values (for example an open but currently empty menu). They
 * may be positioned on the bottom edge because they occupy no rows. Negative viewport dimensions are
 * malformed and fail closed. Popup representability is checked through the same measurement function used
 * by rendering, so this policy never manufactures geometry for text the terminal renderer would reject.
 *
 * No ScreenBuffer is required here. Passing a Size keeps viewport policy independent from storage and I/O;
 * callers that render to ScreenBuffer simply pass buffer.size().
 *
 * @returns a fully visible fitted origin, or std::nullopt when the popup cannot be represented or cannot
 *          fit completely inside the supplied viewport.
 */
[[nodiscard]] inline std::optional<Point>
fitMenuPopupOriginToViewport(const MenuPopupPresentationSnapshot& popup,
                             Point natural_origin,
                             Size viewport,
                             AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    if (viewport.width < 0 || viewport.height < 0) {
        return std::nullopt;
    }

    const auto measured = measureMenuPopupPresentation(popup, ambiguous_width);
    if (!measured.has_value()) {
        return std::nullopt;
    }

    const Size popup_size = measured->size;
    if (popup_size.width > viewport.width || popup_size.height > viewport.height) {
        return std::nullopt;
    }

    /*
     * Coordinate is int32_t, but the subtraction below is performed in int64_t so the policy remains
     * mechanically safe even if the public type changes or callers supply values close to its limits.
     * The resulting legal viewport origins are always within [0, viewport extent], which is already in
     * Coordinate range because Size uses the same underlying type.
     */
    const std::int64_t maximum_x =
        static_cast<std::int64_t>(viewport.width) - static_cast<std::int64_t>(popup_size.width);
    const std::int64_t maximum_y =
        static_cast<std::int64_t>(viewport.height) - static_cast<std::int64_t>(popup_size.height);

    const auto fitted_x = std::clamp<std::int64_t>(
        static_cast<std::int64_t>(natural_origin.x), 0, maximum_x);
    const auto fitted_y = std::clamp<std::int64_t>(
        static_cast<std::int64_t>(natural_origin.y), 0, maximum_y);

    return Point{
        static_cast<Coordinate>(fitted_x),
        static_cast<Coordinate>(fitted_y),
    };
}

} // namespace sasd::ui::terminal
