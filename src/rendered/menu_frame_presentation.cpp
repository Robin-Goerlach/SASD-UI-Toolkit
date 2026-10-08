#include <sasd/ui/rendered/menu_frame_presentation.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>

namespace sasd::ui::rendered {
namespace {
using Wide = std::int64_t;

[[nodiscard]] std::optional<Coordinate> add(Coordinate left, Coordinate right) noexcept {
    const Wide value = static_cast<Wide>(left) + static_cast<Wide>(right);
    if (value < std::numeric_limits<Coordinate>::min() ||
        value > std::numeric_limits<Coordinate>::max()) {
        return std::nullopt;
    }
    return static_cast<Coordinate>(value);
}

[[nodiscard]] bool insideViewport(const Rect& bounds, const Rect& viewport) noexcept {
    if (bounds.isEmpty() || viewport.isEmpty()) return false;
    return viewport.contains({bounds.x, bounds.y}) &&
           viewport.contains({
               static_cast<Coordinate>(static_cast<Wide>(bounds.x) + bounds.width - 1),
               static_cast<Coordinate>(static_cast<Wide>(bounds.y) + bounds.height - 1)});
}

void translate(RenderedMenuPopupPresentationSnapshot& snapshot,
               Coordinate delta_x) {
    auto move_rect = [delta_x](Rect& rect) {
        rect.x = static_cast<Coordinate>(static_cast<Wide>(rect.x) + delta_x);
    };
    move_rect(snapshot.bounds);
    move_rect(snapshot.content_bounds);
    for (auto& row : snapshot.rows) {
        move_rect(row.bounds);
        move_rect(row.label_bounds);
        move_rect(row.shortcut_bounds);
        move_rect(row.submenu_indicator_bounds);
    }
}

[[nodiscard]] std::optional<RenderedMenuPopupPresentationSnapshot>
buildPopupAt(const MenuPopupPresentationSnapshot& semantic,
             Point origin,
             Rect available_viewport,
             const RenderedMeasurementContext& metrics) {
    return buildMenuPopupPresentation(semantic, origin, available_viewport, metrics);
}
} // namespace

std::optional<RenderedMenuFramePresentationSnapshot> buildMenuFramePresentation(
    const MenuBarModel& bar,
    const MenuInteractionController& controller,
    Point menu_bar_origin,
    Rect viewport,
    const RenderedMeasurementContext& metrics) {
    if (viewport.width < 0 || viewport.height < 0 || viewport.isEmpty()) return std::nullopt;

    const MenuBarPresentationSnapshot semantic_bar = snapshotMenuBarPresentation(bar, controller);
    const auto menu_bar = buildMenuBarPresentation(semantic_bar, menu_bar_origin, viewport, metrics);
    if (!menu_bar.has_value()) return std::nullopt;

    RenderedMenuFramePresentationSnapshot frame;
    frame.viewport = viewport;
    frame.menu_bar = *menu_bar;
    if (!controller.popupOpen()) return frame;

    if (!semantic_bar.selection.has_value() ||
        *semantic_bar.selection >= frame.menu_bar.items.size() || controller.popupDepth() == 0U) {
        return std::nullopt;
    }

    const auto root_semantic = snapshotMenuPopupPresentation(bar, controller, 0U);
    if (!root_semantic.has_value()) return std::nullopt;
    const auto& selected_title = frame.menu_bar.items[*semantic_bar.selection].bounds;
    const auto root_y = add(selected_title.y, selected_title.height);
    if (!root_y.has_value()) return std::nullopt;
    const auto root = buildPopupAt(*root_semantic,
                                   {selected_title.x, *root_y}, viewport, metrics);
    if (!root.has_value()) return std::nullopt;
    frame.popups.push_back(RenderedPositionedMenuPopup{*root, false});

    for (std::size_t level = 1; level < controller.popupDepth(); ++level) {
        const auto child_semantic = snapshotMenuPopupPresentation(bar, controller, level);
        if (!child_semantic.has_value()) return std::nullopt;

        auto& parent = frame.popups.back().snapshot;
        if (!parent.selection.has_value() || *parent.selection >= parent.rows.size()) {
            return std::nullopt;
        }
        const Rect parent_row = parent.rows[*parent.selection].bounds;
        const auto right_x = add(parent_row.x, parent_row.width);
        if (!right_x.has_value()) return std::nullopt;

        // Build once at the preferred right-hand origin. The broad logical viewport only supplies enough
        // room to learn the child's natural owned width; the final viewport check happens after placement.
        const Rect broad_right{*right_x, viewport.y,
                               std::numeric_limits<Coordinate>::max(), viewport.height};
        auto child = buildPopupAt(*child_semantic, {*right_x, parent_row.y}, broad_right, metrics);
        bool placed_left = false;
        if (!child.has_value()) {
            // The same natural popup is measured at the parent edge and then translated left exactly once.
            const Rect broad_left{parent_row.x, viewport.y,
                                  std::numeric_limits<Coordinate>::max(), viewport.height};
            child = buildPopupAt(*child_semantic, {parent_row.x, parent_row.y}, broad_left, metrics);
            if (!child.has_value()) return std::nullopt;
            const auto left_x = add(parent_row.x, -child->bounds.width);
            if (!left_x.has_value()) return std::nullopt;
            translate(*child, *left_x - child->bounds.x);
            placed_left = true;
        }

        if (!insideViewport(child->bounds, viewport)) return std::nullopt;
        frame.popups.push_back(RenderedPositionedMenuPopup{std::move(*child), placed_left});
    }
    return frame;
}

bool renderMenuFramePresentation(DisplayList& display_list,
                                 const RenderedMenuFramePresentationSnapshot& frame,
                                 Color background_color) {
    if (!insideViewport(frame.menu_bar.bounds, frame.viewport) && !frame.menu_bar.items.empty()) {
        return false;
    }
    DisplayList candidate = display_list;
    if (!renderMenuBarPresentation(candidate, frame.menu_bar, background_color)) return false;
    for (const auto& popup : frame.popups) {
        if (!insideViewport(popup.snapshot.bounds, frame.viewport) ||
            !renderMenuPopupPresentation(candidate, popup.snapshot, background_color)) {
            return false;
        }
    }
    display_list = std::move(candidate);
    return true;
}

std::optional<RenderedMenuHit> menuFrameHitAt(
    const RenderedMenuFramePresentationSnapshot& frame,
    Point point) noexcept {
    for (std::size_t level = frame.popups.size(); level-- > 0;) {
        const auto& popup = frame.popups[level].snapshot;
        if (const auto row = menuPopupRowAt(popup, point); row.has_value()) {
            return RenderedMenuHit{RenderedMenuHit::Kind::popup_row, level, *row};
        }
    }
    if (const auto item = menuBarItemAt(frame.menu_bar, point); item.has_value()) {
        return RenderedMenuHit{RenderedMenuHit::Kind::menu_bar, 0, *item};
    }
    return std::nullopt;
}
} // namespace sasd::ui::rendered
