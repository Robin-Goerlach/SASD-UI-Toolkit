#include "test_framework.hpp"

#include <sasd/ui/terminal/terminal_event_pump.hpp>
#include <sasd/ui/terminal/testing/mock_terminal_device.hpp>

#include <chrono>
#include <string>
#include <variant>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::terminal;
using sasd::ui::terminal::testing::MockTerminalDevice;
using namespace std::chrono_literals;

namespace {

/**
 * Queues one complete primary-button click in xterm SGR-1006 syntax.
 *
 * The supplied coordinates are intentionally protocol coordinates (one-based terminal cells), making
 * each test explicit about the exact byte stream TerminalEventPump must enrich. Press and release are
 * placed in one device read because the protocol itself carries no timestamps; poll(now) is the
 * observation boundary used by the click synthesizer.
 */
void queuePrimaryClick(MockTerminalDevice& device, std::string_view coordinates = "4;2") {
    device.queueInput(
        std::string{"\x1B[<0;"} + std::string{coordinates} + "M" +
        "\x1B[<0;" + std::string{coordinates} + "m");
}

const PointerEvent& pointerAt(const std::vector<Event>& events, std::size_t index) {
    return std::get<PointerEvent>(events.at(index));
}

} // namespace

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

TEST_CASE("TerminalEventPump synthesizes single double and triple click counts from completed SGR clicks") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPumpOptions options;
    options.multi_click_interval = 400ms;
    TerminalEventPump pump{session, options};

    const auto start = TerminalEventPump::TimePoint{};

    queuePrimaryClick(device);
    const auto first = pump.poll(start);
    CHECK(first.size() == 2);
    CHECK(pointerAt(first, 0).action == PointerAction::press);
    CHECK(pointerAt(first, 0).position == Point{3, 1});
    CHECK(pointerAt(first, 0).click_count == 1);
    CHECK(pointerAt(first, 1).action == PointerAction::release);
    CHECK(pointerAt(first, 1).click_count == 1);

    queuePrimaryClick(device);
    const auto second = pump.poll(start + 100ms);
    CHECK(second.size() == 2);
    CHECK(pointerAt(second, 0).click_count == 2);
    CHECK(pointerAt(second, 1).click_count == 2);

    queuePrimaryClick(device);
    const auto third = pump.poll(start + 200ms);
    CHECK(third.size() == 2);
    CHECK(pointerAt(third, 0).click_count == 3);
    CHECK(pointerAt(third, 1).click_count == 3);

    /*
     * Current terminal interaction semantics distinguish one, two and three clicks. Keep a rapid fourth
     * transition saturated at three rather than wrapping uint8_t or exposing an otherwise undefined count.
     */
    queuePrimaryClick(device);
    const auto fourth = pump.poll(start + 300ms);
    CHECK(fourth.size() == 2);
    CHECK(pointerAt(fourth, 0).click_count == 3);
    CHECK(pointerAt(fourth, 1).click_count == 3);
}

TEST_CASE("TerminalEventPump starts a new click chain after timeout or cell change") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPumpOptions options;
    options.multi_click_interval = 250ms;
    TerminalEventPump pump{session, options};

    const auto start = TerminalEventPump::TimePoint{};
    queuePrimaryClick(device);
    CHECK(pointerAt(pump.poll(start), 0).click_count == 1);

    /* A new press after the configured release-to-press interval starts again at one. */
    queuePrimaryClick(device);
    CHECK(pointerAt(pump.poll(start + 251ms), 0).click_count == 1);

    /*
     * A fast click in another terminal cell also starts a new chain. Terminal geometry is discrete, so
     * there is deliberately no hidden desktop-pixel tolerance around the previous cell.
     */
    queuePrimaryClick(device, "5;2");
    const auto moved_cell = pump.poll(start + 300ms);
    CHECK(pointerAt(moved_cell, 0).position == Point{4, 1});
    CHECK(pointerAt(moved_cell, 0).click_count == 1);
}

TEST_CASE("TerminalEventPump drag motion prevents the gesture from seeding a later double click") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPump pump{session};

    const auto start = TerminalEventPump::TimePoint{};

    device.queueInput("\x1B[<0;4;2M");
    const auto press = pump.poll(start);
    CHECK(press.size() == 1);
    CHECK(pointerAt(press, 0).click_count == 1);

    /* SGR code 32 is primary-button motion. Moving to a different cell marks this gesture as a drag. */
    device.queueInput("\x1B[<32;5;2M");
    const auto move = pump.poll(start + 10ms);
    CHECK(move.size() == 1);
    CHECK(pointerAt(move, 0).action == PointerAction::move);
    CHECK(pointerAt(move, 0).click_count == 0);

    device.queueInput("\x1B[<0;5;2m");
    const auto release = pump.poll(start + 20ms);
    CHECK(release.size() == 1);
    CHECK(pointerAt(release, 0).click_count == 1);

    /* Even a fast click back at the original cell is single because the preceding gesture was a drag. */
    queuePrimaryClick(device);
    const auto after_drag = pump.poll(start + 30ms);
    CHECK(pointerAt(after_drag, 0).click_count == 1);
}

TEST_CASE("TerminalEventPump input reset also resets remembered multi click state") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPump pump{session};

    const auto start = TerminalEventPump::TimePoint{};
    queuePrimaryClick(device);
    CHECK(pointerAt(pump.poll(start), 0).click_count == 1);

    pump.resetInput();

    queuePrimaryClick(device);
    const auto after_reset = pump.poll(start + 10ms);
    CHECK(pointerAt(after_reset, 0).click_count == 1);
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

TEST_CASE("TerminalEventPump rejects negative multi click interval") {
    MockTerminalDevice device;
    TerminalSession session{device};
    TerminalEventPumpOptions options;
    options.multi_click_interval = -1ms;

    bool threw = false;
    try {
        TerminalEventPump pump{session, options};
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
}
