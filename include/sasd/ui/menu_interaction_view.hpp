#pragma once

#include <sasd/ui/menu_interaction_controller.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

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

/**
 * Owned presentation value for one semantic menu item.
 *
 * Unlike MenuItem itself, this type deliberately copies user-facing text and shortcut metadata. It is a
 * frame/presentation snapshot, not a second semantic model: changing a Command or rebuilding a menu does
 * not mutate an already-created snapshot. This lets a backend finish one render transaction without
 * retaining string_views, Command references, or MenuModel/MenuItem pointers into a concurrently changed
 * application model.
 */
struct MenuItemPresentationSnapshot {
    MenuItemKind kind{MenuItemKind::separator};
    std::string text{};
    bool enabled{false};
    std::optional<Shortcut> shortcut{};
};

/**
 * Owned value snapshot for the complete top-level menu bar.
 *
 * titles is always copied from the current MenuBarModel so an inactive controller can still render the
 * persistent menu bar. selection is populated only when the controller's active top-level index is
 * currently valid. popup_open is true only for such a valid selection; stale controller state therefore
 * cannot cause a presenter to draw an apparently open popup for a menu that no longer exists.
 */
struct MenuBarPresentationSnapshot {
    std::vector<std::string> titles{};
    std::optional<std::size_t> selection{};
    bool popup_open{false};
};

/** Owned value snapshot for one currently resolvable popup level. */
struct MenuPopupPresentationSnapshot {
    std::vector<MenuItemPresentationSnapshot> items{};
    std::optional<std::size_t> selection{};
};

/**
 * Copies the current top-level menu-bar presentation state into self-contained values.
 *
 * This is intentionally a per-frame boundary rather than cached semantic state. Copying titles costs
 * small allocations, but it removes model lifetime from the backend transaction and keeps the first
 * presentation contract straightforward. Later profiling may introduce storage reuse without changing
 * the semantic rule that backends consume owned frame data rather than retain MenuModel pointers.
 */
[[nodiscard]] inline MenuBarPresentationSnapshot
snapshotMenuBarPresentation(const MenuBarModel& bar,
                            const MenuInteractionController& controller) {
    MenuBarPresentationSnapshot snapshot;
    snapshot.titles.reserve(bar.menuCount());
    for (std::size_t index = 0; index < bar.menuCount(); ++index) {
        snapshot.titles.emplace_back(bar.menuAt(index).title());
    }

    const auto active = menuBarInteractionView(bar, controller);
    if (active.has_value()) {
        snapshot.selection = active->selection;
        snapshot.popup_open = active->popup_open;
    }

    return snapshot;
}

/**
 * Copies one currently open popup level into self-contained presentation values.
 *
 * The helper first uses menuPopupLevelView() to apply the same fail-closed path/selection validation as
 * the borrowed view API. Only after that validation succeeds are item properties copied. Command text and
 * enabled state are therefore sampled exactly once for this snapshot. Subsequent Command destruction,
 * state changes, or structural menu mutation cannot invalidate the returned value.
 *
 * std::nullopt means the requested popup level cannot currently be proven to exist. The function never
 * repairs MenuInteractionController; observation remains side-effect free.
 */
[[nodiscard]] inline std::optional<MenuPopupPresentationSnapshot>
snapshotMenuPopupPresentation(const MenuBarModel& bar,
                              const MenuInteractionController& controller,
                              std::size_t level) {
    const auto view = menuPopupLevelView(bar, controller, level);
    if (!view.has_value() || view->menu == nullptr) {
        return std::nullopt;
    }

    MenuPopupPresentationSnapshot snapshot;
    snapshot.selection = view->selection;
    snapshot.items.reserve(view->menu->itemCount());

    for (std::size_t index = 0; index < view->menu->itemCount(); ++index) {
        const MenuItem& item = view->menu->itemAt(index);
        snapshot.items.push_back(MenuItemPresentationSnapshot{
            item.kind(),
            std::string{item.text()},
            item.isEnabled(),
            item.shortcut(),
        });
    }

    return snapshot;
}

} // namespace sasd::ui
