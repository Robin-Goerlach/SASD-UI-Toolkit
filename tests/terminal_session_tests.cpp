#include "test_framework.hpp"

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
