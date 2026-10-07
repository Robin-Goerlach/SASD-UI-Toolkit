#pragma once

#include <sasd/ui/geometry.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace sasd::ui::presentation {

/**
 * Vertical side chosen for an anchored popup.
 *
 * The value is explicit presentation data rather than something callers should later infer from
 * coordinates. Keeping the decision visible matters once a presentation wants to draw a direction-
 * sensitive arrow, shadow, animation or attachment seam.
 */
enum class PopupVerticalSide {
    below,
    above,
};

/**
 * Final geometry for one fully visible popup attached to an anchor rectangle.
 *
 * bounds uses the same backend-neutral logical coordinate primitives as Widget layout. A Terminal
 * caller may interpret one logical unit as one cell while a Rendered caller may use device-independent
 * graphical units; this value itself carries no backend or device type.
 */
struct AnchoredPopupPlacement {
    Rect bounds{};
    PopupVerticalSide side{PopupVerticalSide::below};

    friend constexpr bool operator==(const AnchoredPopupPlacement&,
                                     const AnchoredPopupPlacement&) = default;
};

namespace detail {

[[nodiscard]] inline bool popupOriginFitsCoordinate(std::int64_t value) noexcept {
    return value >= static_cast<std::int64_t>(std::numeric_limits<Coordinate>::min()) &&
           value <= static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max());
}

[[nodiscard]] inline std::optional<Coordinate>
popupVerticalOriginForSide(Rect anchor,
                           Size popup_size,
                           Rect viewport,
                           PopupVerticalSide side) noexcept {
    using WideCoordinate = std::int64_t;

    const WideCoordinate anchor_top = static_cast<WideCoordinate>(anchor.y);
    const WideCoordinate anchor_bottom =
        anchor_top + static_cast<WideCoordinate>(anchor.height);
    const WideCoordinate popup_height = static_cast<WideCoordinate>(popup_size.height);

    const WideCoordinate viewport_top = static_cast<WideCoordinate>(viewport.y);
    const WideCoordinate viewport_bottom =
        viewport_top + static_cast<WideCoordinate>(viewport.height);

    const WideCoordinate candidate =
        side == PopupVerticalSide::below
            ? anchor_bottom
            : anchor_top - popup_height;
    const WideCoordinate candidate_bottom = candidate + popup_height;

    /*
     * "Fits" means the complete popup rectangle is visible on this side. We deliberately do not clamp
     * vertically across the anchor because that would destroy the attachment relationship: a popup
     * requested below could end up overlapping the ComboBox that opened it. If neither side works, the
     * caller receives std::nullopt and can later choose scrolling/size-reduction policy explicitly.
     */
    if (candidate < viewport_top ||
        candidate_bottom > viewport_bottom ||
        !popupOriginFitsCoordinate(candidate)) {
        return std::nullopt;
    }

    return static_cast<Coordinate>(candidate);
}

} // namespace detail

/**
 * Places one popup beside a concrete anchor while keeping the complete popup inside a viewport.
 *
 * This is a generic presentation-layer geometry policy intended for controls such as ComboBox and
 * future picker/tooltip surfaces. It deliberately accepts already-measured geometry. Text measurement,
 * item semantics, Widget ownership, rendering and input routing stay in their respective layers.
 *
 * Policy:
 *
 * - anchor, popup and viewport must all describe positive area;
 * - the popup itself must fit completely inside the viewport;
 * - horizontal placement starts at the anchor's left edge and is clamped only as far as necessary to
 *   keep the complete popup visible;
 * - vertical placement preserves the anchor relationship: the preferred side is tried first and the
 *   opposite side second;
 * - below means popup top == anchor bottom; above means popup bottom == anchor top;
 * - if neither vertical side can contain the complete popup, the function fails closed instead of
 *   silently clipping, shrinking, scrolling or overlapping the anchor.
 *
 * All edge arithmetic is widened to int64_t before addition/subtraction. This mirrors the toolkit's
 * existing overflow discipline for Rect hit testing and terminal menu placement: caller-supplied values
 * near Coordinate limits must never wrap signed arithmetic into plausible but incorrect geometry.
 *
 * A viewport may have a non-zero or negative origin. The returned Rect is always expressed in that
 * same logical coordinate space.
 *
 * @returns final fully visible placement, or std::nullopt when this conservative first policy cannot
 *          represent a complete attached popup.
 */
[[nodiscard]] inline std::optional<AnchoredPopupPlacement>
placeAnchoredPopup(Rect anchor,
                   Size popup_size,
                   Rect viewport,
                   PopupVerticalSide preferred_side = PopupVerticalSide::below) noexcept {
    if (anchor.isEmpty() || popup_size.isEmpty() || viewport.isEmpty()) {
        return std::nullopt;
    }

    if (popup_size.width > viewport.width || popup_size.height > viewport.height) {
        return std::nullopt;
    }

    using WideCoordinate = std::int64_t;
    const WideCoordinate viewport_left = static_cast<WideCoordinate>(viewport.x);
    const WideCoordinate viewport_right =
        viewport_left + static_cast<WideCoordinate>(viewport.width);
    const WideCoordinate popup_width = static_cast<WideCoordinate>(popup_size.width);

    const WideCoordinate minimum_x = viewport_left;
    const WideCoordinate maximum_x = viewport_right - popup_width;

    /*
     * minimum_x is a public Coordinate value. maximum_x may lie beyond Coordinate::max() when a
     * viewport itself extends past that representable edge, but the natural anchor x is also a
     * Coordinate. Clamping between the viewport left and maximum complete origin therefore cannot
     * manufacture an out-of-range x unless the viewport's own left edge is already unrepresentable,
     * which Rect cannot express.
     */
    const WideCoordinate fitted_x = std::clamp<WideCoordinate>(
        static_cast<WideCoordinate>(anchor.x),
        minimum_x,
        maximum_x);
    if (!detail::popupOriginFitsCoordinate(fitted_x)) {
        return std::nullopt;
    }

    const PopupVerticalSide fallback_side =
        preferred_side == PopupVerticalSide::below
            ? PopupVerticalSide::above
            : PopupVerticalSide::below;

    if (const auto y = detail::popupVerticalOriginForSide(
            anchor, popup_size, viewport, preferred_side)) {
        return AnchoredPopupPlacement{
            Rect{
                static_cast<Coordinate>(fitted_x),
                *y,
                popup_size.width,
                popup_size.height,
            },
            preferred_side,
        };
    }

    if (const auto y = detail::popupVerticalOriginForSide(
            anchor, popup_size, viewport, fallback_side)) {
        return AnchoredPopupPlacement{
            Rect{
                static_cast<Coordinate>(fitted_x),
                *y,
                popup_size.width,
                popup_size.height,
            },
            fallback_side,
        };
    }

    return std::nullopt;
}

/**
 * Returns one fixed-height row rectangle inside an already-established popup content area.
 *
 * The helper intentionally knows nothing about borders, padding or scrolling. A backend/presentation
 * first computes the popup's *content* Rect after applying its own chrome policy, then asks this helper
 * for deterministic row geometry. That separation lets Terminal use one-cell rows while Rendered uses
 * font/theme-derived row heights without creating two different row-index semantics.
 *
 * The complete declared row set must fit in content_bounds. This is a fail-closed precondition rather
 * than implicit clipping: scrolling/virtualization is a separate future policy and should not appear
 * accidentally merely because the last rows exceed the current rectangle.
 *
 * @returns the requested row rectangle, or std::nullopt for malformed content geometry, zero/nonpositive
 *          row height, an invalid row index, a declared row set that does not completely fit, or an
 *          origin that cannot be represented by Coordinate.
 */
[[nodiscard]] inline std::optional<Rect>
fixedPopupRowBounds(Rect content_bounds,
                    std::size_t row_count,
                    std::size_t row_index,
                    Coordinate row_height) noexcept {
    if (content_bounds.isEmpty() ||
        row_height <= 0 ||
        row_count == 0U ||
        row_index >= row_count) {
        return std::nullopt;
    }

    /*
     * Avoid multiplying an untrusted size_t by row_height merely to discover overflow. The content
     * rectangle has Coordinate-sized positive height, so division gives the exact maximum number of
     * complete fixed rows that can fit and is safe to compare after conversion to size_t.
     */
    const Coordinate maximum_complete_rows =
        static_cast<Coordinate>(content_bounds.height / row_height);
    if (row_count > static_cast<std::size_t>(maximum_complete_rows)) {
        return std::nullopt;
    }

    using WideCoordinate = std::int64_t;
    const WideCoordinate row_offset =
        static_cast<WideCoordinate>(row_index) * static_cast<WideCoordinate>(row_height);
    const WideCoordinate row_y =
        static_cast<WideCoordinate>(content_bounds.y) + row_offset;

    if (!detail::popupOriginFitsCoordinate(row_y)) {
        return std::nullopt;
    }

    return Rect{
        content_bounds.x,
        static_cast<Coordinate>(row_y),
        content_bounds.width,
        row_height,
    };
}

} // namespace sasd::ui::presentation
