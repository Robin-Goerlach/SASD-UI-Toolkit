#include <sasd/ui/application.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/focus_traversal.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/menu_model.hpp>
#include <sasd/ui/menu_interaction_controller.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/radio_group_navigation.hpp>
#include <sasd/ui/shortcut.hpp>
#include <sasd/ui/terminal/menu_close_interaction.hpp>
#include <sasd/ui/terminal/menu_composition.hpp>
#include <sasd/ui/terminal/menu_frame_builder.hpp>
#include <sasd/ui/terminal/menu_hover_interaction.hpp>
#include <sasd/ui/terminal/menu_pointer_deferral_interaction.hpp>
#include <sasd/ui/terminal/menu_pointer_intent.hpp>
#include <sasd/ui/terminal/menu_pointer_interaction.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/terminal_backend.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/terminal/terminal_text_field_pointer_selection.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <variant>

using namespace std::chrono_literals;

namespace {

using namespace sasd::ui;
using namespace sasd::ui::terminal;

/**
 * Re-measures and arranges the demo form for the current terminal size.
 *
 * Row zero is intentionally reserved for the semantic menu bar. The Widget tree itself still knows
 * nothing about terminal menus: the form starts one cell below the top edge and menu presentation is
 * composed later as a transient overlay over the captured application frame.
 *
 * Keeping this relationship explicit in the demo is useful at the current architecture stage. Window
 * still has no implicit client-area/menu-bar policy, and inventing one here would incorrectly couple a
 * backend-neutral Widget to terminal-specific presentation geometry.
 */
void layoutForm(Window& window,
                VBox& form,
                const TerminalMeasurementContext& metrics,
                Size terminal_size) {
    window.arrange({0, 0, terminal_size.width, terminal_size.height});

    const Coordinate content_height =
        std::max<Coordinate>(0, terminal_size.height - 1);
    const Size content_size{terminal_size.width, content_height};

    /*
     * VBox receives only the space below the menu row. Measurement and arrangement therefore agree on
     * the same available client rectangle even after a terminal resize. Tiny terminals collapse the
     * client height to zero instead of producing a negative layout extent.
     */
    (void)form.measure(metrics, {{0, 0}, content_size});
    form.arrange({0, 1, terminal_size.width, content_height});
}

} // namespace

int main() {
    try {
        using namespace sasd::ui;
        using namespace sasd::ui::terminal;

        /*
         * Pointer reporting is an explicit host capability rather than a hidden TerminalBackend side
         * effect. The demo intentionally chooses all-motion tracking because its open popup menus support
         * pointer-motion row selection even when no button is held. TerminalSession still owns the complete
         * xterm protocol lifetime (DECSET 1003 plus SGR coordinates) and restores those modes together with
         * native terminal state during shutdown. The toolkit default remains the quieter button-event mode;
         * this stronger policy is therefore a deliberate demo-host choice rather than a library-wide default.
         */
        TerminalSessionOptions session_options;
        session_options.pointer_input = true;
        session_options.pointer_tracking = TerminalPointerTrackingMode::all_motion;
        TerminalBackend backend{session_options};
        Application application{backend};

        const Size initial_size = backend.terminalSize();

        ScreenBuffer screen{initial_size};
        TerminalPresentationSink presentation{screen};
        TerminalMeasurementContext metrics;

        /*
         * Commands are declared before both MenuBarModel and the Widget tree. Their semantic identity is
         * shared by menu items, ShortcutMap and bound Buttons, while destruction happens in the opposite
         * direction: Widgets, shortcut bindings and menus release their non-owning references before the
         * Commands themselves die.
         */
        Command greet_command{"Greet"};
        Command exit_command{"Exit"};
        Command help_command{"Help"};

        /*
         * The Help menu advertises F1 as presentation metadata, while ShortcutMap below owns the actual
         * input-routing policy. Keeping display metadata and dispatch registration separate is deliberate:
         * merely showing a menu item must never install an application-global keyboard route implicitly.
         */
        const Shortcut help_shortcut{Key::f1, KeyModifier::none};

        MenuBarModel menu_bar;
        MenuModel& actions_menu = menu_bar.appendMenu("Actions");
        actions_menu.appendCommand(greet_command);

        /*
         * Keep one deliberately small nested menu in the demo so delayed hover opening is observable in
         * real terminal use rather than existing only in deterministic unit tests. Reusing help_command is
         * also intentional: one semantic Command may appear on several menu surfaces without duplicating
         * application behavior or inventing a terminal-only callback.
         */
        MenuModel& more_menu = actions_menu.appendSubmenu("More");
        more_menu.appendCommand(help_command);

        actions_menu.appendSeparator();
        actions_menu.appendCommand(exit_command);

        MenuModel& help_menu = menu_bar.appendMenu("Help");
        help_menu.appendCommand(help_command, help_shortcut);

        MenuInteractionController menu_interaction;

        /*
         * ShortcutMap is an explicit application-scoped routing object. The demo intentionally binds F1
         * here instead of hard-coding help behavior into the terminal backend, Widget base class, or menu
         * controller. A future window/focus-scope policy can own a different map without changing Command.
         */
        ShortcutMap shortcuts;
        shortcuts.bind(help_shortcut, help_command);

        /*
         * RadioGroup is semantic and non-visual. It is deliberately not inferred from VBox siblings.
         * Declare it before Window so the visual tree dies first in ordinary reverse local order.
         */
        RadioGroup greeting_group;

        Window window;
        auto& form = window.emplace<VBox>();
        form.setSpacing(1);

        auto& title = form.emplace<Label>("SASD UI Toolkit - Terminal Demo");
        TextStyle title_style;
        title_style.foreground = Color::bright_cyan;
        title_style.bold = true;
        title.setTextStyle(title_style);

        auto& name_label = form.emplace<Label>("Name:");
        TextStyle name_label_style;
        name_label_style.foreground = Color::bright_blue;
        name_label.setTextStyle(name_label_style);

        auto& name = form.emplace<TextField>();
        TextStyle field_style;
        field_style.foreground = Color::bright_white;
        name.setTextStyle(field_style);

        /*
         * This is the same Core CheckBox class used by the SDL3 example. Terminal-specific indicator
         * text such as "[x]" is supplied by TerminalPresentationSink and never stored in the Widget.
         */
        auto& enthusiastic =
            form.emplace<CheckBox>("Enthusiastic greeting", true);
        TextStyle checkbox_style;
        checkbox_style.foreground = Color::bright_magenta;
        enthusiastic.setTextStyle(checkbox_style);

        /*
         * The same explicit RadioGroup/RadioButton model is used in Terminal and SDL3. The terminal
         * backend decides how "(o)" looks; application code sees only semantic selection state.
         */
        auto& hello_style =
            form.emplace<RadioButton>(greeting_group, "Greeting word: Hello");
        auto& hi_style =
            form.emplace<RadioButton>(greeting_group, "Greeting word: Hi");

        TextStyle radio_style;
        radio_style.foreground = Color::bright_blue;
        hello_style.setTextStyle(radio_style);
        hi_style.setTextStyle(radio_style);
        (void)hello_style.setSelected(true);

        auto& greet = form.emplace<Button>();
        greet.bindCommand(greet_command);
        TextStyle greet_style;
        greet_style.foreground = Color::bright_green;
        greet_style.bold = true;
        greet.setTextStyle(greet_style);

        auto& status = form.emplace<Label>(
            "F10 menu. F1 help. Mouse menus + hover/safe-triangle submenus; drag text; double-click+drag selects words.");
        TextStyle status_style;
        status_style.foreground = Color::yellow;
        status.setTextStyle(status_style);

        auto& exit = form.emplace<Button>();
        exit.bindCommand(exit_command);
        TextStyle exit_style;
        exit_style.foreground = Color::bright_red;
        exit.setTextStyle(exit_style);

        FocusManager focus;

        /*
         * PointerRouter is host-owned for the same reason FocusManager is: both describe interaction
         * state that spans multiple backend events. It remains backend-neutral and contains no terminal
         * escape/protocol state. TerminalTextFieldPointerSelection below contributes only cell geometry
         * before delegating gesture ownership back to this router.
         */
        PointerRouter pointer_router;

        /*
         * Word-granular double-click dragging needs one additional piece of semantic state across the
         * press/move/release sequence: the complete word selected by the initial double click. Keep that
         * state in the host beside PointerRouter rather than hiding it in TextField, the backend, or a
         * static helper. GestureState intentionally stores no Widget pointer; PointerRouter capture remains
         * the only authority for which Widget currently owns the physical gesture.
         *
         * Keeping both objects at the same application lifetime also makes scope transitions explicit.
         * When the demo enters modal menu interaction below, it releases Core capture and resets this
         * semantic state together so no old word origin can survive into a later unrelated pointer press.
         */
        TerminalTextFieldPointerSelection::GestureState text_selection_gesture;

        /*
         * Menu Command activation also spans two backend events: Primary press selects and arms one popup
         * row, while a matching Primary release asks Core to activate that same semantic Command. Keep the
         * tiny presentation-value identity in the host just like the TextField gesture state above. The
         * state owns no MenuModel/MenuItem/Command pointer and is always revalidated against the current
         * menu model before activation, so structural mutation cannot turn a stale press into a callback.
         */
        TerminalMenuPointerInteraction::GestureState menu_pointer_gesture;

        /*
         * Safe-triangle handling is intentionally split into geometry and timing. PointerIntent observes the
         * pre-interaction presentation and remembers only the latest value-state anchor in the parent row;
         * PointerDeferral owns the bounded time policy for a sibling row that the diagonal path happens to
         * cross. Neither helper mutates MenuInteractionController or retains a presentation-frame pointer.
         * Keeping both objects host-owned also makes input-policy handoff explicit: resize, keyboard takeover,
         * command callbacks, and non-motion pointer transitions can retire them without hidden callbacks.
         */
        TerminalMenuPointerIntent menu_pointer_intent;
        TerminalMenuPointerDeferralInteraction menu_pointer_deferral;

        /*
         * Delayed submenu opening is a second, independent piece of host-owned transient menu state. It
         * owns only value identity plus a monotonic timestamp; it has no worker thread, timer callback,
         * MenuItem pointer, or retained presentation frame. Pointer handling below first performs immediate
         * row/title semantics and then feeds the same event into this timing policy. The normal main loop
         * advances the deadline, so hover opening remains deterministic and synchronized with presentation.
         */
        TerminalMenuHoverInteraction menu_hover;

        /*
         * Submenu closing has a separate grace policy instead of being folded into hover opening. The
         * distinction matters geometrically: moving onto another real popup row should keep using immediate
         * Core selection/path coherence, while a brief trip through cells belonging to no popup at all may
         * simply be the physical path from a parent popup into its child. This host-owned object therefore
         * stores only value scope plus a monotonic timestamp and closes descendants only after its grace
         * expires. Like menu_hover it owns no thread, callback, MenuItem pointer, or presentation frame.
         */
        TerminalMenuCloseInteraction menu_close;

        /*
         * Button activation, menu activation and shortcut activation deliberately share the same Command
         * callback. This is the practical reason Command is semantic and backend-neutral: none of the
         * presentation/input surfaces owns a duplicate copy of application behavior.
         */
        greet_command.setOnExecuted([&] {
            std::string value{name.text()};
            if (value.empty()) {
                value = "world";
            }

            /*
             * Label::setText() invalidates measurement and presentation. The main loop below notices
             * that the form measurement became stale and performs a fresh layout before repainting.
             *
             * The punctuation comes only from semantic CheckBox state. Terminal marker geometry is a
             * presentation detail and therefore cannot leak into this application-level decision.
             */
            const char punctuation = enthusiastic.isChecked() ? '!' : '.';
            const std::string greeting_word =
                hi_style.isSelected() ? "Hi" : "Hello";
            status.setText(
                greeting_word + ", " + value + std::string(1, punctuation));
        });

        help_command.setOnExecuted([&] {
            status.setText(
                "Help: F10/menu clicks; hover More > to open it; diagonal transfer uses safe-triangle grace; F1 shortcut; mouse drag selects chars; double-click+drag extends by words; triple-click selects all.");
        });

        exit_command.setOnExecuted([&] {
            application.requestExit();
        });

        enthusiastic.setOnCheckedChanged([&](bool checked) {
            status.setText(
                checked
                    ? "Greeting style: enthusiastic (!)"
                    : "Greeting style: calm (.)");
        });

        hello_style.setOnSelected([&] {
            status.setText("Greeting word selected: Hello");
        });

        hi_style.setOnSelected([&] {
            status.setText("Greeting word selected: Hi");
        });

        layoutForm(window, form, metrics, initial_size);
        (void)focus.requestFocus(name);

        /**
         * Captures the current application frame, composes menu presentation over that immutable value,
         * then transports the resulting complete frame. The lambda contains no widget synchronization;
         * callers decide when the base application presentation is current before invoking it.
         *
         * A failed composition is treated as a demo-level presentation error. The lower menu API remains
         * transactional and simply returns std::nullopt; the demo chooses to surface that condition because
         * continuing with a stale visible menu would be more confusing than terminating with diagnostics.
         */
        const auto present_current_frame = [&] {
            const TerminalPresentationFrame base_frame = presentation.captureFrame();
            const auto composed = composeMenuInteractionFrame(
                base_frame,
                menu_bar,
                menu_interaction,
                {0, 0},
                presentation.ambiguousWidthMode());

            if (!composed.has_value()) {
                throw std::runtime_error(
                    "terminal demo menu state cannot be represented in the current viewport");
            }

            backend.session().present(*composed);
        };

        /*
         * Initial full presentation. The Widget tree first updates the reusable ScreenBuffer, then
         * captureFrame() establishes an owned base value and menu composition adds the persistent menu bar.
         */
        const auto initial_pass =
            PresentationCoordinator::synchronize(window, presentation);
        if (!initial_pass.complete()) {
            throw std::runtime_error(
                "terminal demo contains presentation state the current Cell model cannot render");
        }
        present_current_frame();

        /*
         * Command activation returned by MenuInteractionController is intentionally delayed until after
         * the menu has been repainted closed. This follows the controller contract: transient presentation
         * state is stabilized first, then arbitrary application callbacks are allowed to run.
         */
        Command::Reference pending_menu_command;

        while (!application.exitRequested()) {
            bool resized = false;
            bool menu_presentation_changed = false;

            /*
             * Both physical pointer samples and an expired safe-triangle sample must enter the exact same
             * immediate menu adapter. Centralizing that path here is important: the deferral policy returns
             * only a copied PointerEvent and must not grow a second implementation of popup selection,
             * command activation, modal consumption, or hover/close observation.
             *
             * The supplied frame is the presentation transaction that existed immediately before this event
             * is applied. TerminalMenuPointerInteraction may change MenuPath, after which hover/close timing
             * observes the resulting semantic state while using this frame only for the current synchronous
             * hit identity. Neither timing helper retains the frame beyond the call.
             */
            const auto process_menu_pointer_now =
                [&](const MenuFramePresentationSnapshot& menu_frame,
                    const PointerEvent& pointer,
                    bool menu_was_active,
                    std::chrono::steady_clock::time_point menu_timing_now) {
                    const auto menu_result = TerminalMenuPointerInteraction::handle(
                        menu_bar,
                        menu_interaction,
                        menu_frame,
                        pointer,
                        presentation.ambiguousWidthMode(),
                        menu_pointer_gesture);

                    menu_hover.observe(
                        menu_interaction,
                        menu_frame,
                        pointer,
                        presentation.ambiguousWidthMode(),
                        menu_timing_now);
                    menu_close.observe(
                        menu_interaction,
                        menu_frame,
                        pointer,
                        presentation.ambiguousWidthMode(),
                        menu_timing_now);

                    if (!menu_result.has_value()) {
                        return false;
                    }

                    /*
                     * An engaged result means the transient menu scope owns this physical event, even when
                     * its semantic action is `none`. Keep that modal boundary strict so no title/popup event,
                     * outside-dismiss press, motion, release, or replayed deferral also reaches a Widget below.
                     */
                    if (menu_was_active || menu_interaction.isActive()) {
                        pointer_router.releaseCapture();
                        text_selection_gesture.reset();
                    }

                    if (menu_result->action != MenuInteractionAction::none) {
                        menu_presentation_changed = true;
                    }

                    /*
                     * Command activation is still two-phase. Store only the lifetime-safe reference here;
                     * presentation later observes the controller's already-closed menu state before arbitrary
                     * client code executes. The pointer-intent/deferral objects are reset at that callback
                     * boundary as an additional guarantee that no trajectory state enters application code.
                     */
                    if (menu_result->action == MenuInteractionAction::activate_command) {
                        pending_menu_command = menu_result->command;
                    }

                    return true;
                };

            (void)application.processRoutedEvents(
                [&](const Event& event) -> Widget* {
                    if (std::holds_alternative<KeyEvent>(event) ||
                        std::holds_alternative<TextInputEvent>(event)) {
                        /*
                         * While menu interaction is active, keyboard ownership belongs to the menu layer.
                         * Returning no Widget prevents a focused TextField or Button from consuming an arrow,
                         * Enter, Escape, or text event before the application-level menu controller sees it.
                         */
                        return menu_interaction.isActive() ? nullptr : focus.focusedWidget();
                    }

                    /*
                     * PointerRouter owns pointer hit testing/capture and TerminalTextFieldPointerSelection
                     * adds terminal-specific text geometry in the fallback handler below. Returning nullptr
                     * here is essential: Application must not perform a second ordinary dispatch of the
                     * same PointerEvent after that helper has already routed it through Core.
                     *
                     * Resize and other backend-level events are likewise handled outside Widget routing.
                     */
                    return nullptr;
                },
                [&](const Event& event) {
                    if (const auto* resize = std::get_if<ResizeEvent>(&event)) {
                        /*
                         * Every pointer-owned menu identity is tied to the current presented geometry. A
                         * resize can reposition or flip popups before a click, safe-triangle deadline, hover
                         * deadline, or close deadline completes. Retire all geometry-derived state together;
                         * semantic menu selection itself remains owned by the controller and survives resize.
                         */
                        menu_pointer_gesture.reset();
                        menu_pointer_intent.reset();
                        menu_pointer_deferral.reset();
                        menu_hover.reset();
                        menu_close.reset();
                        screen.resize(resize->size);
                        layoutForm(window, form, metrics, resize->size);
                        resized = true;
                        return;
                    }

                    if (const auto* pointer = std::get_if<PointerEvent>(&event)) {
                        const bool menu_was_active = menu_interaction.isActive();
                        const bool primary_press_without_widget_capture =
                            !pointer_router.hasCapture() &&
                            pointer->action == PointerAction::press &&
                            pointer->button == PointerButton::primary;

                        /*
                         * Menu pointer interaction gets first refusal only while a transient menu is
                         * already active or for a fresh uncaptured primary press that could land on the
                         * persistent menu bar. An application Widget that already owns Core capture keeps
                         * that gesture; a second hit-test layer must not steal it merely because the pointer
                         * happens to cross row zero during a drag.
                         *
                         * Build one owned pre-event frame from the controller state and current viewport.
                         * Pointer intent MUST inspect this frame before ordinary row selection: selecting a
                         * sibling first could already close the child whose transfer corridor is being tested.
                         * The frame stays ephemeral and is never retained by any policy object.
                         */
                        if (menu_was_active || primary_press_without_widget_capture) {
                            const auto menu_frame = buildMenuPresentationFrame(
                                menu_bar,
                                menu_interaction,
                                {0, 0},
                                screen.size(),
                                presentation.ambiguousWidthMode());

                            if (!menu_frame.has_value()) {
                                throw std::runtime_error(
                                    "terminal demo menu pointer frame cannot be represented in the current viewport");
                            }

                            const auto menu_timing_now = std::chrono::steady_clock::now();
                            const TerminalMenuPointerIntentKind pointer_intent =
                                menu_pointer_intent.observe(
                                    menu_interaction,
                                    *menu_frame,
                                    *pointer,
                                    presentation.ambiguousWidthMode());
                            const TerminalMenuPointerDeferralDecision deferral_decision =
                                menu_pointer_deferral.observe(
                                    menu_interaction,
                                    *menu_frame,
                                    *pointer,
                                    pointer_intent,
                                    presentation.ambiguousWidthMode(),
                                    menu_timing_now);

                            if (deferral_decision ==
                                TerminalMenuPointerDeferralDecision::defer) {
                                /*
                                 * The pointer is physically over a sibling parent row but is still travelling
                                 * inside the safe triangle toward the already-open child. Consuming this sample
                                 * without immediate Core selection is the whole point of the grace period.
                                 *
                                 * Hover-open and gap-close belong to different policies and must not inherit
                                 * this deliberately suppressed sibling sample. Clear both candidates now. The
                                 * active menu is modal, so application capture/word-drag residue is retired just
                                 * as it would be for an immediately consumed menu motion.
                                 */
                                menu_hover.reset();
                                menu_close.reset();
                                pointer_router.releaseCapture();
                                text_selection_gesture.reset();
                                return;
                            }

                            if (process_menu_pointer_now(
                                    *menu_frame,
                                    *pointer,
                                    menu_was_active,
                                    menu_timing_now)) {
                                return;
                            }
                        }

                        /*
                         * Focus-on-primary-press is host policy, not terminal geometry. This mirrors the
                         * SDL3 demo: the deepest focusable Core widget receives logical focus before the
                         * same semantic PointerEvent is routed. Labels/non-focusable chrome simply leave
                         * the current focus unchanged.
                         */
                        if (!pointer_router.hasCapture() &&
                            pointer->action == PointerAction::press &&
                            pointer->button == PointerButton::primary) {
                            Widget* hit = HitTest::deepestAt(window, pointer->position);
                            if (hit != nullptr && hit->canReceiveFocus()) {
                                (void)focus.requestFocus(*hit);
                            }
                        }

                        /*
                         * Route every application pointer event through the stateful terminal TextField
                         * seam. Ordinary controls still delegate to the normal PointerRouter path. For a
                         * TextField, the host-owned GestureState lets the initial double-click word survive
                         * across captured motion so later cells extend the selection by complete semantic
                         * runs instead of degrading to character endpoints. Ordinary click/drag remains
                         * character-granular and triple-click select-all remains atomic.
                         *
                         * The presentation width mode is supplied explicitly so painting and hit geometry
                         * cannot silently disagree about East Asian ambiguous-width characters.
                         */
                        (void)TerminalTextFieldPointerSelection::route(
                            window,
                            pointer_router,
                            *pointer,
                            presentation.ambiguousWidthMode(),
                            text_selection_gesture);
                        return;
                    }

                    if (const auto* key = std::get_if<KeyEvent>(&event); key != nullptr) {
                        /*
                         * F10 is application policy for entering/leaving keyboard menu mode. It is not baked
                         * into MenuInteractionController because other applications may choose Alt, a mouse
                         * gesture, a platform mnemonic convention, or no global activation key at all.
                         */
                        if (key->pressed &&
                            key->modifiers == KeyModifier::none &&
                            key->key == Key::f10) {
                            /*
                             * Switching menu scope from the keyboard invalidates every pending pointer-owned
                             * menu transaction. Retire press identity, safe-triangle geometry/timing, delayed
                             * open, and delayed close before opening/closing the menu so no later physical or
                             * replayed event can complete across scopes.
                             */
                            menu_pointer_gesture.reset();
                            menu_pointer_intent.reset();
                            menu_pointer_deferral.reset();
                            menu_hover.reset();
                            menu_close.reset();

                            if (menu_interaction.isActive()) {
                                menu_interaction.reset();
                            } else {
                                /*
                                 * Modal menu interaction supersedes an in-progress application pointer
                                 * gesture. Releasing Core capture keeps TextField/Button transient state
                                 * synchronized and preserves any semantic selection reached before F10.
                                 * Reset the host-owned word origin at the same scope boundary: pointer
                                 * reports are ignored while the menu is active, so retaining that semantic
                                 * origin would make it outlive the physical gesture that justified it.
                                 */
                                pointer_router.releaseCapture();
                                text_selection_gesture.reset();
                                (void)menu_interaction.begin(menu_bar);
                            }
                            menu_presentation_changed = true;
                            return;
                        }

                        if (menu_interaction.isActive()) {
                            /*
                             * Keyboard input and pointer-owned menu policies are competing completion paths
                             * for the same transient menu state. The moment keyboard policy takes ownership,
                             * cancel press identity, safe-triangle intent/deferral, and both delayed timers
                             * even if this key later produces no semantic change.
                             */
                            menu_pointer_gesture.reset();
                            menu_pointer_intent.reset();
                            menu_pointer_deferral.reset();
                            menu_hover.reset();
                            menu_close.reset();

                            const MenuInteractionResult menu_result =
                                menu_interaction.handleKey(menu_bar, *key);

                            if (menu_result.action != MenuInteractionAction::none) {
                                menu_presentation_changed = true;
                            }

                            if (menu_result.action == MenuInteractionAction::activate_command) {
                                pending_menu_command = menu_result.command;
                            }

                            /*
                             * Active menu mode owns all keyboard input, including keys the current menu
                             * interpreter deliberately ignores. Falling through to shortcuts or
                             * FocusTraversal here would make menu behavior depend on whichever application
                             * scope happened to register the same gesture.
                             */
                            return;
                        }

                        /*
                         * Application shortcuts run only after transient menu mode has declined ownership.
                         * dispatch() enters arbitrary Command callbacks synchronously, so return immediately
                         * on success and do not touch ShortcutMap again from this event transaction. Disabled
                         * or unmatched commands return false and intentionally leave the key available to the
                         * remaining focus/routing policy below.
                         */
                        if (shortcuts.dispatch(*key)) {
                            return;
                        }
                    }

                    /*
                     * Arrow navigation for RadioGroup is an explicit focus-scope policy, separate from
                     * RadioButton's own Space-to-select semantics and generic Tab traversal.
                     */
                    if (auto* radio =
                            dynamic_cast<RadioButton*>(focus.focusedWidget());
                        radio != nullptr &&
                        RadioGroupNavigation::handleEvent(
                            focus, *radio, event) == EventResult::handled) {
                        return;
                    }

                    if (FocusTraversal::handleEvent(focus, form, event) ==
                        EventResult::handled) {
                        return;
                    }

                    if (const auto* key = std::get_if<KeyEvent>(&event);
                        key != nullptr &&
                        key->pressed &&
                        key->modifiers == KeyModifier::none &&
                        key->key == Key::escape) {
                        application.requestExit();
                    }
                });

            if (application.exitRequested()) {
                break;
            }

            /*
             * All menu deadlines advance from this ordinary polling loop. Safe-triangle deferral is checked
             * first because its expired PointerEvent must pass through the immediate pointer adapter before
             * hover-open or gap-close can observe/advance the resulting semantic state. No policy receives a
             * background callback and all four use the same monotonic clock domain.
             */
            const auto menu_timing_now = std::chrono::steady_clock::now();

            if (menu_pointer_deferral.hasPendingEvent()) {
                const auto menu_frame = buildMenuPresentationFrame(
                    menu_bar,
                    menu_interaction,
                    {0, 0},
                    screen.size(),
                    presentation.ambiguousWidthMode());

                if (!menu_frame.has_value()) {
                    throw std::runtime_error(
                        "terminal demo deferred menu pointer frame cannot be represented in the current viewport");
                }

                if (const auto deferred_pointer = menu_pointer_deferral.advance(
                        menu_interaction,
                        *menu_frame,
                        presentation.ambiguousWidthMode(),
                        menu_timing_now);
                    deferred_pointer.has_value()) {
                    /*
                     * Expiration commits to the sibling replacement that was deliberately held back. Retire
                     * the old geometric transfer anchor before replay so that a later physical point cannot
                     * continue a trajectory whose owning child may now be closed. The copied event then uses
                     * the same immediate helper as a physical sample; Core alone decides what selection/path
                     * transition is still legal against the current model.
                     */
                    const bool menu_was_active = menu_interaction.isActive();
                    menu_pointer_intent.reset();
                    (void)process_menu_pointer_now(
                        *menu_frame,
                        *deferred_pointer,
                        menu_was_active,
                        menu_timing_now);
                }
            }

            const MenuInteractionResult hover_result = menu_hover.advance(
                menu_bar,
                menu_interaction,
                menu_timing_now);
            if (hover_result.action != MenuInteractionAction::none) {
                menu_presentation_changed = true;
            }

            const MenuInteractionResult close_result = menu_close.advance(
                menu_bar,
                menu_interaction,
                menu_timing_now);
            if (close_result.action != MenuInteractionAction::none) {
                menu_presentation_changed = true;
            }

            /*
             * Content changes such as the greeting Label can invalidate natural sizes without a terminal
             * resize. Re-measure only when required; cursor/focus-only changes stay visual.
             */
            if (!form.isMeasureValid()) {
                layoutForm(window, form, metrics, backend.terminalSize());
            }

            const auto pass =
                PresentationCoordinator::synchronize(window, presentation);

            if (!pass.complete()) {
                /*
                 * The current terminal Cell model deliberately defers combining/ZWJ content it cannot
                 * preserve. The sample keeps running instead of acknowledging lossy presentation.
                 */
            }

            if (pass.requested != 0 || resized || menu_presentation_changed) {
                present_current_frame();
            }

            if (Command* command = pending_menu_command.get(); command != nullptr) {
                /*
                 * The controller activation transaction already closed menu state before returning the
                 * lifetime-safe reference. Because presentation above observes that closed state first,
                 * keyboard and pointer callbacks may now rebuild menus, alter Widgets, or request exit
                 * without re-entering transient popup state. Clear every host-owned pointer menu policy as
                 * an explicit arbitrary-code boundary even when the path that activated the command already
                 * retired some of them naturally.
                 */
                menu_pointer_intent.reset();
                menu_pointer_deferral.reset();
                menu_hover.reset();
                menu_close.reset();
                pending_menu_command = {};
                (void)command->execute();
            } else {
                pending_menu_command = {};
            }

            /*
             * Backend polling is deliberately non-blocking. This small sleep bounds idle CPU usage while
             * keeping the demo responsive and allows TerminalEventPump's incomplete-sequence timeout to
             * advance. A later wait/wakeup abstraction can replace polling without changing decoding,
             * menu semantics, shortcut routing, or widget semantics.
             */
            std::this_thread::sleep_for(8ms);
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        /*
         * If Application/TerminalSession was constructed, RAII has already restored raw mode,
         * alternate screen, cursor visibility and platform console state before this is printed.
         */
        std::cerr << "SASD UI terminal demo failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
