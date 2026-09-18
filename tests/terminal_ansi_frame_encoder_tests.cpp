#include "test_framework.hpp"

#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/ansi_frame_encoder.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <optional>
#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("AnsiFrameEncoder emits deterministic full ASCII frame") {
    ScreenBuffer buffer{{2, 2}};
    buffer.set({0, 0}, Cell{U'A'});
    buffer.set({1, 0}, Cell{U'B'});
    buffer.set({0, 1}, Cell{U'C'});
    buffer.set({1, 1}, Cell{U'D'});

    const std::string encoded = AnsiFrameEncoder::encode(buffer);

    CHECK(encoded ==
          std::string{"\x1B[?25l\x1B[0m\x1B[2J"
                      "\x1B[1;1HAB"
                      "\x1B[2;1HCD"});
}

TEST_CASE("AnsiFrameEncoder emits UTF-8 wide glyph once and skips continuation cell") {
    ScreenBuffer buffer{{3, 1}};
    buffer.set({0, 0}, Cell{U'\u754C', CellRole::wide_lead});
    buffer.set({1, 0}, Cell{U' ', CellRole::wide_continuation});
    buffer.set({2, 0}, Cell{U'X'});

    const std::string encoded = AnsiFrameEncoder::encode(buffer);

    CHECK(encoded ==
          std::string{"\x1B[?25l\x1B[0m\x1B[2J"
                      "\x1B[1;1H\xE7\x95\x8C" "X"});
}

TEST_CASE("AnsiFrameEncoder restores a valid hardware caret after repaint") {
    ScreenBuffer buffer{{4, 2}};

    const std::string encoded = AnsiFrameEncoder::encode(buffer, Point{2, 1});

    CHECK(encoded.ends_with("\x1B[2;3H\x1B[?25h"));
}

TEST_CASE("AnsiFrameEncoder leaves cursor hidden for missing or invalid caret") {
    ScreenBuffer buffer{{2, 1}};

    const std::string without = AnsiFrameEncoder::encode(buffer);
    const std::string outside = AnsiFrameEncoder::encode(buffer, Point{2, 0});

    CHECK(without.find("\x1B[?25h") == std::string::npos);
    CHECK(outside.find("\x1B[?25h") == std::string::npos);
}

TEST_CASE("AnsiFrameEncoder handles empty frame deterministically") {
    ScreenBuffer buffer;

    CHECK(AnsiFrameEncoder::encode(buffer) ==
          std::string{"\x1B[?25l\x1B[0m\x1B[2J"});
}


TEST_CASE("Semantic TextField presentation encodes into ANSI frame with real caret request") {
    ScreenBuffer buffer{{12, 3}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 12, 3});

    auto& box = window.emplace<VBox>();
    auto& field = box.emplace<TextField>("abc");

    (void)box.measure(metrics, {{0, 0}, {12, 3}});
    box.arrange({0, 0, 6, 1});

    CHECK(focus.requestFocus(field));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(sink.caretPosition().has_value());

    const std::string encoded =
        AnsiFrameEncoder::encode(buffer, sink.caretPosition());

    CHECK(encoded.find(">abc <") != std::string::npos);
    CHECK(encoded.find("\x1B[?25h") != std::string::npos);
}


TEST_CASE("AnsiFrameEncoder emits SGR only when terminal cell style changes") {
    ScreenBuffer buffer{{3, 1}};

    TextStyle styled;
    styled.foreground = Color::bright_green;
    styled.bold = true;
    styled.underline = true;

    buffer.set({0, 0}, Cell{U'A', CellRole::normal, styled});
    buffer.set({1, 0}, Cell{U'B', CellRole::normal, styled});
    buffer.set({2, 0}, Cell{U'C'});

    const std::string encoded = AnsiFrameEncoder::encode(buffer);

    CHECK(encoded ==
          std::string{"\x1B[?25l\x1B[0m\x1B[2J"
                      "\x1B[1;1H"
                      "\x1B[0;92;1;4mAB"
                      "\x1B[0mC"});
}

TEST_CASE("AnsiFrameEncoder resets non-default style before returning control") {
    ScreenBuffer buffer{{1, 1}};

    TextStyle styled;
    styled.foreground = Color::red;
    styled.inverse = true;
    buffer.set({0, 0}, Cell{U'X', CellRole::normal, styled});

    const std::string encoded = AnsiFrameEncoder::encode(buffer);

    CHECK(encoded.ends_with("X\x1B[0m"));
}
