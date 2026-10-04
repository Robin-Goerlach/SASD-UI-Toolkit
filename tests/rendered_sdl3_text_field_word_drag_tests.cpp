#include "test_framework.hpp"

#include "rendered/sdl3/sdl3_window_backend.hpp"

#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/rendered/rendered_text_field_hit_test.hpp>
#include <sasd/ui/rendered/rendered_text_field_pointer_selection.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <SDL3/SDL.h>

#include <cstddef>
#include <optional>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::rendered;
using namespace sasd::ui::rendered::sdl3;

#ifndef SASD_UI_SDL3_TEST_FONT_PATH
#error "SASD_UI_SDL3_TEST_FONT_PATH must be defined for SDL3 adapter tests"
#endif

namespace {

[[nodiscard]] Sdl3WindowBackendConfig wordDragTestConfig() {
    return {
        "SASD UI Toolkit SDL3 word-drag CI",
        {320, 180},
        SASD_UI_SDL3_TEST_FONT_PATH,
        16.0F,
        true,  // hidden: exercise a real SDL window without mapping it during CI.
        true,
        true,
        Color::black,
    };
}

void drainEvents(Sdl3WindowBackend& backend) {
    while (backend.pollEvent().has_value()) {
    }
}

/**
 * Appends one exact native SDL event without asking the platform mouse driver to synthesize state.
 *
 * SDL_PushEvent() invokes global event filters/watchers and can cause an offscreen video driver to
 * reject or reconcile a synthetic button transition against its own mouse state. SDL_ADDEVENT keeps
 * this test focused on the production queue-consumption boundary instead: Sdl3WindowBackend still
 * receives the event through SDL_PollEvent(), performs real window filtering and logical-coordinate
 * conversion, and invokes the same pointer translator as the interactive desktop demo.
 */
void enqueueNativeEvent(SDL_Event event) {
    CHECK(SDL_PeepEvents(
              &event,
              1,
              SDL_ADDEVENT,
              SDL_EVENT_FIRST,
              SDL_EVENT_LAST) == 1);
}

/**
 * Resolves the real hidden SDL window id owned by the backend under test.
 *
 * Mouse coordinate conversion is window/renderer aware. Supplying the actual id makes the injected
 * event faithful to the production path and also verifies that Sdl3WindowBackend accepts only events
 * belonging to its own top-level surface.
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
 * Polls the public backend until the native queue is empty or one semantic PointerEvent arrives.
 *
 * Hidden SDL windows can produce unrelated lifecycle/expose notifications. Skipping those here keeps
 * the assertion about the injected mouse event deterministic while still exercising the production
 * Sdl3WindowBackend::pollEvent() loop rather than calling the private translator directly.
 */
[[nodiscard]] std::optional<PointerEvent> pollPointerEvent(
    Sdl3WindowBackend& backend) {
    while (const auto event = backend.pollEvent()) {
        if (const auto* pointer = std::get_if<PointerEvent>(&*event);
            pointer != nullptr) {
            return *pointer;
        }
    }

    return std::nullopt;
}

/**
 * Finds a stable logical point inside the painted span of one Unicode scalar.
 *
 * The SDL3 test font is supplied by CI and its advances are deliberately not hard-coded. Scanning the
 * TextField's logical width through the same RenderedTextFieldHitTest used by production interaction
 * lets this integration test remain valid across font versions and platforms while still targeting a
 * particular semantic scalar. The field is a direct child of an origin-zero Window, so its arranged
 * bounds are also the logical coordinates expected by the SDL host.
 */
[[nodiscard]] std::optional<Point> pointInsideScalar(
    const TextField& field,
    std::size_t scalar_index,
    const RenderedMeasurementContext& metrics) {
    const Rect bounds = field.bounds();
    if (bounds.isEmpty()) {
        return std::nullopt;
    }

    const Coordinate y =
        static_cast<Coordinate>(bounds.y + bounds.height / 2);

    for (Coordinate offset = 0; offset < bounds.width; ++offset) {
        const Point candidate{
            static_cast<Coordinate>(bounds.x + offset),
            y};

        if (RenderedTextFieldHitTest::scalarIndexAt(
                field,
                candidate,
                metrics) == std::optional<std::size_t>{scalar_index}) {
            return candidate;
        }
    }

    return std::nullopt;
}

} // namespace

TEST_CASE("SDL3 native double-click drag extends TextField selection by complete words end to end") {
    Sdl3WindowBackend backend{wordDragTestConfig()};
    backend.initialize();
    drainEvents(backend);

    DisplayList display;
    RenderedPresentationSink sink{display, backend, Color::black};
    FocusManager focus;
    PointerRouter pointer_router;
    RenderedTextFieldPointerSelection::GestureState gesture_state;

    Window window;
    const Size size = backend.windowSize();
    window.arrange({0, 0, size.width, size.height});

    auto& field = window.emplace<TextField>("alpha beta gamma");
    field.arrange({20, 20, 280, 32});

    /*
     * Start the viewport at scalar zero so every word is initially representable. The field is wide
     * enough for this short string, but making the initial cursor explicit prevents a future default-
     * cursor policy change from turning this adapter regression into an accidental scrolling test.
     */
    field.setCursorPosition(0);
    CHECK(focus.requestFocus(field));

    CHECK(PresentationCoordinator::replay(window, sink).complete());
    CHECK(backend.presentFrame(display) == display.size());

    /*
     * Pick interior painted scalars rather than guessing pixel advances. Scalar 7 is inside "beta"
     * ([6,10)); scalar 12 is inside "gamma" ([11,16)). Word boundaries themselves remain semantic
     * Core data, while the actual points come from the real SDL_ttf-backed measurement context.
     */
    const auto beta_point = pointInsideScalar(field, 7, backend);
    const auto gamma_point = pointInsideScalar(field, 12, backend);
    CHECK(beta_point.has_value());
    CHECK(gamma_point.has_value());
    if (!beta_point.has_value() || !gamma_point.has_value()) {
        return;
    }

    const SDL_WindowID window_id = currentTestWindowId();
    if (window_id == 0) {
        return;
    }

    SDL_Event down{};
    down.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    down.button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    down.button.windowID = window_id;
    down.button.button = SDL_BUTTON_LEFT;
    down.button.clicks = 2;
    down.button.x = static_cast<float>(beta_point->x);
    down.button.y = static_cast<float>(beta_point->y);
    enqueueNativeEvent(down);

    const auto pressed = pollPointerEvent(backend);
    CHECK(pressed.has_value());
    if (!pressed.has_value()) {
        return;
    }

    CHECK(pressed->action == PointerAction::press);
    CHECK(pressed->button == PointerButton::primary);
    CHECK(pressed->click_count == 2);
    CHECK(pressed->modifiers == KeyModifier::none);

    /*
     * Mirror the real desktop demo's host policy: focus-on-primary-press is outside the selection
     * helper, while rendered word geometry and PointerRouter capture are handled by the shared M4
     * interaction seam. The second-click count comes from the SDL event all the way through this path.
     */
    Widget* hit = HitTest::deepestAt(window, pressed->position);
    CHECK(hit == &field);
    if (hit != nullptr && hit->canReceiveFocus()) {
        CHECK(focus.requestFocus(*hit));
    }

    const PointerRouteResult press_result =
        RenderedTextFieldPointerSelection::route(
            window,
            pointer_router,
            *pressed,
            backend,
            gesture_state);

    CHECK(press_result.targeted);
    CHECK(press_result.handled);
    CHECK(press_result.capture_active);
    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(gesture_state.hasWordGesture());
    CHECK(field.selectionAnchor() == 6);
    CHECK(field.cursorPosition() == 10);
    CHECK(field.selectedText() == "beta");

    SDL_Event motion{};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = window_id;
    motion.motion.x = static_cast<float>(gamma_point->x);
    motion.motion.y = static_cast<float>(gamma_point->y);
    enqueueNativeEvent(motion);

    const auto moved = pollPointerEvent(backend);
    CHECK(moved.has_value());
    if (!moved.has_value()) {
        return;
    }

    CHECK(moved->action == PointerAction::move);
    CHECK(moved->button == PointerButton::none);

    const PointerRouteResult move_result =
        RenderedTextFieldPointerSelection::route(
            window,
            pointer_router,
            *moved,
            backend,
            gesture_state);

    CHECK(move_result.targeted);
    CHECK(move_result.handled);
    CHECK(move_result.capture_active);
    CHECK(gesture_state.hasWordGesture());
    CHECK(field.selectionAnchor() == 6);
    CHECK(field.cursorPosition() == 16);
    CHECK(field.selectionStart() == 6);
    CHECK(field.selectionEnd() == 16);
    CHECK(field.selectedText() == "beta gamma");

    SDL_Event up{};
    up.type = SDL_EVENT_MOUSE_BUTTON_UP;
    up.button.type = SDL_EVENT_MOUSE_BUTTON_UP;
    up.button.windowID = window_id;
    up.button.button = SDL_BUTTON_LEFT;
    up.button.clicks = 2;
    up.button.x = static_cast<float>(gamma_point->x);
    up.button.y = static_cast<float>(gamma_point->y);
    enqueueNativeEvent(up);

    const auto released = pollPointerEvent(backend);
    CHECK(released.has_value());
    if (!released.has_value()) {
        return;
    }

    CHECK(released->action == PointerAction::release);
    CHECK(released->button == PointerButton::primary);

    const PointerRouteResult release_result =
        RenderedTextFieldPointerSelection::route(
            window,
            pointer_router,
            *released,
            backend,
            gesture_state);

    /*
     * Matching release retires both ownership channels through their normal APIs: PointerRouter drops
     * capture (calling TextField::onPointerCaptureLost() as needed) and GestureState forgets the word
     * origin. The semantic selection itself must survive because it belongs to TextField, not either
     * transient gesture mechanism.
     */
    CHECK(release_result.targeted);
    CHECK(release_result.handled);
    CHECK(!release_result.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(!gesture_state.hasWordGesture());
    CHECK(field.selectionAnchor() == 6);
    CHECK(field.cursorPosition() == 16);
    CHECK(field.selectedText() == "beta gamma");

    /*
     * Finish through the real presentation path as well. Existing rendered unit tests inspect the
     * exact selection display-list commands; this adapter regression instead proves that the semantic
     * result produced from native SDL events can be replayed and executed by the actual SDL renderer.
     */
    display.clear();
    CHECK(PresentationCoordinator::replay(window, sink).complete());
    CHECK(display.size() > 0);
    CHECK(backend.presentFrame(display) == display.size());
}
