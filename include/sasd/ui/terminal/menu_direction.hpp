#pragma once

#include <cstddef>

namespace sasd::ui::terminal {

/**
 * Horizontal side on which a child submenu is presented relative to its parent popup.
 *
 * This value is terminal presentation state, not semantic menu state. Keeping the side in a tiny header
 * breaks the dependency cycle between viewport placement, directional cell rendering, and frame
 * composition: all three layers can share the same vocabulary without depending on each other's
 * implementation headers.
 */
enum class SubmenuPopupSide {
    right,
    left,
};

/**
 * Directional chrome for the one submenu row that currently owns an open child popup.
 *
 * One popup level can have at most one direct child open at a time. Keeping only that active row and the
 * already-decided SubmenuPopupSide avoids copying placement state into every semantic menu item while still
 * giving presentation code enough information to draw a direction-sensitive indicator.
 */
struct ActiveSubmenuPresentationDirection {
    std::size_t item_index{0};
    SubmenuPopupSide side{SubmenuPopupSide::right};
};

} // namespace sasd::ui::terminal
