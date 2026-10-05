#pragma once

#include <sasd/ui/events/event.hpp>
#include <sasd/ui/menu_interaction_controller.hpp>
#include <sasd/ui/terminal/menu_hit_test.hpp>

#include <optional>

namespace sasd::ui::terminal {

/**
 * Translates terminal menu pointer geometry into the existing semantic menu interaction state.
 *
 * Geometry and semantics deliberately remain separate. TerminalMenuHitTest consumes the already-built
 * MenuFramePresentationSnapshot and answers which painted title or popup row owns a terminal cell. This
 * class then applies only the small semantic transitions that are defined for the current pointer slice.
 * It does not re-measure menu text, calculate popup placement, render cells, own focus, or route ordinary
 * Widget pointers underneath the transient menu surface.
 *
 * The return type uses std::optional<MenuInteractionResult> as an explicit consumption contract:
 *
 * - std::nullopt means the menu layer did not consume the event and the host may continue normal routing;
 * - an engaged result means the event belongs to the menu interaction scope, even when action == none;
 * - state_changed/closed keep the existing MenuInteractionAction vocabulary so hosts can decide whether a
 *   fresh menu frame must be presented without learning a terminal-specific second state machine.
 *
 * This first slice intentionally supports only primary-press opening/switching of top-level menus plus
 * outside-click dismissal. Popup-row presses are consumed but do not yet select, open submenus, or activate
 * commands. While the controller is active, other pointer transitions are also consumed so motion/release
 * events cannot leak through the visible popup overlay into application Widgets underneath it.
 */
class TerminalMenuPointerInteraction final {
public:
    TerminalMenuPointerInteraction() = delete;

    /**
     * Applies one PointerEvent against a menu frame representing the controller state visible to the user.
     *
     * A primary press on a top-level title is defined as the pointer equivalent of selecting that title and
     * opening its root popup without preselecting a popup row. The implementation intentionally composes the
     * controller's existing public begin()+Enter transition rather than reaching into controller internals;
     * keyboard and pointer entry therefore share the same root-popup semantics.
     *
     * Popup rows have visual precedence over the menu bar because MenuFramePresentationSnapshot paints them
     * after the bar. When an active popup overlaps the bar in a synthetic or future placement scenario, a
     * row hit is therefore consumed before title hit testing is considered.
     *
     * A primary press outside all currently represented menu surfaces closes an active controller and is
     * still consumed. This is the critical no-click-through rule: dismissing a transient menu must not also
     * press the Button/TextField that happened to be underneath the same terminal cell.
     */
    [[nodiscard]] static std::optional<MenuInteractionResult>
    handle(const MenuBarModel& bar,
           MenuInteractionController& controller,
           const MenuFramePresentationSnapshot& frame,
           const PointerEvent& event,
           AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        const bool active_before = controller.isActive();
        const bool primary_press =
            event.action == PointerAction::press && event.button == PointerButton::primary;

        /*
         * Active menus form a transient modal pointer scope. Until richer hover/release semantics are added,
         * every non-primary-press event is consumed without changing semantic state. This prevents a release
         * following a menu-title press, or incidental motion over an open popup, from reaching Widgets below
         * the overlay merely because this first interaction slice does not interpret that transition yet.
         */
        if (!primary_press) {
            return active_before
                       ? std::optional<MenuInteractionResult>{MenuInteractionResult{}}
                       : std::nullopt;
        }

        /*
         * Popup layers are painted after the menu bar, so they own overlapping cells while the interaction
         * is active. Popup-item behavior is deliberately deferred, but consuming the press now establishes
         * the correct modal boundary and prevents accidental title switching or Widget click-through.
         */
        if (active_before &&
            TerminalMenuHitTest::popupItemAt(frame, event.position, ambiguous_width).has_value()) {
            return MenuInteractionResult{};
        }

        if (const auto title =
                TerminalMenuHitTest::menuBarIndexAt(frame, event.position, ambiguous_width);
            title.has_value()) {
            /*
             * A frame is normally built immediately from the same MenuBarModel, but structural application
             * mutation can theoretically occur between frame construction and input handling. Never pass a
             * stale presentation index to begin(): begin() intentionally falls back to index zero for a stale
             * preferred index, which would turn an unprovable pointer hit into the wrong semantic menu.
             * Consuming the visible hit without mutation is safer than guessing.
             */
            if (*title >= bar.menuCount()) {
                return MenuInteractionResult{};
            }

            if (!controller.begin(bar, *title)) {
                return MenuInteractionResult{};
            }

            /*
             * Enter on the explicitly selected top-level title is the existing canonical transition that
             * opens the root popup without selecting its first row. Reusing it keeps keyboard and pointer
             * entry behavior identical while this adapter remains a thin terminal-specific translation
             * layer. The controller still owns all retained semantic menu state.
             */
            return controller.handleKey(
                bar,
                KeyEvent{Key::enter, true, KeyModifier::none});
        }

        if (active_before) {
            /*
             * The press hit neither a top-level title nor a visible popup row. Close the transient menu but
             * report the event as consumed so the same physical press cannot be dispatched a second time to
             * an application control underneath the overlay.
             */
            controller.reset();
            return MenuInteractionResult{MenuInteractionAction::closed, {}};
        }

        return std::nullopt;
    }
};

} // namespace sasd::ui::terminal
