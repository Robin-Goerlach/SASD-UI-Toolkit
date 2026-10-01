#pragma once

#include <sasd/ui/menu_model.hpp>

#include <cstddef>
#include <optional>

namespace sasd::ui {

/** Direction for cyclic navigation inside one semantic MenuModel. */
enum class MenuNavigationDirection {
    previous,
    next,
};

/**
 * Finds the next selectable item in one menu without retaining presentation state.
 *
 * The helper is deliberately stateless. Callers own the currently selected index and can therefore
 * keep popup lifetime, focus policy, pointer hover, and backend-specific open/close behavior outside
 * the semantic model. This mirrors RadioGroupNavigation: the toolkit supplies deterministic semantic
 * movement while the surrounding presenter decides when navigation is active.
 *
 * Selectability is defined by MenuItem::isEnabled(). Separators, expired Commands, and disabled
 * Commands are skipped; submenu items are selectable because entering the submenu is a valid semantic
 * navigation target even though MenuItem::activate() does not execute it.
 *
 * Navigation is cyclic. When current is std::nullopt, next starts at the first selectable item and
 * previous starts at the last. A stale/out-of-range current index is treated exactly like no current
 * selection instead of throwing: structural menu mutation can invalidate an old selection index, and
 * recovery at the next navigation gesture is safer than turning ordinary UI mutation into an error.
 *
 * If current names the only selectable item, a complete wrap returns that same index. If no selectable
 * item exists, std::nullopt is returned.
 */
[[nodiscard]] inline std::optional<std::size_t>
navigateMenu(const MenuModel& menu,
             std::optional<std::size_t> current,
             MenuNavigationDirection direction) {
    const std::size_t count = menu.itemCount();
    if (count == 0) {
        return std::nullopt;
    }

    const bool current_is_valid = current.has_value() && *current < count;

    /*
     * The loop advances before testing. For an existing selection that means "move away from the
     * current item"; for no/stale selection the synthetic starting edge makes the first advance land
     * on the natural beginning (next) or end (previous) of the menu.
     */
    std::size_t index = current_is_valid
                            ? *current
                            : (direction == MenuNavigationDirection::next ? count - 1U : 0U);

    for (std::size_t visited = 0; visited < count; ++visited) {
        if (direction == MenuNavigationDirection::next) {
            index = (index + 1U) % count;
        } else {
            index = index == 0U ? count - 1U : index - 1U;
        }

        if (menu.itemAt(index).isEnabled()) {
            return index;
        }
    }

    return std::nullopt;
}

/**
 * Semantic result of interpreting one key while a vertical popup-style menu is active.
 *
 * The result deliberately describes intent rather than performing presentation work. In particular,
 * activate_command does not execute client code and open_submenu does not create a popup. The caller
 * owns those effects after it has applied whatever focus, dismissal, repaint, or backend policy is
 * appropriate for the active menu surface.
 */
enum class MenuPopupKeyAction {
    none,
    select,
    activate_command,
    open_submenu,
    close_menu,
};

/** Stateless interpretation result for one popup-menu key event. */
struct MenuPopupKeyResult {
    MenuPopupKeyAction action{MenuPopupKeyAction::none};
    std::optional<std::size_t> selection{};
};

/**
 * Interprets conventional unmodified keyboard input for one vertical popup menu.
 *
 * This helper intentionally stops short of becoming a menu controller. It does not retain selection,
 * own popup lifetime, execute Commands, mutate focus, or recursively enter submenus. Instead it maps a
 * KeyEvent plus the caller-owned current selection to a small semantic instruction that Terminal,
 * Rendered, and later native presenters can consume consistently.
 *
 * Only key-press events with no modifiers participate. This is important because modified gestures
 * remain available to ShortcutMap and higher-level routing; a menu helper must not silently swallow a
 * Ctrl/Alt/Meta combination merely because its base key is an arrow or Enter.
 *
 * Supported vertical-popup conventions are deliberately narrow:
 * - Up/Down move cyclically through selectable items using navigateMenu();
 * - Home/End select the first/last selectable item without depending on current selection;
 * - Enter requests Command activation or submenu opening for the current selectable item;
 * - Right requests opening only when the current item is a submenu;
 * - Left and Escape request closing the current menu level;
 * - all other input is left unhandled.
 *
 * A stale current index is sanitized to no selection for non-navigation actions. Arrow/Home/End input
 * can recover from stale state through navigateMenu(), matching the mutation-tolerant contract of the
 * lower-level helper.
 */
[[nodiscard]] inline MenuPopupKeyResult
interpretMenuPopupKey(const MenuModel& menu,
                      std::optional<std::size_t> current,
                      const KeyEvent& event) {
    const bool current_is_valid = current.has_value() && *current < menu.itemCount();
    const std::optional<std::size_t> valid_current = current_is_valid ? current : std::nullopt;

    if (!event.pressed || event.modifiers != KeyModifier::none) {
        return {MenuPopupKeyAction::none, valid_current};
    }

    switch (event.key) {
    case Key::up: {
        const auto target = navigateMenu(menu, current, MenuNavigationDirection::previous);
        return target.has_value()
                   ? MenuPopupKeyResult{MenuPopupKeyAction::select, target}
                   : MenuPopupKeyResult{MenuPopupKeyAction::none, std::nullopt};
    }
    case Key::down: {
        const auto target = navigateMenu(menu, current, MenuNavigationDirection::next);
        return target.has_value()
                   ? MenuPopupKeyResult{MenuPopupKeyAction::select, target}
                   : MenuPopupKeyResult{MenuPopupKeyAction::none, std::nullopt};
    }
    case Key::home: {
        const auto target = navigateMenu(menu, std::nullopt, MenuNavigationDirection::next);
        return target.has_value()
                   ? MenuPopupKeyResult{MenuPopupKeyAction::select, target}
                   : MenuPopupKeyResult{MenuPopupKeyAction::none, std::nullopt};
    }
    case Key::end: {
        const auto target = navigateMenu(menu, std::nullopt, MenuNavigationDirection::previous);
        return target.has_value()
                   ? MenuPopupKeyResult{MenuPopupKeyAction::select, target}
                   : MenuPopupKeyResult{MenuPopupKeyAction::none, std::nullopt};
    }
    case Key::escape:
    case Key::left:
        return {MenuPopupKeyAction::close_menu, valid_current};
    case Key::enter:
    case Key::right:
        break;
    default:
        return {MenuPopupKeyAction::none, valid_current};
    }

    if (!valid_current.has_value()) {
        return {MenuPopupKeyAction::none, std::nullopt};
    }

    const MenuItem& item = menu.itemAt(*valid_current);
    if (!item.isEnabled()) {
        return {MenuPopupKeyAction::none, valid_current};
    }

    if (item.kind() == MenuItemKind::submenu) {
        return {MenuPopupKeyAction::open_submenu, valid_current};
    }

    /*
     * Right is meaningful only for entering a submenu. Enter, on the other hand, requests execution of
     * an enabled Command item. We return intent rather than calling activate() here so client callbacks
     * cannot destroy menu state while a presenter is still processing its key-routing transaction.
     */
    if (event.key == Key::enter && item.kind() == MenuItemKind::command) {
        return {MenuPopupKeyAction::activate_command, valid_current};
    }

    return {MenuPopupKeyAction::none, valid_current};
}

} // namespace sasd::ui
