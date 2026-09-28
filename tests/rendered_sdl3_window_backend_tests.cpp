#include "test_framework.hpp"

#include "rendered/sdl3/sdl3_window_backend.hpp"

#include <sasd/ui/application.hpp>
#include <sasd/ui/rendered/display_list.hpp>

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
