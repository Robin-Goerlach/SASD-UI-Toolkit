#include "rendered/sdl3/sdl3_window_backend.hpp"

#include <sasd/ui/application.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/check_box.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/focus_traversal.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/radio_group_navigation.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/rendered/rendered_text_field_hit_test.hpp>
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
 * treated as persistent state after SDL_RenderPresent(). PresentationCoordinator::replay() therefore
 * reconstructs the complete current visual tree without pretending that the semantic Widgets became
 * dirty merely because the native presentation surface needs a new frame. This is intentionally
 * correctness-first until M3 grows an explicit retained backing-store/dirty-region policy.
 */
void presentFullFrame(Window& window,
                      DisplayList& display_list,
                      RenderedPresentationSink& presentation,
                      Sdl3WindowBackend& backend) {
    display_list.clear();

    const auto pass = PresentationCoordinator::replay(window, presentation);
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

#if defined(_WIN32)
    /*
     * MSVC deliberately diagnoses std::getenv() as C4996. The demo is built with /W4 and can be
     * built with /WX, so suppressing that diagnostic globally would weaken the warning policy for
     * unrelated code. Use Microsoft's ownership-explicit environment API locally instead.
     *
     * _dupenv_s() allocates the returned buffer with malloc-compatible storage. Copy into std::string
     * before releasing it so no environment pointer/lifetime leaks into the rest of the demo.
     */
    char* environment = nullptr;
    std::size_t environment_size = 0;

    if (_dupenv_s(&environment, &environment_size, "SASD_UI_FONT") == 0) {
        std::string value;

        if (environment != nullptr) {
            value.assign(environment);
            std::free(environment);
        }

        if (!value.empty()) {
            return value;
        }
    } else if (environment != nullptr) {
        /*
         * Defensive cleanup in case a CRT implementation ever reports failure after assigning a
         * buffer. Current MSVC documentation normally leaves it null on failure.
         */
        std::free(environment);
    }
#else
    if (const char* environment = std::getenv("SASD_UI_FONT");
        environment != nullptr && environment[0] != '\0') {
        return environment;
    }
#endif

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
        backend_config.title = "SASD UI Toolkit - Rendered SDL3 Demo";
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

        /*
         * RadioGroup is a non-visual semantic relationship object. Declare it before Window so normal
         * reverse local destruction destroys the Window (and therefore its RadioButtons) first. The
         * relationship code is lifetime-safe in either order, but this ordering also makes the demo's
         * intended ownership graph obvious: Window owns Widgets; RadioGroup owns none of them.
         */
        RadioGroup greeting_group;

        Window window;
        auto& form = window.emplace<VBox>();
        form.setSpacing(8);

        auto& title = form.emplace<Label>("SASD UI Toolkit - Rendered SDL3 Demo");
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
         * Keep the first M4 form control in the same semantic tree as the M2/M3 widgets. The example
         * does not construct an SDL-specific checkbox: Core owns checked state and interaction while
         * RenderedPresentationSink decides how that state becomes pixels.
         *
         * Starting checked preserves the demo's historical "Hello, name!" behavior until the user
         * deliberately changes the option.
         */
        auto& enthusiastic =
            form.emplace<CheckBox>("Enthusiastic greeting", true);
        TextStyle checkbox_style;
        checkbox_style.foreground = Color::bright_magenta;
        enthusiastic.setTextStyle(checkbox_style);

        /*
         * Two RadioButtons share one explicit semantic group. Their mutual exclusion therefore
         * survives future layout/reparenting changes and is not an accidental property of VBox.
         */
        auto& hello_style =
            form.emplace<RadioButton>(greeting_group, "Greeting word: Hello");
        auto& hi_style =
            form.emplace<RadioButton>(greeting_group, "Greeting word: Hi");

        TextStyle radio_style;
        radio_style.foreground = Color::bright_blue;
        hello_style.setTextStyle(radio_style);
        hi_style.setTextStyle(radio_style);

        /*
         * Establish the initial application choice programmatically. The first selection callback is
         * installed below, so startup does not manufacture a user-visible change notification.
         */
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
        PointerRouter pointer_router;

        greet.setOnActivated([&] {
            std::string value{name.text()};
            if (value.empty()) {
                value = "world";
            }

            /*
             * Application logic consumes the semantic checked value only. Whether the option was
             * toggled by keyboard, SDL pointer input or a future native peer is irrelevant here.
             */
            const char punctuation = enthusiastic.isChecked() ? '!' : '.';
            const std::string greeting_word =
                hi_style.isSelected() ? "Hi" : "Hello";
            status.setText(
                greeting_word + ", " + value + std::string(1, punctuation));
        });

        enthusiastic.setOnCheckedChanged([&](bool checked) {
            /*
             * The callback demonstrates that checked-state notification is backend-neutral. It also
             * gives the visible smoke test immediate feedback without coupling CheckBox to Label.
             */
            status.setText(
                checked
                    ? "Greeting style: enthusiastic (!)"
                    : "Greeting style: calm (.)");
        });

        hello_style.setOnSelected([&] {
            /*
             * RadioGroup has already retired the previous selection before this callback runs. The
             * application can therefore trust group state immediately and need not repair exclusivity.
             */
            status.setText("Greeting word selected: Hello");
        });

        hi_style.setOnSelected([&] {
            status.setText("Greeting word selected: Hi");
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

                    /*
                     * PointerRouter owns hit testing/capture and dispatches pointer events itself in
                     * the fallback handler below. Returning nullptr here prevents Application from
                     * performing a second ordinary dispatch of the same pointer input.
                     */
                    return nullptr;
                },
                [&](const Event& event) {
                    if (const auto* surface =
                            std::get_if<PointerSurfaceEvent>(&event)) {
                        /*
                         * Surface leave is host lifecycle, not a Widget event. Core cannot assume
                         * native mouse capture continues outside this SDL window, so retire hover and
                         * any active semantic capture conservatively. Enter needs no action: the next
                         * real PointerEvent rebuilds hover from trustworthy logical coordinates.
                         */
                        if (surface->action == PointerSurfaceAction::left) {
                            pointer_router.leaveRoot();
                        }
                        return;
                    }

                    if (const auto* pointer = std::get_if<PointerEvent>(&event)) {
                        /*
                         * Desktop focus-on-primary-press is host policy, not PointerRouter policy.
                         * Keep FocusManager independent from pointer mechanics while still providing
                         * familiar Button/TextField behavior in this concrete desktop demo.
                         */
                        if (pointer->action == PointerAction::press &&
                            pointer->button == PointerButton::primary) {
                            Widget* hit = HitTest::deepestAt(window, pointer->position);

                            if (hit != nullptr && hit->canReceiveFocus()) {
                                (void)focus.requestFocus(*hit);
                            }

                            /*
                             * TextField cursor placement is rendered-presentation geometry, not Core
                             * editing geometry. Use the same viewport/font metrics as painting; when
                             * a shaping boundary is not representable, keep the existing cursor
                             * instead of guessing.
                             */
                            if (auto* field = dynamic_cast<TextField*>(hit)) {
                                if (const auto scalar =
                                        RenderedTextFieldHitTest::caretIndexAt(
                                            *field,
                                            pointer->position,
                                            backend)) {
                                    field->setCursorPosition(*scalar);
                                }
                            }
                        }

                        (void)pointer_router.route(window, *pointer);
                        return;
                    }

                    if (const auto* resize = std::get_if<ResizeEvent>(&event)) {
                        layoutForm(window, form, backend, resize->size);

                        /*
                         * Present resize results immediately instead of waiting until
                         * processRoutedEvents() has drained the entire native event queue.
                         *
                         * During an interactive desktop resize SDL/Windows can enqueue many resize
                         * notifications while the old back buffer is still visible. Deferring the
                         * repaint until the end of that batch lets the window manager temporarily
                         * stretch or expose stale pixels, which is especially noticeable on borders
                         * and text. Rendering here keeps layout and the visible back buffer closely
                         * synchronized without teaching the backend-neutral Application class about
                         * frame scheduling.
                         *
                         * A minimized/native-zero-sized window has no useful drawable surface. Keep
                         * its semantic layout current, but wait for the next positive ResizeEvent
                         * before presenting again.
                         */
                        if (!resize->size.isEmpty()) {
                            presentFullFrame(
                                window,
                                display_list,
                                presentation,
                                backend);
                        }
                        return;
                    }

                    /*
                     * Radio-group arrow navigation is deliberately separate from generic Tab
                     * traversal. This host opts the current focus scope into the policy explicitly,
                     * preventing a semantic RadioGroup that spans windows from silently moving focus
                     * across top-level surfaces.
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
            if (!backend.windowSize().isEmpty() &&
                (window.isVisualUpdatePending() || backend.presentationRequested())) {
                /*
                 * Non-resize invalidation and expose/scale requests still use the normal end-of-batch
                 * presentation path. Resize itself is handled eagerly above to reduce live-resize
                 * stale-frame artifacts.
                 */
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
