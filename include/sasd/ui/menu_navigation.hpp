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

} // namespace sasd::ui
