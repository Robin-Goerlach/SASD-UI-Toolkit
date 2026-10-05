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
 * - state_changed/activate_command/closed reuse the existing MenuInteractionAction vocabulary so hosts can
 *   rebuild presentation and delay application callbacks without learning a terminal-specific state machine.
 *
 * Stateless handling preserves the established press-only selection behavior. Stateful handling additionally
 * gives command items desktop-style press/release activation: press selects and arms one popup identity,
 * release activates only when it lands on that same row and the backend-neutral controller can still prove
 * that the row is the selected live command. Submenu opening remains a separate later gesture policy.
 */
class TerminalMenuPointerInteraction final {
public:
    /**
     * Host-owned transient state for one possible popup command click.
     *
     * Only presentation value identity (popup level + row index) is retained. No MenuModel, MenuItem, Command,
     * Widget, terminal device, or frame pointer survives between events. The release transaction resolves and
     * validates the current semantic model again before any activation can be returned.
     */
    class GestureState final {
    public:
        GestureState() = default;

        void reset() noexcept {
            pressed_popup_item_.reset();
        }

        [[nodiscard]] bool hasPressedPopupItem() const noexcept {
            return pressed_popup_item_.has_value();
        }

    private:
        friend class TerminalMenuPointerInteraction;

        std::optional<TerminalMenuPopupHit> pressed_popup_item_{};
    };

    TerminalMenuPointerInteraction() = delete;

    /**
     * Stateless pointer handling preserving the existing atomic row-selection behavior.
     *
     * Primary press can open/switch a top-level menu, select an enabled popup row, or dismiss an active menu
     * from outside. Releases remain consumed while a menu is active but cannot activate Commands because this
     * overload intentionally retains no press identity between events.
     */
    [[nodiscard]] static std::optional<MenuInteractionResult>
    handle(const MenuBarModel& bar,
           MenuInteractionController& controller,
           const MenuFramePresentationSnapshot& frame,
           const PointerEvent& event,
           AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        return handleImpl(bar, controller, frame, event, ambiguous_width, nullptr);
    }

    /**
     * Stateful pointer handling that additionally activates a selected Command on matching Primary release.
     *
     * A Primary press on a visible popup row first performs the same selectPopupItem() transaction as the
     * stateless overload and records only that painted level/row identity. Motion does not change the armed
     * identity. A later Primary release must hit the exact same topmost popup row; otherwise activation is
     * cancelled while the menu remains active.
     *
     * Even a matching geometric release is only a request. MenuInteractionController::activatePopupItem()
     * re-normalizes the semantic tree, requires that exact row to remain selected in the deepest open popup,
     * and accepts only a live enabled Command. On success the controller closes all menu state and returns a
     * lifetime-safe Command::Reference without executing client code. The host can therefore repaint the
     * closed menu before executing the command, exactly like keyboard activation.
     *
     * Separator, disabled and submenu rows may still be consumed/selected according to existing press policy,
     * but release never activates them. Submenu opening is intentionally deferred to a later semantic slice.
     */
    [[nodiscard]] static std::optional<MenuInteractionResult>
    handle(const MenuBarModel& bar,
           MenuInteractionController& controller,
           const MenuFramePresentationSnapshot& frame,
           const PointerEvent& event,
           AmbiguousWidthMode ambiguous_width,
           GestureState& gesture_state) {
        return handleImpl(bar, controller, frame, event, ambiguous_width, &gesture_state);
    }

private:
    [[nodiscard]] static std::optional<MenuInteractionResult>
    handleImpl(const MenuBarModel& bar,
               MenuInteractionController& controller,
               const MenuFramePresentationSnapshot& frame,
               const PointerEvent& event,
               AmbiguousWidthMode ambiguous_width,
               GestureState* gesture_state) {
        const bool active_before = controller.isActive();
        const bool primary_press =
            event.action == PointerAction::press && event.button == PointerButton::primary;
        const bool primary_release =
            event.action == PointerAction::release && event.button == PointerButton::primary;

        if (gesture_state != nullptr && !active_before) {
            /*
             * Keyboard/F10/application policy may have closed the menu between two pointer reports. Because
             * GestureState has no controller callback dependency, the next observed event is the natural
             * synchronization point for retiring any stale press identity.
             */
            gesture_state->reset();
        }

        if (primary_press) {
            if (gesture_state != nullptr) {
                /*
                 * Every fresh Primary press starts a new possible click transaction. Reset before hit testing
                 * so an outside/title/unavailable press can never inherit an older popup release target.
                 */
                gesture_state->reset();
            }

            /*
             * Popup layers are painted after the menu bar, so they own overlapping cells while interaction is
             * active. Geometry identifies only the painted level/row. The backend-neutral controller then owns
             * the semantic proof that the same identity still names a selectable live item in the current model.
             *
             * selectPopupItem() may return action==none for a separator, disabled command, or stale frame
             * identity. We still return an engaged result: the visible popup surface consumed the physical
             * press even when no semantic selection transition was legal.
             */
            if (active_before) {
                if (const auto popup_hit =
                        TerminalMenuHitTest::popupItemAt(frame, event.position, ambiguous_width);
                    popup_hit.has_value()) {
                    MenuInteractionResult result = controller.selectPopupItem(
                        bar,
                        popup_hit->level,
                        popup_hit->item_index);

                    if (gesture_state != nullptr && controller.isActive()) {
                        /*
                         * Store presentation value identity only. Release will re-hit-test the current frame
                         * and ask Core to validate current selection/item semantics before activation.
                         */
                        gesture_state->pressed_popup_item_ = *popup_hit;
                    }

                    return result;
                }
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

        if (primary_release) {
            if (!active_before) {
                return std::nullopt;
            }

            if (gesture_state != nullptr && gesture_state->pressed_popup_item_.has_value()) {
                const TerminalMenuPopupHit pressed = *gesture_state->pressed_popup_item_;

                /*
                 * Retire the physical press identity before asking Core for activation. Even a successful
                 * result may later execute arbitrary application code after the host repaints, so no gesture
                 * residue should survive past this release transaction.
                 */
                gesture_state->reset();

                const auto release_hit =
                    TerminalMenuHitTest::popupItemAt(frame, event.position, ambiguous_width);
                if (release_hit.has_value() && *release_hit == pressed) {
                    return controller.activatePopupItem(
                        bar,
                        pressed.level,
                        pressed.item_index);
                }

                /*
                 * Release on another row/outside cancels activation but remains inside the active menu's
                 * modal pointer scope. Selection established by the press is intentionally preserved.
                 */
                return MenuInteractionResult{};
            }

            /*
             * Stateless callers and title presses have no armed popup identity. Preserve the existing rule
             * that releases are consumed while the transient menu is active without inventing activation.
             */
            return MenuInteractionResult{};
        }

        if (gesture_state != nullptr && event.action == PointerAction::press) {
            /*
             * A non-primary press interrupts the pending primary click transaction even though this slice does
             * not assign another menu meaning to that button. The event remains modal/consumed below.
             */
            gesture_state->reset();
        }

        /*
         * Active menus form a transient modal pointer scope. Motion and currently unsupported pointer
         * transitions are consumed without changing semantic state so they cannot leak through popup chrome
         * into Widgets underneath. Motion deliberately leaves an armed primary identity intact: moving away
         * and back before releasing on the original row is still a valid click completion.
         */
        return active_before
                   ? std::optional<MenuInteractionResult>{MenuInteractionResult{}}
                   : std::nullopt;
    }
};

} // namespace sasd::ui::terminal
