#include <sasd/ui/application.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/focus_traversal.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/radio_group_navigation.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/terminal_backend.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

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
 * Re-measures and arranges the simple demo form for the current terminal size.
 *
 * Window itself intentionally has no built-in layout policy yet, so the sample makes the root-form
 * arrangement explicit. Keeping this visible in example code is preferable to inventing implicit
 * Window behavior before more than one backend has validated the desired semantics.
 */
void layoutForm(Window& window,
                VBox& form,
                const TerminalMeasurementContext& metrics,
                Size terminal_size) {
    window.arrange({0, 0, terminal_size.width, terminal_size.height});

    /*
     * Give the form the complete screen as its parent constraints. VBox computes natural child
     * heights and stretches each visible child across the available terminal width.
     */
    (void)form.measure(metrics, {{0, 0}, terminal_size});
    form.arrange({0, 0, terminal_size.width, terminal_size.height});
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

        auto& greet = form.emplace<Button>("Greet");
        TextStyle greet_style;
        greet_style.foreground = Color::bright_green;
        greet_style.bold = true;
        greet.setTextStyle(greet_style);

        auto& status = form.emplace<Label>(
            "Tab moves focus. Space toggles/selects controls. F1 help.");
        TextStyle status_style;
        status_style.foreground = Color::yellow;
        status.setTextStyle(status_style);

        auto& exit = form.emplace<Button>("Exit");
        TextStyle exit_style;
        exit_style.foreground = Color::bright_red;
        exit.setTextStyle(exit_style);

        FocusManager focus;

        greet.setOnActivated([&] {
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

        exit.setOnActivated([&] {
            application.requestExit();
        });

        layoutForm(window, form, metrics, initial_size);
        (void)focus.requestFocus(name);

        /*
         * Initial full presentation. TerminalPresentationSink writes only into ScreenBuffer;
         * TerminalSession performs the final ANSI/VT byte transport to the real console.
         */
        const auto initial_pass =
            PresentationCoordinator::synchronize(window, presentation);
        if (!initial_pass.complete()) {
            throw std::runtime_error(
                "terminal demo contains presentation state the current Cell model cannot render");
        }
        backend.session().present(screen, presentation.caretPosition());

        while (!application.exitRequested()) {
            bool resized = false;

            (void)application.processRoutedEvents(
                [&](const Event& event) -> Widget* {
                    if (std::holds_alternative<KeyEvent>(event) ||
                        std::holds_alternative<TextInputEvent>(event)) {
                        return focus.focusedWidget();
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
                             * Function keys are ordinary semantic KeyEvents. The sample deliberately
                             * handles F1 at application scope instead of baking Help behavior into the
                             * terminal backend or a Widget base class.
                             */
                            status.setText(
                                "Help: Tab moves focus; Space toggles/selects; Arrow keys move within RadioGroup; Enter/Space activates Buttons.");
                            return;
                        }

                        if (key->key == Key::f10 || key->key == Key::escape) {
                            application.requestExit();
                        }
                    }
                });

            if (application.exitRequested()) {
                break;
            }

            /*
             * Content changes such as the greeting Label can invalidate natural sizes without a
             * terminal resize. Re-measure only when required; cursor/focus-only changes stay visual.
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

            if (pass.requested != 0 || resized) {
                backend.session().present(screen, presentation.caretPosition());
            }

            /*
             * Backend polling is deliberately non-blocking. This small sleep bounds idle CPU usage
             * while keeping the first demo responsive and allows TerminalEventPump's 30 ms incomplete
             * sequence timeout to advance. A later wait/wakeup abstraction can replace polling without
             * changing decoding or widget semantics.
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
