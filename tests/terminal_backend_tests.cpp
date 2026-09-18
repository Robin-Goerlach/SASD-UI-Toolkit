#include "test_framework.hpp"

#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/terminal_backend.hpp>
#include <sasd/ui/terminal/testing/mock_terminal_device.hpp>

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
