#include "rendered/sdl3/sdl3_window_backend.hpp"

#include <sasd/ui/application.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/focus_traversal.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <variant>

using namespace std::chrono_literals;

namespace {

using namespace sasd::ui;
using namespace sasd::ui::rendered;
using namespace sasd::ui::rendered::sdl3;

/**
 * Measures/arranges the same small semantic form used by the terminal demo.
 *
 * The only presentation-specific input is the MeasurementContext implemented by the SDL3 backend.
 * Window/VBox/Button/TextField remain the same Core widget classes; there is no parallel SDL widget
 * hierarchy hidden in this example.
 */
void layoutForm(Window& window,
                VBox& form,
                const RenderedMeasurementContext& metrics,
                Size window_size) {
    window.arrange({0, 0, window_size.width, window_size.height});

    (void)form.measure(metrics, {{0, 0}, window_size});
    form.arrange({0, 0, window_size.width, window_size.height});
}

/**
 * Mirrors logical widget focus into SDL's text-input/IME activation boundary.
 *
 * SDL3 intentionally leaves text input disabled until requested. Keeping this policy in the desktop
 * host/example is preferable to teaching FocusManager about SDL or making Backend assume every
 * focused control accepts text.
 */
void synchronizeTextInput(Sdl3WindowBackend& backend, const FocusManager& focus) {
    const auto* field = dynamic_cast<const TextField*>(focus.focusedWidget());
    const bool should_enable =
        field != nullptr && field->isVisible() && field->isEnabled();

    backend.setTextInputEnabled(should_enable);
}

/**
 * Builds and presents a complete rendered frame.
 *
 * RenderedPresentationSink supports incremental synchronization, but an SDL window back buffer is not
 * treated as persistent state after SDL_RenderPresent(). Until M3 grows an explicit retained backing
 * store/dirty-region policy, the window adapter therefore asks the existing conservative subtree
 * invalidation mechanism for a complete frame. This is intentionally correctness-first.
 */
void presentFullFrame(Window& window,
                      DisplayList& display_list,
                      RenderedPresentationSink& presentation,
                      Sdl3WindowBackend& backend) {
    display_list.clear();
    window.invalidatePresentationSubtree();

    const auto pass = PresentationCoordinator::synchronize(window, presentation);
    if (!pass.complete()) {
        throw std::runtime_error{
            "SDL3 demo contains presentation state the current rendered model cannot represent"};
    }

    (void)backend.presentFrame(display_list);
}

[[nodiscard]] std::string fontPathFromArguments(int argc, char** argv) {
    if (argc >= 2 && argv[1] != nullptr && argv[1][0] != '\0') {
        return argv[1];
    }

    if (const char* environment = std::getenv("SASD_UI_FONT");
        environment != nullptr && environment[0] != '\0') {
        return environment;
    }

    return {};
}

} // namespace

int main(int argc, char** argv) {
    try {
        using namespace sasd::ui;
        using namespace sasd::ui::rendered;
        using namespace sasd::ui::rendered::sdl3;

        const std::string font_path = fontPathFromArguments(argc, argv);
        if (font_path.empty()) {
            std::cerr
                << "Usage: sasd_ui_sdl3_demo <path-to-font.ttf>\n"
                << "   or: set SASD_UI_FONT to a TrueType/OpenType font file.\n";
            return EXIT_FAILURE;
        }

        Sdl3WindowBackendConfig backend_config;
        backend_config.title = "SASD UI Toolkit - Rendered M3 SDL3 Demo";
        backend_config.initial_size = {800, 500};
        backend_config.font_path = font_path;
        backend_config.font_point_size = 18.0F;
        backend_config.hidden = false;
        backend_config.resizable = true;
        backend_config.high_pixel_density = true;
        backend_config.background_color = Color::black;

        Sdl3WindowBackend backend{std::move(backend_config)};
        Application application{backend};

        DisplayList display_list;
        RenderedPresentationSink presentation{
            display_list,
            backend,
            Color::black};

        Window window;
        auto& form = window.emplace<VBox>();
        form.setSpacing(8);

        auto& title = form.emplace<Label>("SASD UI Toolkit - Rendered M3 SDL3 Demo");
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

        auto& greet = form.emplace<Button>("Greet");
        TextStyle greet_style;
        greet_style.foreground = Color::bright_green;
        greet_style.bold = true;
        greet.setTextStyle(greet_style);

        auto& status = form.emplace<Label>(
            "Tab/Shift+Tab focus. F1 help. F10/Escape exits.");
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
            status.setText("Hello, " + value + "!");
        });

        exit.setOnActivated([&] {
            application.requestExit();
        });

        layoutForm(window, form, backend, backend.windowSize());
        (void)focus.requestFocus(name);
        synchronizeTextInput(backend, focus);
        presentFullFrame(window, display_list, presentation, backend);

        while (!application.exitRequested()) {
            (void)application.processRoutedEvents(
                [&](const Event& event) -> Widget* {
                    if (std::holds_alternative<KeyEvent>(event) ||
                        std::holds_alternative<TextInputEvent>(event)) {
                        return focus.focusedWidget();
                    }

                    // Resize and other backend-level events are handled outside widget routing.
                    return nullptr;
                },
                [&](const Event& event) {
                    if (const auto* resize = std::get_if<ResizeEvent>(&event)) {
                        layoutForm(window, form, backend, resize->size);
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
                            status.setText(
                                "Help: type a name, Tab to Greet/Exit, Enter or Space activates.");
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
             * Focus traversal may have moved from TextField to Button (or back). Apply text-input
             * activation after the whole event batch so SDL IME state mirrors the final logical focus.
             */
            synchronizeTextInput(backend, focus);

            if (!form.isMeasureValid()) {
                layoutForm(window, form, backend, backend.windowSize());
            }

            /*
             * A semantic visual change makes Window pending. Expose/scale events can require repaint
             * even when every Widget is semantically clean, which is why the backend also exposes its
             * presentationRequested() bit.
             */
            if (window.isVisualUpdatePending() || backend.presentationRequested()) {
                presentFullFrame(window, display_list, presentation, backend);
            }

            /*
             * The generic Backend polling contract is deliberately non-blocking. Keep the first
             * desktop demo aligned with the terminal demo's simple loop; a later Application::run()
             * wait/wakeup abstraction can replace this sleep for both without changing event semantics.
             */
            std::this_thread::sleep_for(8ms);
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "SASD UI SDL3 demo failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
