#pragma once

#include <sasd/ui/menu_model.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace sasd::ui {

/**
 * Structural route from one root MenuModel to a nested submenu.
 *
 * Each element is the item index of the submenu that must be entered at that level. An empty path
 * therefore names the root menu itself. The path intentionally stores indices rather than borrowed
 * MenuModel/MenuItem pointers: popup/navigation state may survive vector reallocations, and resolving
 * the path against the current semantic tree gives callers one explicit place to detect stale state.
 *
 * MenuPath is transient interaction state, not a persistent menu identity. A structural rebuild may
 * change what an index means. Callers that keep a path while application code mutates the menu tree
 * should resolve or sanitize it before using it again.
 */
using MenuPath = std::vector<std::size_t>;

/**
 * Resolves a structural path against the current menu tree.
 *
 * The function never throws for stale navigation state. If any index is out of range, names a command
 * or separator instead of a submenu, or reaches an absent nested model, nullptr is returned. The root
 * MenuModel remains externally owned and must outlive this synchronous call; no pointer is retained.
 */
[[nodiscard]] inline const MenuModel* resolveMenuPath(const MenuModel& root,
                                                      const MenuPath& path) noexcept {
    const MenuModel* current = &root;

    for (const std::size_t item_index : path) {
        if (item_index >= current->itemCount()) {
            return nullptr;
        }

        const MenuItem& item = current->itemAt(item_index);
        const MenuModel* const nested = item.submenu();
        if (item.kind() != MenuItemKind::submenu || nested == nullptr) {
            return nullptr;
        }

        current = nested;
    }

    return current;
}

/**
 * Returns the longest still-valid prefix of a possibly stale path.
 *
 * This is intentionally a structural recovery operation. It does not guess a sibling replacement when
 * an index no longer names the same submenu; doing so could silently move an open popup into unrelated
 * application semantics. Instead the path is truncated at the first invalid level, leaving the caller
 * at the deepest parent whose route can still be proven from the current tree.
 */
[[nodiscard]] inline MenuPath sanitizeMenuPath(const MenuModel& root, const MenuPath& path) {
    MenuPath result;
    result.reserve(path.size());

    const MenuModel* current = &root;
    for (const std::size_t item_index : path) {
        if (item_index >= current->itemCount()) {
            break;
        }

        const MenuItem& item = current->itemAt(item_index);
        const MenuModel* const nested = item.submenu();
        if (item.kind() != MenuItemKind::submenu || nested == nullptr) {
            break;
        }

        result.push_back(item_index);
        current = nested;
    }

    return result;
}

/**
 * Builds the child path for entering one submenu from the currently resolved menu.
 *
 * The helper validates both the current path and the selected structural item before returning a new
 * value. It does not mutate popup state and does not retain pointers. A caller can therefore decide
 * when to commit the returned path after completing focus, repaint, or backend-specific transactions.
 */
[[nodiscard]] inline std::optional<MenuPath>
enterMenuSubmenu(const MenuModel& root,
                 const MenuPath& current_path,
                 std::size_t item_index) {
    const MenuModel* const current = resolveMenuPath(root, current_path);
    if (current == nullptr || item_index >= current->itemCount()) {
        return std::nullopt;
    }

    const MenuItem& item = current->itemAt(item_index);
    if (item.kind() != MenuItemKind::submenu || item.submenu() == nullptr) {
        return std::nullopt;
    }

    MenuPath child_path = current_path;
    child_path.push_back(item_index);
    return child_path;
}

/**
 * Returns the structural parent path, or std::nullopt when the supplied path already names the root.
 *
 * This operation is purely syntactic and therefore does not require a MenuModel. Callers should still
 * resolve/sanitize the resulting path before using it if application code may have rebuilt the menu
 * tree in the meantime.
 */
[[nodiscard]] inline std::optional<MenuPath> parentMenuPath(const MenuPath& path) {
    if (path.empty()) {
        return std::nullopt;
    }

    MenuPath parent = path;
    parent.pop_back();
    return parent;
}

} // namespace sasd::ui
