#pragma once

#include <sasd/ui/menu_interaction_view.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui::rendered {

/** One owned row in a rendered menu popup. Geometry is the geometry that is painted and hit-tested. */
struct RenderedMenuPopupRow {
    MenuItemPresentationSnapshot item{};
    Rect bounds{};
    /** Measured label extent copied at snapshot-build time; rendering never measures again. */
    Size label_size{};
    /** The centrally formatted shortcut label, owned by this row. */
    std::string shortcut_text{};
    /** Measured shortcut extent copied at snapshot-build time. */
    Size shortcut_size{};
    /** Final lane rectangles. Empty rectangles mean that the lane is not visible for this row. */
    Rect label_bounds{};
    Rect shortcut_bounds{};
    Rect submenu_indicator_bounds{};
};

/**
 * Immutable rendered popup transaction. It owns copied semantic text and final row geometry; no
 * MenuModel, Command or measurement-context lifetime crosses this presentation boundary.
 */
struct RenderedMenuPopupPresentationSnapshot {
    Rect bounds{};
    Rect content_bounds{};
    std::vector<RenderedMenuPopupRow> rows{};
    std::optional<std::size_t> selection{};
    Coordinate padding{0};
    Coordinate row_height{0};
    Coordinate border_thickness{0};
    Coordinate label_lane_width{0};
    Coordinate shortcut_lane_width{0};
    Coordinate submenu_indicator_lane_width{0};
};

[[nodiscard]] std::optional<RenderedMenuPopupPresentationSnapshot>
buildMenuPopupPresentation(const MenuPopupPresentationSnapshot& menu,
                           Point origin,
                           Rect viewport,
                           const RenderedMeasurementContext& metrics);

/** Paints exactly the rows and bounds stored in a validated snapshot. */
[[nodiscard]] bool renderMenuPopupPresentation(
    DisplayList& display_list,
    const RenderedMenuPopupPresentationSnapshot& snapshot,
    Color background_color = Color::default_color);

/** Returns the topmost row from the already-placed snapshot; stale/malformed geometry fails closed. */
[[nodiscard]] std::optional<std::size_t> menuPopupRowAt(
    const RenderedMenuPopupPresentationSnapshot& snapshot,
    Point point) noexcept;

} // namespace sasd::ui::rendered
