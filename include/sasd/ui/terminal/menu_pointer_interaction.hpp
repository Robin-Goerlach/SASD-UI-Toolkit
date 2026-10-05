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
 * Pointer motion over a visible popup row is translated into the same backend-neutral selectPopupItem()
 * transaction used by a Primary press. Motion over a different visible top-level title while menu interaction
 * is already active switches the selected root popup using the controller's established begin()+Enter
 * transition. This mirrors desktop menu bars without moving terminal geometry into Core. Motion still does not
 * open child submenus and does not activate Commands. Whether the terminal produces button-held motion or
 * passive/all-motion reports is a TerminalSession protocol policy outside this semantic adapter.
 *
 * Stateful handling additionally gives popup items desktop-style press/release completion: press selects and
 * arms one popup identity, and a matching release asks the backend-neutral controller to open that selected
 * submenu or activate that selected Command. Motion may change semantic popup-row selection while deliberately
 * leaving the armed press identity unchanged. A top-level title switch is different: it replaces the root menu
 * model, so any armed popup identity is retired before the switch to prevent the same numeric level/index from
 * being reinterpreted in another top-level menu.
 */
class TerminalMenuPointerInteraction final {
public:
    /**
     * Host-owned transient state for one possible popup item click.
     *
     * Only presentation value identity (popup level + row index) is retained. No MenuModel, MenuItem, Command,
     * Widget, terminal device, or frame pointer survives between events. The release transaction resolves and
     * validates the current semantic model again before any completion can be returned.
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
     * Stateless pointer handling for menu selection and modal consumption.
     *
     * Primary press can open/switch a top-level menu, select an enabled popup row, or dismiss an active menu
     * from outside. Pointer motion over an active popup can select a row, and motion over a different visible
     * top-level title switches the open root popup to that title. Motion never activates a Command or opens a
     * child submenu. Releases remain consumed while a menu is active but cannot complete popup items because
     * this overload intentionally retains no press identity between events.
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
     * Stateful pointer handling that completes a selected popup item on matching Primary release.
     *
     * A Primary press on a visible popup row first performs the same selectPopupItem() transaction as the
     * stateless overload and records only that painted level/row identity. Motion can update Core's current
     * row selection, but deliberately does not rewrite the armed identity while remaining in the same root
     * menu. If motion switches to another top-level title, the armed identity is reset because level/row values
     * are meaningful only within the old root menu model. A later Primary release must hit the exact same
     * topmost popup row and Core must still prove that identity as the current selection; otherwise completion
     * is cancelled while the menu remains active.
     *
     * Even a matching geometric release is only a request. The adapter first asks
     * MenuInteractionController::openPopupSubmenu() whether the selected live item owns a child popup. A
     * successful submenu request opens exactly one child level with no initial selection. If no submenu
     * transition applies, activatePopupItem() independently proves whether the same identity is a selected
     * live enabled Command. Command success closes all menu state and returns a lifetime-safe
     * Command::Reference without executing client code, so the host can repaint before callbacks run.
     *
     * Separator and disabled rows remain consumed but cannot complete semantically. An already-open submenu
     * is a no-op and preserves its descendants. Motion-driven selection does not add delayed submenu opening;
     * that remains a separate policy.
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
                         * and ask Core to validate current selection/item semantics before completion.
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
                 * Retire the physical press identity before asking Core for completion. A successful Command
                 * result may later execute arbitrary application code after the host repaints, while a submenu
                 * result changes presentation geometry. Neither transition may inherit stale press residue.
                 */
                gesture_state->reset();

                const auto release_hit =
                    TerminalMenuHitTest::popupItemAt(frame, event.position, ambiguous_width);
                if (release_hit.has_value() && *release_hit == pressed) {
                    /*
                     * The terminal adapter intentionally does not inspect MenuItemKind. Ask Core first whether
                     * this selected identity is an enabled submenu; if so, opening one child is the complete
                     * semantic result of the click. Any normalization repair is also returned immediately so a
                     * pre-repair pointer identity cannot fall through and acquire a different meaning.
                     */
                    MenuInteractionResult submenu_result = controller.openPopupSubmenu(
                        bar,
                        pressed.level,
                        pressed.item_index);
                    if (submenu_result.action != MenuInteractionAction::none) {
                        return submenu_result;
                    }

                    /*
                     * If no submenu transition applies, the same exact identity may be a Command. The
                     * activation transaction performs its own current-model proof, closes menu state on
                     * success and returns only a lifetime-safe reference for delayed host execution.
                     */
                    return controller.activatePopupItem(
                        bar,
                        pressed.level,
                        pressed.item_index);
                }

                /*
                 * Release on another row/outside cancels completion but remains inside the active menu's
                 * modal pointer scope. Selection established by the press is intentionally preserved.
                 */
                return MenuInteractionResult{};
            }

            /*
             * Stateless callers and title presses have no armed popup identity. Preserve the existing rule
             * that releases are consumed while the transient menu is active without inventing completion.
             */
            return MenuInteractionResult{};
        }

        if (event.action == PointerAction::move) {
            if (!active_before) {
                return std::nullopt;
            }

            /*
             * Motion uses the same topmost-popup geometry as press handling, but it is deliberately only a
             * selection transaction. This keeps terminal-specific coordinates out of Core and lets the
             * controller enforce semantic availability plus parent/child path coherence. In particular,
             * moving to a different ancestor row can close descendants that no longer belong to the selected
             * parent, while moving over the currently owning submenu preserves that valid child route.
             */
            if (const auto popup_hit =
                    TerminalMenuHitTest::popupItemAt(frame, event.position, ambiguous_width);
                popup_hit.has_value()) {
                return controller.selectPopupItem(
                    bar,
                    popup_hit->level,
                    popup_hit->item_index);
            }

            /*
             * Once a transient menu is active, moving over another visible top-level title follows the familiar
             * desktop-menu rule: replace the root popup immediately, but do not select any row in the new menu.
             * Popup hit testing remains first because popups are painted above the bar and therefore own any
             * overlapping cell. A title index is rechecked against the current semantic model before begin()
             * so a stale presentation snapshot is consumed rather than allowed to trigger begin()'s fallback.
             */
            if (const auto title =
                    TerminalMenuHitTest::menuBarIndexAt(frame, event.position, ambiguous_width);
                title.has_value()) {
                if (*title >= bar.menuCount()) {
                    return MenuInteractionResult{};
                }

                const bool same_open_title =
                    controller.menuBarSelection() == *title && controller.popupOpen();
                if (same_open_title) {
                    /*
                     * Do not collapse descendants merely because the pointer crossed the already-active title.
                     * This also preserves an armed popup click when motion briefly traverses that same title and
                     * later returns to the original row before release.
                     */
                    return MenuInteractionResult{};
                }

                if (gesture_state != nullptr) {
                    /*
                     * A top-level switch replaces the root MenuModel. The same numeric {level,item_index}
                     * could name a completely different item after the switch, so an armed popup press must
                     * never survive this semantic scope boundary.
                     */
                    gesture_state->reset();
                }

                if (!controller.begin(bar, *title)) {
                    return MenuInteractionResult{};
                }

                return controller.handleKey(
                    bar,
                    KeyEvent{Key::enter, true, KeyModifier::none});
            }

            /*
             * Moving across other menu chrome or outside the popup surface remains modal/consumed while
             * interaction is active but does not clear the current row selection. Delayed submenu opening and
             * submenu-close timing remain separate policy decisions rather than side effects here.
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
         * Active menus form a transient modal pointer scope. Currently unsupported non-motion transitions are
         * consumed without changing semantic state so they cannot leak through popup chrome into Widgets
         * underneath. Pointer motion has already been handled above and intentionally leaves any armed Primary
         * identity intact while updating only Core's current row selection.
         */
        return active_before
                   ? std::optional<MenuInteractionResult>{MenuInteractionResult{}}
                   : std::nullopt;
    }
};

} // namespace sasd::ui::terminal
