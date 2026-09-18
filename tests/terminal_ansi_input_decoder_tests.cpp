#include "test_framework.hpp"

#include <sasd/ui/terminal/ansi_input_decoder.hpp>

#include <string>
#include <variant>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

namespace {

const KeyEvent& keyAt(const std::vector<Event>& events, std::size_t index) {
    return std::get<KeyEvent>(events.at(index));
}

const TextInputEvent& textAt(const std::vector<Event>& events, std::size_t index) {
    return std::get<TextInputEvent>(events.at(index));
}

} // namespace

TEST_CASE("AnsiInputDecoder emits UTF-8 text input without byte splitting assumptions") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(std::string{"A\xCE\xA9\xE7\x95\x8C"});

    CHECK(events.size() == 1);
    CHECK(textAt(events, 0).text == std::string{"A\xCE\xA9\xE7\x95\x8C"});
    CHECK(!decoder.hasPendingInput());
}

TEST_CASE("AnsiInputDecoder holds incomplete UTF-8 scalar across device reads") {
    AnsiInputDecoder decoder;

    const auto first = decoder.feed(std::string{"\xE7\x95"});
    CHECK(first.empty());
    CHECK(decoder.hasPendingInput());

    const auto second = decoder.feed(std::string{"\x8C"});
    CHECK(second.size() == 1);
    CHECK(textAt(second, 0).text == std::string{"\xE7\x95\x8C"});
    CHECK(!decoder.hasPendingInput());
}

TEST_CASE("AnsiInputDecoder flush replaces incomplete UTF-8 bytes deterministically") {
    AnsiInputDecoder decoder;
    CHECK(decoder.feed(std::string{"\xE7"}).empty());

    const auto events = decoder.flushPending();

    CHECK(events.size() == 1);
    CHECK(textAt(events, 0).text == std::string{"\xEF\xBF\xBD"});
    CHECK(!decoder.hasPendingInput());
}

TEST_CASE("AnsiInputDecoder translates Enter Tab and Backspace controls") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(std::string{"\r\t\x7F"});

    CHECK(events.size() == 3);
    CHECK(keyAt(events, 0).key == Key::enter);
    CHECK(keyAt(events, 1).key == Key::tab);
    CHECK(keyAt(events, 2).key == Key::backspace);
}

TEST_CASE("AnsiInputDecoder collapses CRLF into one Enter") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed("\r\n");

    CHECK(events.size() == 1);
    CHECK(keyAt(events, 0).key == Key::enter);
}

TEST_CASE("AnsiInputDecoder emits Space as key intent plus text input") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(" ");

    CHECK(events.size() == 2);
    CHECK(keyAt(events, 0).key == Key::space);
    CHECK(keyAt(events, 0).modifiers == KeyModifier::none);
    CHECK(textAt(events, 1).text == " ");
}

TEST_CASE("AnsiInputDecoder decodes CSI arrows even when sequence is split") {
    AnsiInputDecoder decoder;

    CHECK(decoder.feed("\x1B").empty());
    CHECK(decoder.feed("[").empty());

    const auto events = decoder.feed("A");

    CHECK(events.size() == 1);
    CHECK(keyAt(events, 0).key == Key::up);
    CHECK(!decoder.hasPendingInput());
}

TEST_CASE("AnsiInputDecoder decodes navigation and editing CSI tilde keys") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(
        "\x1B[H"
        "\x1B[F"
        "\x1B[3~"
        "\x1B[5~"
        "\x1B[6~");

    CHECK(events.size() == 5);
    CHECK(keyAt(events, 0).key == Key::home);
    CHECK(keyAt(events, 1).key == Key::end);
    CHECK(keyAt(events, 2).key == Key::delete_forward);
    CHECK(keyAt(events, 3).key == Key::page_up);
    CHECK(keyAt(events, 4).key == Key::page_down);
}

TEST_CASE("AnsiInputDecoder decodes Shift Tab and xterm key modifiers") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(
        "\x1B[Z"
        "\x1B[1;5D"
        "\x1B[1;2C");

    CHECK(events.size() == 3);

    CHECK(keyAt(events, 0).key == Key::tab);
    CHECK(keyAt(events, 0).modifiers == KeyModifier::shift);

    CHECK(keyAt(events, 1).key == Key::left);
    CHECK(keyAt(events, 1).modifiers == KeyModifier::control);

    CHECK(keyAt(events, 2).key == Key::right);
    CHECK(keyAt(events, 2).modifiers == KeyModifier::shift);
}

TEST_CASE("AnsiInputDecoder supports SS3 cursor sequences") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(
        "\x1BOA"
        "\x1BOF");

    CHECK(events.size() == 2);
    CHECK(keyAt(events, 0).key == Key::up);
    CHECK(keyAt(events, 1).key == Key::end);
}

TEST_CASE("AnsiInputDecoder buffers lone Escape until explicit flush") {
    AnsiInputDecoder decoder;

    CHECK(decoder.feed("\x1B").empty());
    CHECK(decoder.hasPendingInput());

    const auto events = decoder.flushPending();

    CHECK(events.size() == 1);
    CHECK(keyAt(events, 0).key == Key::escape);
    CHECK(!decoder.hasPendingInput());
}

TEST_CASE("AnsiInputDecoder consumes unknown complete CSI atomically") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed("\x1B[999X");

    CHECK(events.empty());
    CHECK(!decoder.hasPendingInput());
}

TEST_CASE("AnsiInputDecoder treats non-sequence ESC prefix as Escape then text") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed("\x1Bx");

    CHECK(events.size() == 2);
    CHECK(keyAt(events, 0).key == Key::escape);
    CHECK(textAt(events, 1).text == "x");
}

TEST_CASE("AnsiInputDecoder reset discards partial sequence state") {
    AnsiInputDecoder decoder;
    CHECK(decoder.feed("\x1B[").empty());

    decoder.reset();

    CHECK(!decoder.hasPendingInput());
    CHECK(decoder.feed("A").size() == 1);
}
