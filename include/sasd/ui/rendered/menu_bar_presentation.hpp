#pragma once

#include <sasd/ui/menu_interaction_view.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui::rendered {

/** One owned, measured and placed top-level menu title. */
struct RenderedMenuBarItem {
    std::string title{};
    Size measured_size{};
    Rect bounds{};
};

/**
 * Final logical geometry for the persistent rendered menu bar.
 *
 * The semantic MenuBarPresentationSnapshot remains free of graphical coordinates. This value is the
 * explicit boundary where title measurement and viewport fitting become owned presentation state; both
 * rendering and hit testing consume these same rectangles and never re-measure the semantic model.
 */
struct RenderedMenuBarPresentationSnapshot {
    Rect bounds{};
    std::vector<RenderedMenuBarItem> items{};
    std::optional<std::size_t> selection{};
    bool popup_open{false};
    Coordinate horizontal_padding{0};
    Coordinate row_height{0};
};

[[nodiscard]] std::optional<RenderedMenuBarPresentationSnapshot>
buildMenuBarPresentation(const MenuBarPresentationSnapshot& menu,
                         Point origin,
                         Rect viewport,
                         const RenderedMeasurementContext& metrics);

/** Paints the exact title rectangles stored in a validated snapshot. */
[[nodiscard]] bool renderMenuBarPresentation(
    DisplayList& display_list,
    const RenderedMenuBarPresentationSnapshot& snapshot,
    Color background_color = Color::default_color);

/** Returns the topmost title at point using half-open snapshot geometry. */
[[nodiscard]] std::optional<std::size_t> menuBarItemAt(
    const RenderedMenuBarPresentationSnapshot& snapshot,
    Point point) noexcept;

} // namespace sasd::ui::rendered
