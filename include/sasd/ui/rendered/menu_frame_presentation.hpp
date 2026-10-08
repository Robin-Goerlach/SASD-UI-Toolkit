#pragma once

#include <sasd/ui/menu_interaction_view.hpp>
#include <sasd/ui/rendered/menu_bar_presentation.hpp>
#include <sasd/ui/rendered/menu_presentation.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace sasd::ui::rendered {

/** One popup layer in final paint order. Later layers are visually on top. */
struct RenderedPositionedMenuPopup {
    RenderedMenuPopupPresentationSnapshot snapshot{};
    bool placed_left_of_parent{false};
};

/**
 * Complete owned Rendered menu surface.
 *
 * The builder observes Core only while constructing this value. No MenuModel, Command, measurement
 * context or backend object crosses the boundary. Rendering and hit testing consume this exact frame,
 * so nested placement is never silently recomputed for a second consumer.
 */
struct RenderedMenuFramePresentationSnapshot {
    Rect viewport{};
    RenderedMenuBarPresentationSnapshot menu_bar{};
    std::vector<RenderedPositionedMenuPopup> popups{};
};

[[nodiscard]] std::optional<RenderedMenuFramePresentationSnapshot>
buildMenuFramePresentation(const MenuBarModel& bar,
                           const MenuInteractionController& controller,
                           Point menu_bar_origin,
                           Rect viewport,
                           const RenderedMeasurementContext& metrics);

/** Composes base content and the complete menu frame transactionally. */
[[nodiscard]] bool renderMenuFramePresentation(
    DisplayList& display_list,
    const RenderedMenuFramePresentationSnapshot& frame,
    Color background_color = Color::default_color);

struct RenderedMenuHit {
    enum class Kind {
        menu_bar,
        popup_row,
    };

    Kind kind{Kind::menu_bar};
    std::size_t level{0};
    std::size_t index{0};

    friend constexpr bool operator==(const RenderedMenuHit&, const RenderedMenuHit&) = default;
};

/** Hit-tests only the final frame; no semantic model or measurement context is needed. */
[[nodiscard]] std::optional<RenderedMenuHit>
menuFrameHitAt(const RenderedMenuFramePresentationSnapshot& frame,
               Point point) noexcept;

} // namespace sasd::ui::rendered
