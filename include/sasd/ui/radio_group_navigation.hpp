#pragma once

#include <sasd/ui/events/event.hpp>

#include <cstdint>

namespace sasd::ui {

class FocusManager;
class RadioButton;

/** Logical direction through one RadioGroup's stable membership order. */
enum class RadioGroupNavigationDirection : std::uint8_t {
    previous,
    next,
};

/**
 * Explicit keyboard navigation policy for RadioButton groups.
 *
 * RadioButton owns selection semantics; FocusManager owns focus transitions; RadioGroup owns
 * exclusivity. This utility composes those three responsibilities without making RadioButton store a
 * FocusManager pointer or teaching FocusTraversal about radio-specific arrow-key behavior.
 *
 * Applications normally call handleEvent() from their scope-level fallback after ordinary widget
 * dispatch has ignored an arrow key, alongside FocusTraversal::handleEvent() for Tab. Keeping radio
 * navigation explicit avoids silently crossing an application's chosen focus-scope boundary.
 */
class RadioGroupNavigation final {
public:
    RadioGroupNavigation() = delete;

    /**
     * Moves focus and selection from source to the next eligible group member.
     *
     * Membership order is RadioGroup attachment order and wraps at both ends. Candidates are skipped
     * when they are locally non-focusable/hidden/disabled, have a hidden/disabled visual ancestor, or
     * belong to a different top-level visual root than source. The latter rule lets semantic groups
     * survive reparenting while preventing one arrow key from jumping focus into another window.
     *
     * Focus is requested before selection changes. If focus callbacks redirect/clear the transition,
     * selection is left unchanged. Once focus succeeds, target selection is committed and any
     * onSelected callback may safely release/destroy the target because this function touches no
     * target state afterwards.
     *
     * @returns true when a different eligible member received focus. The target may already have been
     * selected, in which case focus still moves but no duplicate selection callback is emitted.
     */
    [[nodiscard]] static bool move(FocusManager& focus,
                                   RadioButton& source,
                                   RadioGroupNavigationDirection direction);

    /**
     * Handles unmodified Left/Up as previous and Right/Down as next.
     *
     * Press performs the move. A matching release is consumed only when this source currently has a
     * different eligible group member in that direction, preventing desktop key-up from bubbling
     * after a handled key-down. Modified arrows and standalone RadioButtons remain unhandled.
     */
    [[nodiscard]] static EventResult handleEvent(FocusManager& focus,
                                                 RadioButton& source,
                                                 const Event& event);

private:
    [[nodiscard]] static RadioButton* targetFor(
        RadioButton& source,
        RadioGroupNavigationDirection direction) noexcept;
};

} // namespace sasd::ui
