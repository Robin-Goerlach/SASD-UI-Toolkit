#include "test_framework.hpp"

#include "rendered/sdl3/sdl3_pointer_event_translation.hpp"
#include "rendered/sdl3/sdl3_window_backend.hpp"

#include <sasd/ui/application.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/rendered/rendered_text_field_hit_test.hpp>
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
    /*
     * Use the queue primitive directly for adapter translation tests.
     *
     * SDL_PushEvent() intentionally runs global event filters/watchers first. Those hooks are part of
     * SDL's own input-state machinery and may reject a synthetic mouse-button transition that did
     * not originate from the platform mouse driver. Here we are not testing SDL's mouse state; we
     * are testing how Sdl3WindowBackend translates an SDL_Event already present in the queue.
     *
     * SDL_ADDEVENT appends the exact event without invoking those filters, making the test
     * deterministic while still exercising the production SDL_PollEvent() path.
     */
    CHECK(SDL_PeepEvents(
              &event,
              1,
              SDL_ADDEVENT,
              SDL_EVENT_FIRST,
              SDL_EVENT_LAST) == 1);
}

/**
 * Returns the SDL id of the one window owned by the current backend test.
 *
 * SDL_ConvertEventToRenderCoordinates() intentionally consults the event's window id before applying
 * renderer logical-presentation transforms. Supplying zero happened to work for motion in the first
 * test draft but is not a faithful native event. Resolve the actual hidden test window instead of
 * weakening production code to accommodate a synthetic id SDL itself would not normally generate.
 */
[[nodiscard]] SDL_WindowID currentTestWindowId() {
    int count = 0;
    SDL_Window** windows = SDL_GetWindows(&count);

    CHECK(windows != nullptr);
    CHECK(count == 1);

    if (windows == nullptr || count != 1) {
        if (windows != nullptr) {
            SDL_free(windows);
        }
        return 0;
    }

    const SDL_WindowID id = SDL_GetWindowID(windows[0]);
    SDL_free(windows);

    CHECK(id != 0);
    return id;
}

/**
 * Returns the SDL renderer owned by the one hidden window in the current adapter test.
 *
 * This is intentionally a test-only native seam. Production code keeps SDL handles private; the
 * regression below needs temporary access only to pre-seed and inspect renderer clip state around a
 * public Sdl3WindowBackend::drawText() call.
 */
[[nodiscard]] SDL_Renderer* currentTestRenderer() {
    int count = 0;
    SDL_Window** windows = SDL_GetWindows(&count);

    CHECK(windows != nullptr);
    CHECK(count == 1);

    if (windows == nullptr || count != 1) {
        if (windows != nullptr) {
            SDL_free(windows);
        }
        return nullptr;
    }

    SDL_Renderer* renderer = SDL_GetRenderer(windows[0]);
    SDL_free(windows);

    CHECK(renderer != nullptr);
    return renderer;
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

TEST_CASE("SDL3 text command restores the renderer clip state it temporarily replaces") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    SDL_Renderer* renderer = currentTestRenderer();
    CHECK(renderer != nullptr);
    if (renderer == nullptr) {
        return;
    }

    /*
     * Pretend a future outer render pass already owns a native clip. DrawTextCommand has its own
     * command-local clipping contract and may temporarily replace this rectangle, but it must not
     * destroy renderer state that it did not create.
     */
    const SDL_Rect outer_clip{7, 9, 240, 120};
    CHECK(SDL_SetRenderClipRect(renderer, &outer_clip));
    CHECK(SDL_RenderClipEnabled(renderer));

    DrawTextCommand command;
    command.origin = {0, 0};
    command.text = "Scoped clip";
    command.style.foreground = Color::bright_white;
    command.clip_bounds = Rect{20, 0, 40, 24};

    backend.drawText(command);

    CHECK(SDL_RenderClipEnabled(renderer));

    SDL_Rect restored{};
    CHECK(SDL_GetRenderClipRect(renderer, &restored));
    CHECK(restored.x == outer_clip.x);
    CHECK(restored.y == outer_clip.y);
    CHECK(restored.w == outer_clip.w);
    CHECK(restored.h == outer_clip.h);

    // Leave the native renderer in the default state expected by the rest of this isolated test.
    CHECK(SDL_SetRenderClipRect(renderer, nullptr));
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

TEST_CASE("SDL3 pointer click moves TextField cursor and replay emits matching caret") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    DisplayList display;
    RenderedPresentationSink sink{display, backend, Color::black};
    FocusManager focus;
    PointerRouter pointer_router;

    Window window;
    const Size size = backend.windowSize();
    window.arrange({0, 0, size.width, size.height});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({12, 48, 180, 32});
    field.setCursorPosition(3);

    const auto first_advance =
        backend.textAdvanceToScalar(field.text(), 1);
    CHECK(first_advance.has_value());

    if (!first_advance.has_value()) {
        return;
    }

    /*
     * Build the native button event at a known shaped scalar boundary. The private SDL translator is
     * the same deterministic semantic mapping used by Sdl3WindowBackend after native window/logical
     * coordinate conversion; using it directly avoids depending on SDL's offscreen mouse-driver
     * state while still beginning this integration test with an SDL_Event.
     */
    SDL_Event native{};
    native.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    native.button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    native.button.button = SDL_BUTTON_LEFT;
    native.button.clicks = 1;
    native.button.x =
        static_cast<float>(13 + *first_advance); // one-unit border => content starts at x=13.
    native.button.y = 56.0F;

    const auto pointer =
        detail::translateLogicalPointerEvent(native);
    CHECK(pointer.has_value());

    if (!pointer.has_value()) {
        return;
    }

    /*
     * Mirror the desktop host policy used by the runnable SDL3 demo. Focus selection and rendered
     * caret placement intentionally remain outside PointerRouter: the router owns target/capture
     * mechanics, while font-dependent TextField caret geometry belongs to the Rendered layer.
     */
    Widget* hit = HitTest::deepestAt(window, pointer->position);
    CHECK(hit == &field);
    CHECK(hit != nullptr);

    if (hit == nullptr) {
        return;
    }

    CHECK(focus.requestFocus(*hit));

    const auto scalar =
        RenderedTextFieldHitTest::caretIndexAt(
            field,
            pointer->position,
            backend);
    CHECK(scalar == std::optional<std::size_t>{1});

    if (!scalar.has_value()) {
        return;
    }

    field.setCursorPosition(*scalar);

    const PointerRouteResult routed =
        pointer_router.route(window, *pointer);

    /*
     * TextField currently has no pointer gesture of its own, so the press is geometrically targeted
     * but intentionally not captured/handled. Caret placement is host/rendered policy above.
     */
    CHECK(routed.targeted);
    CHECK(!routed.handled);
    CHECK(!routed.capture_active);

    display.clear();
    const auto replay =
        PresentationCoordinator::replay(window, sink);

    CHECK(replay.complete());
    CHECK(field.cursorPosition() == 1);

    const auto expected_caret_x =
        static_cast<Coordinate>(13 + *first_advance);

    bool found_caret = false;
    for (const auto& command : display.commands()) {
        if (const auto* fill = std::get_if<FillRectCommand>(&command);
            fill != nullptr &&
            fill->role == FillRole::foreground &&
            fill->bounds.x == expected_caret_x) {
            CHECK(fill->bounds.y == 49);
            CHECK(fill->bounds.width == 1);
            CHECK(fill->bounds.height > 0);
            found_caret = true;
        }
    }

    CHECK(found_caret);
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

TEST_CASE("SDL3 window backend reports native pointer surface enter and leave") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    SDL_Event enter{};
    enter.type = SDL_EVENT_WINDOW_MOUSE_ENTER;
    enter.window.type = SDL_EVENT_WINDOW_MOUSE_ENTER;
    enter.window.windowID = currentTestWindowId();
    pushEvent(enter);

    const auto entered = backend.pollEvent();
    CHECK(entered.has_value());
    CHECK(std::holds_alternative<PointerSurfaceEvent>(*entered));
    CHECK(std::get<PointerSurfaceEvent>(*entered).action ==
          PointerSurfaceAction::entered);

    SDL_Event leave{};
    leave.type = SDL_EVENT_WINDOW_MOUSE_LEAVE;
    leave.window.type = SDL_EVENT_WINDOW_MOUSE_LEAVE;
    leave.window.windowID = currentTestWindowId();
    pushEvent(leave);

    const auto left = backend.pollEvent();
    CHECK(left.has_value());
    CHECK(std::holds_alternative<PointerSurfaceEvent>(*left));
    CHECK(std::get<PointerSurfaceEvent>(*left).action ==
          PointerSurfaceAction::left);
}

TEST_CASE("SDL3 pointer surface leave can retire Core hover and capture without coordinates") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    PointerRouter pointer_router;
    Window window;
    window.arrange({0, 0, 320, 180});

    auto& button = window.emplace<Button>("Leave");
    button.arrange({10, 10, 80, 30});

    CHECK(pointer_router.route(
        window,
        PointerEvent{{20, 20}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(button.isPointerOver());
    CHECK(button.isPressed());
    CHECK(pointer_router.hasCapture());

    SDL_Event leave{};
    leave.type = SDL_EVENT_WINDOW_MOUSE_LEAVE;
    leave.window.type = SDL_EVENT_WINDOW_MOUSE_LEAVE;
    leave.window.windowID = currentTestWindowId();
    pushEvent(leave);

    const auto translated = backend.pollEvent();
    CHECK(translated.has_value());
    CHECK(std::holds_alternative<PointerSurfaceEvent>(*translated));

    if (const auto* surface = std::get_if<PointerSurfaceEvent>(&*translated);
        surface != nullptr && surface->action == PointerSurfaceAction::left) {
        pointer_router.leaveRoot();
    }

    CHECK(!button.isPointerOver());
    CHECK(!button.isPressed());
    CHECK(!pointer_router.hasHover());
    CHECK(!pointer_router.hasCapture());
}

TEST_CASE("SDL3 window backend exposes logical pointer motion through native queue") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    CHECK(backend.capabilities().pointer_input);

    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = currentTestWindowId();
    motion.motion.x = 42.75F;
    motion.motion.y = 18.25F;
    pushEvent(motion);

    const auto translated = backend.pollEvent();
    CHECK(translated.has_value());
    CHECK(std::holds_alternative<PointerEvent>(*translated));

    const auto& pointer = std::get<PointerEvent>(*translated);
    CHECK(pointer.action == PointerAction::move);
    CHECK(pointer.button == PointerButton::none);
    CHECK(pointer.position == Point{42, 18});
    CHECK(pointer.click_count == 0);
}

TEST_CASE("SDL3 pointer translator maps button transitions without platform mouse state") {
    /*
     * SDL's offscreen mouse driver is free to reject synthetic button transitions while pumping
     * native state. Test SASD's deterministic mapping at the private adapter seam instead: pollEvent
     * uses this exact function after window filtering and coordinate conversion.
     */
    SDL_Event down{};
    down.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    down.button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    down.button.button = SDL_BUTTON_LEFT;
    down.button.clicks = 2;
    down.button.x = 75.9F;
    down.button.y = 30.1F;

    const auto pressed = detail::translateLogicalPointerEvent(down);
    CHECK(pressed.has_value());
    CHECK(pressed->action == PointerAction::press);
    CHECK(pressed->button == PointerButton::primary);
    CHECK(pressed->position == Point{75, 30});
    CHECK(pressed->click_count == 2);

    SDL_Event up = down;
    up.type = SDL_EVENT_MOUSE_BUTTON_UP;
    up.button.type = SDL_EVENT_MOUSE_BUTTON_UP;
    up.button.button = SDL_BUTTON_RIGHT;
    up.button.clicks = 1;

    const auto released = detail::translateLogicalPointerEvent(up);
    CHECK(released.has_value());
    CHECK(released->action == PointerAction::release);
    CHECK(released->button == PointerButton::secondary);
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

TEST_CASE("SDL3 pixel-size change requests repaint without changing logical layout or metrics") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    DisplayList frame;
    const Size logical_before = backend.windowSize();
    const std::uint64_t revision_before = backend.revision();
    frame.fillRect({0, 0, logical_before.width, logical_before.height}, Color::black);
    (void)backend.presentFrame(frame);
    CHECK(!backend.presentationRequested());

    SDL_Event native{};
    native.type = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
    native.window.type = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
    native.window.windowID = 0;
    native.window.data1 = 640;
    native.window.data2 = 360;
    pushEvent(native);

    /*
     * Physical back-buffer changes are deliberately presentation-only. pollEvent() consumes the
     * native notification, requests a replay and emits no semantic ResizeEvent because Widget layout
     * still lives in logical window coordinates.
     */
    CHECK(!backend.pollEvent().has_value());
    CHECK(backend.presentationRequested());
    CHECK(backend.windowSize() == logical_before);
    CHECK(backend.revision() == revision_before);
}

TEST_CASE("SDL3 display-scale change requests repaint without changing logical layout or metrics") {
    Sdl3WindowBackend backend{testConfig()};
    backend.initialize();
    drainEvents(backend);

    DisplayList frame;
    const Size logical_before = backend.windowSize();
    const std::uint64_t revision_before = backend.revision();
    frame.fillRect({0, 0, logical_before.width, logical_before.height}, Color::black);
    (void)backend.presentFrame(frame);
    CHECK(!backend.presentationRequested());

    SDL_Event native{};
    native.type = SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED;
    native.window.type = SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED;
    native.window.windowID = 0;
    pushEvent(native);

    /*
     * A monitor/DPI transition may change physical pixel density, but the STRETCH logical
     * presentation keeps SASD geometry and font metrics expressed in the same logical coordinate
     * system. Repainting is required; semantic re-layout and measurement invalidation are not.
     */
    CHECK(!backend.pollEvent().has_value());
    CHECK(backend.presentationRequested());
    CHECK(backend.windowSize() == logical_before);
    CHECK(backend.revision() == revision_before);
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
