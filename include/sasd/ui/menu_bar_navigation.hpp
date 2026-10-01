#pragma once

#include <sasd/ui/menu_navigation.hpp>
#include <sasd/ui/menu_model.hpp>

#include <cstddef>
#include <optional>

namespace sasd::ui {

/**
 * Finds the previous/next top-level menu in one MenuBarModel.
 *
 * The helper deliberately reuses MenuNavigationDirection instead of introducing a second direction
 * enum whose semantics would be identical. All structurally present top-level menus participate in
 * navigation, including empty menus: MenuBarModel currently has no disabled/hidden top-level state,
 * so inventing such policy here would create semantics that the model cannot represent.
 *
 * Navigation is cyclic. A missing or stale current index enters from the natural edge: next chooses
 * the first menu and previous chooses the last. An empty bar returns std::nullopt.
 */
[[nodiscard]] inline std::optional<std::size_t>
navigateMenuBar(const MenuBarModel& bar,
                std::optional<std::size_t> current,
                MenuNavigationDirection direction) noexcept {
    const std::size_t count = bar.menuCount();
    if (count == 0U) {
        return std::nullopt;
    }

    const bool current_is_valid = current.has_value() && *current < count;
    if (!current_is_valid) {
        return direction == MenuNavigationDirection::next ? std::optional<std::size_t>{0U}
                                                         : std::optional<std::size_t>{count - 1U};
    }

    if (direction == MenuNavigationDirection::next) {
        return (*current + 1U) % count;
    }

    return *current == 0U ? count - 1U : *current - 1U;
}

/** Semantic result of interpreting one key while a menu bar itself is active. */
enum class MenuBarKeyAction {
    none,
    select,
    open_menu,
    close_menu_bar,
};

/** Stateless interpretation result for one menu-bar key event. */
struct MenuBarKeyResult {
    MenuBarKeyAction action{MenuBarKeyAction::none};
    std::optional<std::size_t> selection{};
};

/**
 * Interprets conventional unmodified keyboard input for the top-level menu bar.
 *
 * This function intentionally returns intent instead of opening a popup or retaining selection. A
 * presenter/controller can first update its own focus/popup transaction and then materialize the menu
 * surface. That mirrors interpretMenuPopupKey() and keeps semantic keyboard policy independent from
 * Terminal, Rendered, or future native menu presentation.
 *
 * The initial contract is deliberately small:
 * - Left/Right select the previous/next top-level menu cyclically;
 * - Home/End select the first/last top-level menu;
 * - Down/Enter request opening the currently selected top-level menu;
 * - Escape requests leaving menu-bar interaction;
 * - releases, modified gestures, and unrelated keys remain unhandled.
 *
 * Up is intentionally not treated as "open and select the last popup item" yet. That behavior spans
 * two independently owned interaction states (menu-bar selection and popup selection) and belongs in
 * the later controller that composes both contracts rather than being guessed in this stateless layer.
 */
[[nodiscard]] inline MenuBarKeyResult
interpretMenuBarKey(const MenuBarModel& bar,
                    std::optional<std::size_t> current,
                    const KeyEvent& event) noexcept {
    const std::size_t count = bar.menuCount();
    const bool current_is_valid = current.has_value() && *current < count;
    const std::optional<std::size_t> valid_current = current_is_valid ? current : std::nullopt;

    if (!event.pressed || event.modifiers != KeyModifier::none) {
        return {MenuBarKeyAction::none, valid_current};
    }

    switch (event.key) {
    case Key::left: {
        const auto target = navigateMenuBar(bar, current, MenuNavigationDirection::previous);
        return target.has_value() ? MenuBarKeyResult{MenuBarKeyAction::select, target}
                                  : MenuBarKeyResult{MenuBarKeyAction::none, std::nullopt};
    }
    case Key::right: {
        const auto target = navigateMenuBar(bar, current, MenuNavigationDirection::next);
        return target.has_value() ? MenuBarKeyResult{MenuBarKeyAction::select, target}
                                  : MenuBarKeyResult{MenuBarKeyAction::none, std::nullopt};
    }
    case Key::home:
        return count > 0U ? MenuBarKeyResult{MenuBarKeyAction::select, std::size_t{0U}}
                          : MenuBarKeyResult{};
    case Key::end:
        return count > 0U ? MenuBarKeyResult{MenuBarKeyAction::select, count - 1U}
                          : MenuBarKeyResult{};
    case Key::down:
    case Key::enter:
        return valid_current.has_value()
                   ? MenuBarKeyResult{MenuBarKeyAction::open_menu, valid_current}
                   : MenuBarKeyResult{MenuBarKeyAction::none, std::nullopt};
    case Key::escape:
        return {MenuBarKeyAction::close_menu_bar, valid_current};
    default:
        return {MenuBarKeyAction::none, valid_current};
    }
}

} // namespace sasd::ui
