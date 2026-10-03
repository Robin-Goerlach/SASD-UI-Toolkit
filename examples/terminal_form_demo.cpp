#include <sasd/ui/application.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/focus_traversal.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/menu_model.hpp>
#include <sasd/ui/menu_interaction_controller.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/radio_group_navigation.hpp>
#include <sasd/ui/terminal/menu_composition.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/terminal_backend.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
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

        TerminalBackend backend;
        Application application{backend};

        const Size initial_size = backend.terminalSize();

        ScreenBuffer screen{initial_size};
        TerminalPresentationSink presentation{screen};
        TerminalMeasurementContext metrics;

        /*
         * Commands are declared before both MenuBarModel and the Widget tree. Their semantic identity is
         * shared by menu items and bound Buttons, while destruction happens in the opposite direction:
         * Widgets and menus release their non-owning references before the Commands themselves die.
         */
        Command greet_command{"Greet"};
        Command exit_command{"Exit"};
        Command help_command{"Help"};

        MenuBarModel menu_bar;
        MenuModel& actions_menu = menu_bar.appendMenu("Actions");
        actions_menu.appendCommand(greet_command);
        actions_menu.appendSeparator();
        actions_menu.appendCommand(exit_command);

        MenuModel& help_menu = menu_bar.appendMenu("Help");
        help_menu.appendCommand(help_command);

        MenuInteractionController menu_interaction;

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
            "F10 opens menu. Tab moves focus. Space toggles/selects controls.");
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
         * Button activation and menu activation deliberately share the same Command callback. This is the
         * practical reason Command is semantic and backend-neutral: neither the Button nor the menu item
         * owns a duplicate copy of application behavior.
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
                "Help: F10 menu; arrows navigate menus/radios; Tab changes focus; Enter/Space activates.");
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

                    // Resize and future backend-level events are handled outside widget routing.
                    return nullptr;
                },
                [&](const Event& event) {
                    if (const auto* resize = std::get_if<ResizeEvent>(&event)) {
                        screen.resize(resize->size);
                        layoutForm(window, form, metrics, resize->size);
                        resized = true;
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
                            if (menu_interaction.isActive()) {
                                menu_interaction.reset();
                            } else {
                                (void)menu_interaction.begin(menu_bar);
                            }
                            menu_presentation_changed = true;
                            return;
                        }

                        if (menu_interaction.isActive()) {
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
                             * interpreter deliberately ignores. Falling through to FocusTraversal here would
                             * make menu behavior depend on whichever Widget happened to remain focused.
                             */
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
                        key->modifiers == KeyModifier::none) {
                        if (key->key == Key::f1) {
                            /*
                             * F1 remains an application-level convenience in addition to the Help menu.
                             * Both routes update the same status presentation without introducing Help
                             * behavior into the terminal backend or a generic Widget base class.
                             */
                            (void)help_command.execute();
                            return;
                        }

                        if (key->key == Key::escape) {
                            application.requestExit();
                        }
                    }
                });

            if (application.exitRequested()) {
                break;
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
                 * handleKey() already closed the controller before returning the lifetime-safe reference.
                 * Because presentation above observes that closed state first, command callbacks may now
                 * rebuild menus, alter Widgets, or request application exit without re-entering transient
                 * popup state. Any Widget changes become part of the next normal synchronization pass.
                 */
                pending_menu_command = {};
                (void)command->execute();
            } else {
                pending_menu_command = {};
            }

            /*
             * Backend polling is deliberately non-blocking. This small sleep bounds idle CPU usage while
             * keeping the demo responsive and allows TerminalEventPump's incomplete-sequence timeout to
             * advance. A later wait/wakeup abstraction can replace polling without changing decoding,
             * menu semantics, or widget semantics.
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
