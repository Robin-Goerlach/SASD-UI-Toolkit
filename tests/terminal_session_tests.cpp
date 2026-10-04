#include "test_framework.hpp"

#include <sasd/ui/terminal/presentation_frame.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/terminal_session.hpp>
#include <sasd/ui/terminal/testing/mock_terminal_device.hpp>

#include <stdexcept>
#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;
using sasd::ui::terminal::testing::MockTerminalDevice;

TEST_CASE("TerminalSession begins once and restores device on destruction") {
    MockTerminalDevice device;

    {
        TerminalSession session{device};

        CHECK(session.active());
        CHECK(device.active());
        CHECK(device.beginCount() == 1);
        CHECK(device.endCount() == 0);
        CHECK(device.lastOptions() == TerminalSessionOptions{});
    }

    CHECK(!device.active());
    CHECK(device.endCount() == 1);
}

TEST_CASE("TerminalSession leaves pointer reporting opt-in and silent by default") {
    MockTerminalDevice device;

    TerminalSession session{device};

    CHECK(session.active());
    CHECK(!session.pointerInputEnabled());
    CHECK(device.writeCount() == 0);
    CHECK(device.writes().empty());
}

TEST_CASE("TerminalSession owns SGR drag pointer reporting for its RAII lifetime") {
    MockTerminalDevice device;
    TerminalSessionOptions options;
    options.pointer_input = true;

    {
        TerminalSession session{device, options};

        CHECK(session.active());
        CHECK(session.pointerInputEnabled());
        CHECK(device.active());
        CHECK(device.lastOptions() == options);
        CHECK(device.writeCount() == 1);
        CHECK(device.writes().size() == 1);

        /*
         * DECSET 1002 is intentionally used instead of all-motion 1003: click-and-drag selection needs
         * movement while a button is held, not a permanent stream of hover reports. DECSET 1006 then
         * selects the SGR coordinate representation understood by AnsiInputDecoder.
         */
        CHECK(device.writes()[0] == std::string{"\x1B[?1002h\x1B[?1006h"});
    }

    CHECK(!device.active());
    CHECK(device.endCount() == 1);
    CHECK(device.writeCount() == 2);
    CHECK(device.writes().size() == 2);

    /*
     * Protocol teardown happens before native endSession(), while the device is still writable and VT
     * output is still configured. Reversing the enable order also avoids leaving SGR encoding selected
     * after button-event tracking has been released.
     */
    CHECK(device.writes()[1] == std::string{"\x1B[?1006l\x1B[?1002l"});
}

TEST_CASE("TerminalSession rolls native state back when pointer reporting activation fails") {
    MockTerminalDevice device;
    device.setFailWrite(true);

    TerminalSessionOptions options;
    options.pointer_input = true;

    bool threw = false;
    try {
        TerminalSession session{device, options};
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(!device.active());
    CHECK(device.beginCount() == 1);
    CHECK(device.endCount() == 1);

    /*
     * One write attempts activation and the second is the constructor's best-effort inverse sequence.
     * The mock fails both writes before recording bytes, which is useful here: endSession() must still
     * execute even when protocol recovery itself cannot reach the terminal.
     */
    CHECK(device.writeCount() == 2);
    CHECK(device.writes().empty());
}

TEST_CASE("TerminalSession still restores native state when pointer shutdown write fails") {
    MockTerminalDevice device;
    TerminalSessionOptions options;
    options.pointer_input = true;

    TerminalSession session{device, options};
    CHECK(session.pointerInputEnabled());
    CHECK(device.writes().size() == 1);

    device.setFailWrite(true);
    session.close();

    CHECK(!session.active());
    CHECK(!session.pointerInputEnabled());
    CHECK(!device.active());
    CHECK(device.endCount() == 1);
    CHECK(device.writeCount() == 2);
    CHECK(device.writes().size() == 1);

    // close() remains idempotent after a failed best-effort pointer-protocol teardown.
    session.close();
    CHECK(device.endCount() == 1);
    CHECK(device.writeCount() == 2);
}

TEST_CASE("TerminalSession rejects non-interactive device before native mutation") {
    MockTerminalDevice device;
    device.setInteractive(false);

    bool threw = false;
    try {
        TerminalSession session{device};
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(device.beginCount() == 0);
    CHECK(device.endCount() == 0);
}

TEST_CASE("TerminalSession does not teardown when beginSession fails transactionally") {
    MockTerminalDevice device;
    device.setFailBegin(true);

    bool threw = false;
    try {
        TerminalSession session{device};
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(device.beginCount() == 1);
    CHECK(device.endCount() == 0);
    CHECK(!device.active());
}

TEST_CASE("TerminalSession delegates visible cell size only while active") {
    MockTerminalDevice device;
    device.setSize({132, 43});

    TerminalSession session{device};
    CHECK(session.size() == Size{132, 43});

    session.close();

    bool threw = false;
    try {
        (void)session.size();
    } catch (const std::logic_error&) {
        threw = true;
    }
    CHECK(threw);
}

TEST_CASE("TerminalSession presents ScreenBuffer as one ANSI byte write") {
    MockTerminalDevice device;
    TerminalSession session{device};

    ScreenBuffer buffer{{2, 1}};
    buffer.set({0, 0}, Cell{U'A'});
    buffer.set({1, 0}, Cell{U'B'});

    session.present(buffer, Point{1, 0});

    CHECK(device.writeCount() == 1);
    CHECK(device.writes().size() == 1);

    const std::string& bytes = device.writes().front();
    CHECK(bytes.find("AB") != std::string::npos);
    CHECK(bytes.ends_with("\x1B[1;2H\x1B[?25h"));
}

TEST_CASE("TerminalSession presents complete TerminalPresentationFrame through the same transport path") {
    MockTerminalDevice device;
    TerminalSession session{device};

    TerminalPresentationFrame frame{ScreenBuffer{{2, 1}}, Point{1, 0}};
    frame.buffer.set({0, 0}, Cell{U'A'});
    frame.buffer.set({1, 0}, Cell{U'B'});

    session.present(frame);

    CHECK(device.writeCount() == 1);
    CHECK(device.writes().size() == 1);

    const std::string& bytes = device.writes().front();
    CHECK(bytes.find("AB") != std::string::npos);
    CHECK(bytes.ends_with("\x1B[1;2H\x1B[?25h"));

    /*
     * Closing the session and retrying through the frame overload must inherit the primitive overload's
     * active-session guard. This protects the one-way delegation contract from accidentally growing an
     * independent transport path with different lifetime semantics later.
     */
    session.close();

    bool threw = false;
    try {
        session.present(frame);
    } catch (const std::logic_error&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(device.writeCount() == 1);
}

TEST_CASE("TerminalSession transport failure leaves native session active for retry or close") {
    MockTerminalDevice device;
    TerminalSession session{device};

    ScreenBuffer buffer{{1, 1}};
    device.setFailWrite(true);

    bool threw = false;
    try {
        session.present(buffer);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(session.active());
    CHECK(device.active());

    device.setFailWrite(false);
    session.present(buffer);
    CHECK(device.writeCount() == 2);
}

TEST_CASE("TerminalSession explicit close is idempotent") {
    MockTerminalDevice device;
    TerminalSession session{device};

    session.close();
    session.close();

    CHECK(!session.active());
    CHECK(!device.active());
    CHECK(device.endCount() == 1);

    bool threw = false;
    try {
        ScreenBuffer buffer{{1, 1}};
        session.present(buffer);
    } catch (const std::logic_error&) {
        threw = true;
    }
    CHECK(threw);
}

TEST_CASE("TerminalSession forwards explicit session options") {
    MockTerminalDevice device;
    TerminalSessionOptions options;
    options.alternate_screen = false;
    options.raw_input = false;

    TerminalSession session{device, options};

    CHECK(device.lastOptions() == options);
}

TEST_CASE("TerminalSession polls currently available input bytes without decoding") {
    MockTerminalDevice device;
    TerminalSession session{device};

    device.queueInput("\x1B[A");
    device.queueInput("abc");

    CHECK(session.pollInputBytes() == std::string{"\x1B[A"});
    CHECK(session.pollInputBytes() == "abc");
    CHECK(session.pollInputBytes().empty());
    CHECK(device.readCount() == 3);
}

TEST_CASE("TerminalSession input failure leaves session active") {
    MockTerminalDevice device;
    TerminalSession session{device};
    device.setFailRead(true);

    bool threw = false;
    try {
        (void)session.pollInputBytes();
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(session.active());
    CHECK(device.active());
}

TEST_CASE("TerminalSession rejects input polling after close") {
    MockTerminalDevice device;
    TerminalSession session{device};
    session.close();

    bool threw = false;
    try {
        (void)session.pollInputBytes();
    } catch (const std::logic_error&) {
        threw = true;
    }

    CHECK(threw);
}
