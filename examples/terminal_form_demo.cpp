#include <sasd/ui/application.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/focus_traversal.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
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

        Window window;
        auto& form = window.emplace<VBox>();
        form.setSpacing(1);

        form.emplace<Label>("SASD UI Toolkit - Terminal M2 Demo");
        form.emplace<Label>("Name:");
        auto& name = form.emplace<TextField>();

        auto& greet = form.emplace<Button>("Greet");
        auto& status = form.emplace<Label>(
            "Tab/Shift+Tab changes focus. Escape exits.");
        auto& exit = form.emplace<Button>("Exit");

        FocusManager focus;

        greet.setOnActivated([&] {
            std::string value{name.text()};
            if (value.empty()) {
                value = "world";
            }

            /*
             * Label::setText() invalidates measurement and presentation. The main loop below notices
             * that the form measurement became stale and performs a fresh layout before repainting.
             */
            status.setText("Hello, " + value + "!");
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

                    if (FocusTraversal::handleEvent(focus, form, event) ==
                        EventResult::handled) {
                        return;
                    }

                    if (const auto* key = std::get_if<KeyEvent>(&event);
                        key != nullptr &&
                        key->pressed &&
                        key->key == Key::escape &&
                        key->modifiers == KeyModifier::none) {
                        application.requestExit();
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
