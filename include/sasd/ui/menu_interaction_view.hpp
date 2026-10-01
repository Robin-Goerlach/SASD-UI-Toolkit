#pragma once

#include <sasd/ui/menu_interaction_controller.hpp>

#include <cstddef>
#include <optional>

namespace sasd::ui {

/**
 * Read-only description of the currently selected top-level menu.
 *
 * menu is a borrowed pointer into the caller-owned MenuBarModel. It is valid only for immediate,
 * synchronous presentation work and must not be retained across structural mutation. selection is the
 * current top-level index. popup_open reports whether the controller currently represents an open popup
 * below this menu; concrete popup levels are still queried separately through menuPopupLevelView().
 */
struct MenuBarInteractionView {
    const MenuModel* menu{nullptr};
    std::size_t selection{0};
    bool popup_open{false};
};

/**
 * Resolves the controller's current top-level selection for immediate presentation.
 *
 * The helper is intentionally observational and side-effect free. If menu interaction is inactive, the
 * controller has no selection, or the retained index is outside the current MenuBarModel, std::nullopt
 * is returned rather than mutating the controller or guessing another top-level menu. This lets a
 * presenter fail closed when application code clears or shortens the menu bar between input dispatch
 * and frame construction.
 *
 * The returned pointer is re-resolved from the current MenuBarModel on every call. No pointer is cached
 * in MenuInteractionController and none should be cached by the presenter. As with MenuPath, the index
 * is transient interaction state rather than a persistent semantic identity: rebuilding the bar with a
 * different menu at the same still-valid index cannot be distinguished without introducing explicit
 * stable menu identities, which this view deliberately does not invent.
 */
[[nodiscard]] inline std::optional<MenuBarInteractionView>
menuBarInteractionView(const MenuBarModel& bar,
                       const MenuInteractionController& controller) noexcept {
    if (!controller.isActive()) {
        return std::nullopt;
    }

    const auto selection = controller.menuBarSelection();
    if (!selection.has_value() || *selection >= bar.menuCount()) {
        return std::nullopt;
    }

    return MenuBarInteractionView{&bar.menuAt(*selection), *selection, controller.popupOpen()};
}

/**
 * Read-only description of one currently open popup level.
 *
 * menu is a borrowed pointer into the caller-owned MenuBarModel. It is intentionally returned only as
 * an ephemeral presentation view: callers must not retain it across structural menu mutation. selection
 * is the currently meaningful item index for that level, or std::nullopt when no item can be proven
 * selected against the current semantic model.
 */
struct MenuPopupLevelView {
    const MenuModel* menu{nullptr};
    std::optional<std::size_t> selection{};
};

/**
 * Resolves one open popup level for immediate presentation without mutating interaction state.
 *
 * level zero names the selected top-level menu's root popup. Higher levels follow the submenu indices
 * stored in MenuInteractionController::popupPath(). The helper deliberately re-resolves those indices
 * against the supplied MenuBarModel on every call instead of caching MenuModel/MenuItem pointers.
 * Consequently a presenter can build a frame after application-side menu mutation without inheriting a
 * dangling pointer from an older interaction transaction.
 *
 * If the requested level is not open, the selected top-level menu has disappeared, or any structural
 * path element no longer names a live submenu, std::nullopt is returned. The controller is not repaired
 * here: normalization remains the responsibility of its next input transaction. This keeps observation
 * side-effect free while still making stale state safe to inspect.
 *
 * Parent-level selection is derived from the path element that opened the next submenu. The deepest
 * level uses MenuInteractionController::popupSelection(). Before returning, the candidate selection is
 * checked against the current MenuModel and cleared if it is out of range, disabled, expired, or (for a
 * parent level) no longer a submenu. This avoids presenting stale highlight state after semantic model
 * changes while preserving the controller's conservative, explicit normalization policy.
 */
[[nodiscard]] inline std::optional<MenuPopupLevelView>
menuPopupLevelView(const MenuBarModel& bar,
                   const MenuInteractionController& controller,
                   std::size_t level) noexcept {
    if (!controller.popupOpen() || level >= controller.popupDepth()) {
        return std::nullopt;
    }

    const auto top_level = controller.menuBarSelection();
    const auto& deepest_path = controller.popupPath();
    if (!top_level.has_value() || !deepest_path.has_value() || *top_level >= bar.menuCount()) {
        return std::nullopt;
    }

    const MenuPath& path = *deepest_path;
    if (level > path.size()) {
        return std::nullopt;
    }

    const MenuModel* current = &bar.menuAt(*top_level);

    /*
     * Resolve only the prefix needed for this level. All bounds/type checks happen before itemAt() or
     * submenu traversal, so stale value-state turns into std::nullopt instead of an exception or an
     * invalid borrowed pointer.
     */
    for (std::size_t depth = 0; depth < level; ++depth) {
        if (depth >= path.size()) {
            return std::nullopt;
        }

        const std::size_t item_index = path[depth];
        if (item_index >= current->itemCount()) {
            return std::nullopt;
        }

        const MenuItem& item = current->itemAt(item_index);
        const MenuModel* const nested = item.submenu();
        if (item.kind() != MenuItemKind::submenu || nested == nullptr) {
            return std::nullopt;
        }

        current = nested;
    }

    std::optional<std::size_t> selection;
    const bool parent_level = level < path.size();
    if (parent_level) {
        selection = path[level];
    } else {
        selection = controller.popupSelection();
    }

    if (selection.has_value()) {
        if (*selection >= current->itemCount()) {
            selection.reset();
        } else {
            const MenuItem& item = current->itemAt(*selection);
            const bool semantically_selectable = item.isEnabled();
            const bool still_opens_child = !parent_level ||
                                           (item.kind() == MenuItemKind::submenu &&
                                            item.submenu() != nullptr);
            if (!semantically_selectable || !still_opens_child) {
                selection.reset();
            }
        }
    }

    return MenuPopupLevelView{current, selection};
}

} // namespace sasd::ui
