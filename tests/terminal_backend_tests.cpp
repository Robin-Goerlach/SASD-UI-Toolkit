#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/terminal_backend.hpp>
#include <sasd/ui/terminal/testing/mock_terminal_device.hpp>
#include <sasd/ui/window.hpp>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::terminal;
using sasd::ui::terminal::testing::MockTerminalDevice;
using namespace std::chrono_literals;

TEST_CASE("TerminalBackend owns TerminalSession lifecycle and restores injected device") {
    auto device = std::make_unique<MockTerminalDevice>();
    MockTerminalDevice* observed = device.get();

    TerminalBackend backend{std::move(device)};

    CHECK(!backend.isInitialized());
    backend.initialize();

    CHECK(backend.isInitialized());
    CHECK(observed->active());

    backend.shutdown();
    CHECK(!backend.isInitialized());
    CHECK(!observed->active());
    CHECK(observed->endCount() == 1);

    backend.shutdown();
    CHECK(observed->endCount() == 1);
}

TEST_CASE("TerminalBackend converts native bytes into semantic Backend events") {
    auto device = std::make_unique<MockTerminalDevice>();
    MockTerminalDevice* observed = device.get();

    TerminalEventPumpOptions options;
    options.incomplete_sequence_timeout = 0ms;

    TerminalBackend backend{
        std::move(device),
        TerminalSessionOptions{},
        options};
    backend.initialize();

    observed->queueInput("\tA");

    auto first = backend.pollEvent();
    auto second = backend.pollEvent();
    auto third = backend.pollEvent();

    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(std::holds_alternative<KeyEvent>(*first));
    CHECK(std::get<KeyEvent>(*first).key == Key::tab);
    CHECK(std::holds_alternative<TextInputEvent>(*second));
    CHECK(std::get<TextInputEvent>(*second).text == "A");
    CHECK(!third.has_value());
}

TEST_CASE("TerminalBackend SGR pointer events route through the Core PointerRouter") {
    auto device = std::make_unique<MockTerminalDevice>();
    MockTerminalDevice* observed = device.get();

    TerminalSessionOptions session_options;
    session_options.pointer_input = true;

    TerminalEventPumpOptions pump_options;
    pump_options.incomplete_sequence_timeout = 0ms;

    TerminalBackend backend{
        std::move(device),
        session_options,
        pump_options};
    backend.initialize();

    /*
     * Arrange a real Core Button in terminal-cell coordinates. The SGR reports below deliberately use
     * x=4,y=2 because the wire protocol is one-based; AnsiInputDecoder must therefore produce logical
     * point {3,1}, which lies inside this Button. No terminal-specific hit-testing adapter participates
     * after decoding: PointerRouter consumes the same PointerEvent contract used by desktop backends.
     */
    Window window;
    window.arrange({0, 0, 20, 4});
    auto& button = window.emplace<Button>("Run");
    button.arrange({2, 1, 8, 1});

    int activations = 0;
    button.setOnActivated([&] {
        ++activations;
    });

    PointerRouter pointer_router;

    observed->queueInput("\x1B[<0;4;2M");
    const auto pressed = backend.pollEvent();

    CHECK(pressed.has_value());
    CHECK(std::holds_alternative<PointerEvent>(*pressed));

    const PointerEvent& press = std::get<PointerEvent>(*pressed);
    CHECK(press.position == Point{3, 1});
    CHECK(press.action == PointerAction::press);
    CHECK(press.button == PointerButton::primary);

    const PointerRouteResult press_route = pointer_router.route(window, press);
    CHECK(press_route.handled);
    CHECK(pointer_router.hasCapture());
    CHECK(button.isPressed());

    /*
     * Lowercase 'm' is the SGR-1006 release final. Routing that decoded event through the existing
     * capture path must activate the Button exactly once and retire capture. This is the important
     * end-to-end contract: terminal transport/decoding stops at PointerEvent, while normal Core routing
     * owns interaction semantics from that point onward.
     */
    observed->queueInput("\x1B[<0;4;2m");
    const auto released = backend.pollEvent();

    CHECK(released.has_value());
    CHECK(std::holds_alternative<PointerEvent>(*released));

    const PointerEvent& release = std::get<PointerEvent>(*released);
    CHECK(release.position == Point{3, 1});
    CHECK(release.action == PointerAction::release);
    CHECK(release.button == PointerButton::primary);

    const PointerRouteResult release_route = pointer_router.route(window, release);
    CHECK(release_route.handled);
    CHECK(!pointer_router.hasCapture());
    CHECK(!button.isPressed());
    CHECK(activations == 1);
}

TEST_CASE("TerminalBackend produces resize event from changed native dimensions") {
    auto device = std::make_unique<MockTerminalDevice>();
    MockTerminalDevice* observed = device.get();

    TerminalBackend backend{std::move(device)};
    backend.initialize();

    CHECK(backend.terminalSize() == Size{80, 24});

    observed->setSize({132, 43});

    auto event = backend.pollEvent();
    CHECK(event.has_value());
    CHECK(std::holds_alternative<ResizeEvent>(*event));
    CHECK(std::get<ResizeEvent>(*event).size == Size{132, 43});
    CHECK(backend.terminalSize() == Size{132, 43});
}

TEST_CASE("TerminalBackend exposes active session for presentation transport") {
    auto device = std::make_unique<MockTerminalDevice>();
    MockTerminalDevice* observed = device.get();

    TerminalBackend backend{std::move(device)};
    backend.initialize();

    ScreenBuffer buffer{{1, 1}};
    buffer.set({0, 0}, Cell{U'X'});
    backend.session().present(buffer);

    CHECK(observed->writeCount() == 1);
    CHECK(observed->writes().front().find("X") != std::string::npos);
}

TEST_CASE("TerminalBackend rejects duplicate initialization") {
    auto device = std::make_unique<MockTerminalDevice>();
    TerminalBackend backend{std::move(device)};
    backend.initialize();

    bool threw = false;
    try {
        backend.initialize();
    } catch (const std::logic_error&) {
        threw = true;
    }

    CHECK(threw);
}
