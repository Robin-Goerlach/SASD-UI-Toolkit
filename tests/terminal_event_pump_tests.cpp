#include "test_framework.hpp"

#include <sasd/ui/terminal/terminal_event_pump.hpp>
#include <sasd/ui/terminal/testing/mock_terminal_device.hpp>

#include <chrono>
#include <string>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::terminal;
using sasd::ui::terminal::testing::MockTerminalDevice;
using namespace std::chrono_literals;

TEST_CASE("TerminalEventPump captures initial size without emitting synthetic resize") {
    MockTerminalDevice device;
    device.setSize({100, 30});
    TerminalSession session{device};
    TerminalEventPump pump{session};

    const auto now = TerminalEventPump::TimePoint{};
    const auto events = pump.poll(now);

    CHECK(events.empty());
    CHECK(pump.lastKnownSize() == Size{100, 30});
}

TEST_CASE("TerminalEventPump emits ResizeEvent before input from same tick") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPump pump{session};

    device.setSize({120, 40});
    device.queueInput("x");

    const auto events = pump.poll(TerminalEventPump::TimePoint{});

    CHECK(events.size() == 2);
    CHECK(std::holds_alternative<ResizeEvent>(events[0]));
    CHECK(std::get<ResizeEvent>(events[0]).size == Size{120, 40});
    CHECK(std::holds_alternative<TextInputEvent>(events[1]));
    CHECK(std::get<TextInputEvent>(events[1]).text == "x");
}

TEST_CASE("TerminalEventPump does not repeat unchanged resize") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPump pump{session};

    device.setSize({90, 20});

    const auto first = pump.poll(TerminalEventPump::TimePoint{});
    const auto second = pump.poll(TerminalEventPump::TimePoint{} + 1ms);

    CHECK(first.size() == 1);
    CHECK(std::holds_alternative<ResizeEvent>(first[0]));
    CHECK(second.empty());
}

TEST_CASE("TerminalEventPump resolves lone Escape only after timeout") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPumpOptions options;
    options.incomplete_sequence_timeout = 25ms;
    TerminalEventPump pump{session, options};

    const auto start = TerminalEventPump::TimePoint{};
    device.queueInput("\x1B");

    CHECK(pump.poll(start).empty());
    CHECK(pump.poll(start + 24ms).empty());

    const auto expired = pump.poll(start + 25ms);
    CHECK(expired.size() == 1);
    CHECK(std::get<KeyEvent>(expired[0]).key == Key::escape);
}

TEST_CASE("TerminalEventPump extends timeout when a partial sequence makes progress") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPumpOptions options;
    options.incomplete_sequence_timeout = 25ms;
    TerminalEventPump pump{session, options};

    const auto start = TerminalEventPump::TimePoint{};
    device.queueInput("\x1B");
    CHECK(pump.poll(start).empty());

    device.queueInput("[");
    CHECK(pump.poll(start + 20ms).empty());

    // Timeout is measured from the most recent sequence progress, not the original ESC byte.
    CHECK(pump.poll(start + 44ms).empty());

    device.queueInput("A");
    const auto completed = pump.poll(start + 45ms);

    CHECK(completed.size() == 1);
    CHECK(std::get<KeyEvent>(completed[0]).key == Key::up);
}

TEST_CASE("TerminalEventPump timeout flushes truncated UTF-8 instead of buffering forever") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPumpOptions options;
    options.incomplete_sequence_timeout = 10ms;
    TerminalEventPump pump{session, options};

    const auto start = TerminalEventPump::TimePoint{};
    device.queueInput(std::string{"\xE7"});
    CHECK(pump.poll(start).empty());

    const auto flushed = pump.poll(start + 10ms);
    CHECK(flushed.size() == 1);
    CHECK(std::get<TextInputEvent>(flushed[0]).text == std::string{"\xEF\xBF\xBD"});
}

TEST_CASE("TerminalEventPump rejects negative sequence timeout") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPumpOptions options;
    options.incomplete_sequence_timeout = -1ms;

    bool threw = false;
    try {
        TerminalEventPump pump{session, options};
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
}
