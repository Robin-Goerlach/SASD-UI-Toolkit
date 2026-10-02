#pragma once

#include <sasd/ui/terminal/menu_direction.hpp>
#include <sasd/ui/terminal/menu_frame_presentation.hpp>

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

/**
 * Result of submenu-aware viewport placement.
 *
 * Keeping the chosen side beside the origin makes the directional decision explicit presentation data.
 * Callers must not infer direction later from coordinates because future overlap/gap policies could make
 * that ambiguous.
 */
struct SubmenuPopupViewportPlacement {
    Point origin{};
    SubmenuPopupSide side{SubmenuPopupSide::right};
};

/**
 * Fits a child submenu while preserving a meaningful left/right relationship to its parent popup.
 *
 * Generic viewport fitting is intentionally allowed to translate a popup horizontally to any visible
 * origin. That behavior is appropriate for root popups, but a child submenu has stronger geometry: users
 * expect it to open beside its parent. This helper therefore evaluates two side-preserving candidates in
 * deterministic order instead of merely clamping the natural x coordinate.
 *
 * Policy:
 *
 * 1. Prefer the natural right-hand side when the complete child fits there.
 * 2. If the right side does not fit, try immediately left of the parent popup.
 * 3. If neither side can contain the complete child, return std::nullopt rather than overlapping the
 *    parent or silently falling back to arbitrary horizontal clamping.
 * 4. Vertical placement remains anchor-preserving where possible and is clamped only enough to keep the
 *    child fully visible. This is independent from horizontal side choice.
 *
 * Both parent and child snapshots are measured before geometry is accepted. item_index must identify a
 * submenu row in the parent snapshot; command/separator rows are rejected even for synthetic snapshots.
 * Negative viewport dimensions, unrenderable menu text, and a child larger than the viewport fail closed.
 * All intermediate coordinate arithmetic uses int64_t so extreme caller-supplied origins cannot overflow.
 *
 * @returns the fitted side-preserving placement, or std::nullopt when no complete left/right placement
 *          exists inside the viewport.
 */
[[nodiscard]] inline std::optional<SubmenuPopupViewportPlacement>
fitSubmenuPopupToViewport(const PositionedMenuPopupPresentationSnapshot& parent,
                          std::size_t item_index,
                          const MenuPopupPresentationSnapshot& child,
                          Size viewport,
                          AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
    if (viewport.width < 0 || viewport.height < 0 ||
        item_index >= parent.snapshot.items.size() ||
        parent.snapshot.items[item_index].kind != MenuItemKind::submenu) {
        return std::nullopt;
    }

    const auto parent_measurement = measureMenuPopupPresentation(parent.snapshot, ambiguous_width);
    const auto child_measurement = measureMenuPopupPresentation(child, ambiguous_width);
    if (!parent_measurement.has_value() || !child_measurement.has_value()) {
        return std::nullopt;
    }

    const Size parent_size = parent_measurement->size;
    const Size child_size = child_measurement->size;
    if (child_size.width > viewport.width || child_size.height > viewport.height) {
        return std::nullopt;
    }

    const std::int64_t viewport_width = viewport.width;
    const std::int64_t maximum_y =
        static_cast<std::int64_t>(viewport.height) - static_cast<std::int64_t>(child_size.height);
    const std::int64_t anchor_y =
        static_cast<std::int64_t>(parent.origin.y) + static_cast<std::int64_t>(item_index);
    const std::int64_t fitted_y = std::clamp<std::int64_t>(anchor_y, 0, maximum_y);

    const std::int64_t right_x =
        static_cast<std::int64_t>(parent.origin.x) + static_cast<std::int64_t>(parent_size.width);
    const std::int64_t right_end = right_x + static_cast<std::int64_t>(child_size.width);
    if (right_x >= 0 && right_end <= viewport_width) {
        return SubmenuPopupViewportPlacement{
            Point{static_cast<Coordinate>(right_x), static_cast<Coordinate>(fitted_y)},
            SubmenuPopupSide::right,
        };
    }

    const std::int64_t left_x =
        static_cast<std::int64_t>(parent.origin.x) - static_cast<std::int64_t>(child_size.width);
    const std::int64_t left_end = left_x + static_cast<std::int64_t>(child_size.width);
    if (left_x >= 0 && left_end <= viewport_width) {
        return SubmenuPopupViewportPlacement{
            Point{static_cast<Coordinate>(left_x), static_cast<Coordinate>(fitted_y)},
            SubmenuPopupSide::left,
        };
    }

    return std::nullopt;
}

} // namespace sasd::ui::terminal
