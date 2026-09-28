#include "test_framework.hpp"

#include "rendered/sdl3/sdl3_window_backend.hpp"

#include <sasd/ui/application.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <SDL3/SDL.h>

#include <cstddef>
#include <optional>
#include <string>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::rendered;
using namespace sasd::ui::rendered::sdl3;

#ifndef SASD_UI_SDL3_TEST_FONT_PATH
#error "SASD_UI_SDL3_TEST_FONT_PATH must be defined for SDL3 adapter tests"
#endif

namespace {

[[nodiscard]] Sdl3WindowBackendConfig testConfig() {
    return {
        "SASD UI Toolkit SDL3 CI",
        {320, 180},
        SASD_UI_SDL3_TEST_FONT_PATH,
        16.0F,
        true,  // hidden: exercise a real SDL_Window without mapping it during automated tests.
        true,
        true,
        Color::black,
    };
}

void drainEvents(Sdl3WindowBackend& backend) {
    while (backend.pollEvent().has_value()) {
    }
}

void pushEvent(SDL_Event event) {
    CHECK(SDL_PushEvent(&event));
}

} // namespace

TEST_CASE("SDL3 window backend initializes transactionally and presents a complete frame") {
    Sdl3WindowBackend backend{testConfig()};

    CHECK(!backend.isInitialized());

    backend.initialize();

    CHECK(backend.isInitialized());
    CHECK(backend.windowSize().width > 0);
    CHECK(backend.windowSize().height > 0);
    CHECK(backend.pixelSize().width > 0);
    CHECK(backend.pixelSize().height > 0);
    CHECK(backend.displayScale() > 0.0F);
    CHECK(backend.presentationRequested());

    DisplayList frame;
    const Size size = backend.windowSize();
    frame.fillRect({0, 0, size.width, size.height}, Color::black);
    frame.strokeRect({8, 8, 120, 40}, Color::bright_cyan, 1);

    TextStyle style;
    style.foreground = Color::bright_white;
    frame.drawText({12, 12}, "SDL3 window", style, Rect{12, 12, 100, 20});

    CHECK(backend.presentFrame(frame) == 3);
    CHECK(!backend.presentationRequested());

    backend.shutdown();
    CHECK(!backend.isInitialized());

    // shutdown() is intentionally idempotent for Application/destructor safety.
    backend.shutdown();
}

TEST_CASE("SDL3 window backend presents a real semantic rendered widget tree end to end") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    DisplayList display;
    RenderedPresentationSink sink{display, backend, Color::black};
    FocusManager focus;

    Window window;
    const Size size = backend.windowSize();
    window.arrange({0, 0, size.width, size.height});

    auto& label = window.emplace<Label>("Rendered through a real SDL3 window");
    label.arrange({12, 12, 240, 24});

    auto& field = window.emplace<TextField>("A\xCE\xA9");
    field.arrange({12, 48, 180, 32});
    CHECK(focus.requestFocus(field));

    /*
     * replay() is the important architectural part of this test. A window back buffer can need a
     * complete redraw independently from semantic Widget dirty state, so reconstruction must not
     * require application code to reach into Widget's protected invalidation machinery.
     */
    const auto first_replay = PresentationCoordinator::replay(window, sink);

    CHECK(first_replay.complete());
    CHECK(first_replay.requested == 3);
    CHECK(display.size() >= 6);

    const std::size_t first_command_count = display.size();
    CHECK(backend.presentFrame(display) == first_command_count);
    CHECK(!backend.presentationRequested());

    /*
     * Present the same now-clean tree again. This proves surface recovery is independent of normal
     * incremental invalidation: replay still walks every current visual Widget and produces a fresh
     * complete frame.
     */
    display.clear();
    const auto second_replay = PresentationCoordinator::replay(window, sink);

    CHECK(second_replay.complete());
    CHECK(second_replay.forced == 3);
    CHECK(display.size() > 0);
    CHECK(backend.presentFrame(display) == display.size());
}

TEST_CASE("SDL3 window backend maps key modifiers and key identity to semantic events") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    SDL_Event native{};
    native.type = SDL_EVENT_KEY_DOWN;
    native.key.type = SDL_EVENT_KEY_DOWN;
    native.key.windowID = 0;
    native.key.key = SDLK_TAB;
    native.key.mod = static_cast<SDL_Keymod>(SDL_KMOD_SHIFT | SDL_KMOD_CTRL);
    native.key.down = true;
    pushEvent(native);

    const auto translated = backend.pollEvent();

    CHECK(translated.has_value());
    CHECK(std::holds_alternative<KeyEvent>(*translated));

    const auto& key = std::get<KeyEvent>(*translated);
    CHECK(key.key == Key::tab);
    CHECK(key.pressed);
    CHECK(hasModifier(key.modifiers, KeyModifier::shift));
    CHECK(hasModifier(key.modifiers, KeyModifier::control));
    CHECK(!hasModifier(key.modifiers, KeyModifier::alt));
}

TEST_CASE("SDL3 window backend exposes logical pointer motion and button transitions") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    CHECK(backend.capabilities().pointer_input);

    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = 0;
    motion.motion.x = 42.75F;
    motion.motion.y = 18.25F;
    pushEvent(motion);

    const auto translated_motion = backend.pollEvent();
    CHECK(translated_motion.has_value());
    CHECK(std::holds_alternative<PointerEvent>(*translated_motion));

    const auto& pointer_motion = std::get<PointerEvent>(*translated_motion);
    CHECK(pointer_motion.action == PointerAction::move);
    CHECK(pointer_motion.button == PointerButton::none);
    CHECK(pointer_motion.position == Point{42, 18});
    CHECK(pointer_motion.click_count == 0);

    SDL_Event button{};
    button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    button.button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    button.button.windowID = 0;
    button.button.button = SDL_BUTTON_LEFT;
    button.button.down = true;
    button.button.clicks = 2;
    button.button.x = 75.9F;
    button.button.y = 30.1F;
    pushEvent(button);

    const auto translated_button = backend.pollEvent();
    CHECK(translated_button.has_value());
    CHECK(std::holds_alternative<PointerEvent>(*translated_button));

    const auto& pointer_button = std::get<PointerEvent>(*translated_button);
    CHECK(pointer_button.action == PointerAction::press);
    CHECK(pointer_button.button == PointerButton::primary);
    CHECK(pointer_button.position == Point{75, 30});
    CHECK(pointer_button.click_count == 2);
}

TEST_CASE("SDL3 window backend separates committed UTF-8 text from physical key input") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    backend.setTextInputEnabled(true);
    CHECK(backend.textInputEnabled());

    static const char text[] = "A\xCE\xA9";

    SDL_Event native{};
    native.type = SDL_EVENT_TEXT_INPUT;
    native.text.type = SDL_EVENT_TEXT_INPUT;
    native.text.windowID = 0;
    native.text.text = text;
    pushEvent(native);

    const auto translated = backend.pollEvent();

    CHECK(translated.has_value());
    CHECK(std::holds_alternative<TextInputEvent>(*translated));
    CHECK(std::get<TextInputEvent>(*translated).text == std::string{text});

    backend.setTextInputEnabled(false);
    CHECK(!backend.textInputEnabled());
}

TEST_CASE("SDL3 window backend turns native resize into logical resize and repaint request") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    DisplayList frame;
    const Size original = backend.windowSize();
    frame.fillRect({0, 0, original.width, original.height}, Color::black);
    (void)backend.presentFrame(frame);
    CHECK(!backend.presentationRequested());

    SDL_Event native{};
    native.type = SDL_EVENT_WINDOW_RESIZED;
    native.window.type = SDL_EVENT_WINDOW_RESIZED;
    native.window.windowID = 0;
    native.window.data1 = 420;
    native.window.data2 = 240;
    pushEvent(native);

    const auto translated = backend.pollEvent();

    CHECK(translated.has_value());
    CHECK(std::holds_alternative<ResizeEvent>(*translated));
    CHECK(std::get<ResizeEvent>(*translated).size == Size{420, 240});
    CHECK(backend.windowSize() == Size{420, 240});
    CHECK(backend.presentationRequested());
}

TEST_CASE("SDL3 window expose requests repaint without inventing a semantic widget event") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    DisplayList frame;
    const Size size = backend.windowSize();
    frame.fillRect({0, 0, size.width, size.height}, Color::black);
    (void)backend.presentFrame(frame);
    CHECK(!backend.presentationRequested());

    SDL_Event native{};
    native.type = SDL_EVENT_WINDOW_EXPOSED;
    native.window.type = SDL_EVENT_WINDOW_EXPOSED;
    native.window.windowID = 0;
    pushEvent(native);

    CHECK(!backend.pollEvent().has_value());
    CHECK(backend.presentationRequested());
}

TEST_CASE("SDL3 window close request becomes application-level QuitEvent") {
    Sdl3WindowBackend backend{testConfig()};
    Application application{backend};
    drainEvents(backend);

    SDL_Event native{};
    native.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    native.window.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    native.window.windowID = 0;
    pushEvent(native);

    CHECK(application.processEvents() == 1);
    CHECK(application.exitRequested());
}
