#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/focus_traversal.hpp>
#include <sasd/ui/terminal/ansi_input_decoder.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>

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

const PointerEvent& pointerAt(const std::vector<Event>& events, std::size_t index) {
    return std::get<PointerEvent>(events.at(index));
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

TEST_CASE("AnsiInputDecoder translates unambiguous Ctrl-letter C0 bytes to key intent") {
    AnsiInputDecoder decoder;
    const auto events = decoder.feed("\x01\x03\x18\x16");

    CHECK(events.size() == 4);
    CHECK(keyAt(events, 0).key == Key::a);
    CHECK(keyAt(events, 1).key == Key::c);
    CHECK(keyAt(events, 2).key == Key::x);
    CHECK(keyAt(events, 3).key == Key::v);
    for (const Event& event : events) {
        CHECK(std::get<KeyEvent>(event).pressed);
        CHECK(std::get<KeyEvent>(event).modifiers == KeyModifier::control);
    }
}

TEST_CASE("AnsiInputDecoder preserves reserved C0 control semantics") {
    AnsiInputDecoder decoder;
    const auto events = decoder.feed("\x08\t\n\r");

    CHECK(events.size() == 4);
    CHECK(keyAt(events, 0).key == Key::backspace);
    CHECK(keyAt(events, 1).key == Key::tab);
    CHECK(keyAt(events, 2).key == Key::enter);
    CHECK(keyAt(events, 3).key == Key::enter);
    for (const Event& event : events) {
        CHECK(std::get<KeyEvent>(event).modifiers == KeyModifier::none);
    }
}

TEST_CASE("AnsiInputDecoder collapses CRLF into one Enter") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed("\r\n");

    CHECK(events.size() == 1);
    CHECK(keyAt(events, 0).key == Key::enter);
}

TEST_CASE("AnsiInputDecoder suppresses LF when CRLF is split across reads") {
    AnsiInputDecoder decoder;

    const auto first = decoder.feed("\r");
    CHECK(first.size() == 1);
    CHECK(keyAt(first, 0).key == Key::enter);

    const auto second = decoder.feed("\n");
    CHECK(second.empty());

    const auto text = decoder.feed("x");
    CHECK(text.size() == 1);
    CHECK(textAt(text, 0).text == "x");
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


TEST_CASE("ANSI bytes drive TextField focus traversal and Button activation end to end") {
    AnsiInputDecoder decoder;

    VBox root;
    auto& field = root.emplace<TextField>();
    auto& button = root.emplace<Button>("OK");

    FocusManager focus;
    int activations = 0;
    button.setOnActivated([&] { ++activations; });

    /*
     * Physical terminal bytes:
     *   Tab  -> enter focus sequence at TextField
     *   A    -> TextInputEvent inserts into TextField
     *   Tab  -> TextField ignores it, FocusTraversal moves to Button
     *   Space-> KeyEvent activates Button; accompanying TextInputEvent is ignored by Button
     */
    const auto events = decoder.feed("\tA\t ");

    for (const Event& event : events) {
        bool handled = false;

        if (Widget* target = focus.focusedWidget()) {
            handled = EventDispatcher::dispatch(*target, event).handled();
        }

        if (!handled) {
            handled = FocusTraversal::handleEvent(focus, root, event) ==
                      EventResult::handled;
        }

        (void)handled;
    }

    CHECK(field.text() == "A");
    CHECK(focus.focusedWidget() == &button);
    CHECK(button.hasFocus());
    CHECK(activations == 1);
}


TEST_CASE("AnsiInputDecoder decodes SS3 and legacy CSI function keys F1 through F12") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(
        "\x1BOP"   // F1: common SS3 form
        "\x1BOQ"   // F2
        "\x1BOR"   // F3
        "\x1BOS"   // F4
        "\x1B[15~" // F5
        "\x1B[17~" // F6
        "\x1B[18~" // F7
        "\x1B[19~" // F8
        "\x1B[20~" // F9
        "\x1B[21~" // F10
        "\x1B[23~" // F11
        "\x1B[24~" // F12
    );

    CHECK(events.size() == 12);
    CHECK(keyAt(events, 0).key == Key::f1);
    CHECK(keyAt(events, 1).key == Key::f2);
    CHECK(keyAt(events, 2).key == Key::f3);
    CHECK(keyAt(events, 3).key == Key::f4);
    CHECK(keyAt(events, 4).key == Key::f5);
    CHECK(keyAt(events, 5).key == Key::f6);
    CHECK(keyAt(events, 6).key == Key::f7);
    CHECK(keyAt(events, 7).key == Key::f8);
    CHECK(keyAt(events, 8).key == Key::f9);
    CHECK(keyAt(events, 9).key == Key::f10);
    CHECK(keyAt(events, 10).key == Key::f11);
    CHECK(keyAt(events, 11).key == Key::f12);
}

TEST_CASE("AnsiInputDecoder accepts legacy CSI tilde encodings for F1 through F4") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(
        "\x1B[11~"
        "\x1B[12~"
        "\x1B[13~"
        "\x1B[14~");

    CHECK(events.size() == 4);
    CHECK(keyAt(events, 0).key == Key::f1);
    CHECK(keyAt(events, 1).key == Key::f2);
    CHECK(keyAt(events, 2).key == Key::f3);
    CHECK(keyAt(events, 3).key == Key::f4);
}

TEST_CASE("AnsiInputDecoder preserves xterm modifiers on function keys") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(
        "\x1B[1;2P"  // Shift+F1
        "\x1B[1;5Q"  // Ctrl+F2
        "\x1B[15;3~" // Alt+F5
        "\x1B[24;6~" // Shift+Ctrl+F12
    );

    CHECK(events.size() == 4);

    CHECK(keyAt(events, 0).key == Key::f1);
    CHECK(keyAt(events, 0).modifiers == KeyModifier::shift);

    CHECK(keyAt(events, 1).key == Key::f2);
    CHECK(keyAt(events, 1).modifiers == KeyModifier::control);

    CHECK(keyAt(events, 2).key == Key::f5);
    CHECK(keyAt(events, 2).modifiers == KeyModifier::alt);

    CHECK(keyAt(events, 3).key == Key::f12);
    CHECK(keyAt(events, 3).modifiers ==
          (KeyModifier::shift | KeyModifier::control));
}

TEST_CASE("AnsiInputDecoder keeps split function-key escape sequences incremental") {
    AnsiInputDecoder decoder;

    CHECK(decoder.feed("\x1B").empty());
    CHECK(decoder.feed("[21").empty());

    const auto events = decoder.feed("~");

    CHECK(events.size() == 1);
    CHECK(keyAt(events, 0).key == Key::f10);
    CHECK(!decoder.hasPendingInput());
}

TEST_CASE("AnsiInputDecoder decodes SGR mouse press and release into zero-based pointer cells") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(
        "\x1B[<0;12;4M"
        "\x1B[<0;12;4m");

    CHECK(events.size() == 2);

    const PointerEvent& press = pointerAt(events, 0);
    CHECK(press.position == Point{11, 3});
    CHECK(press.action == PointerAction::press);
    CHECK(press.button == PointerButton::primary);
    CHECK(press.click_count == 1);
    CHECK(press.modifiers == KeyModifier::none);

    const PointerEvent& release = pointerAt(events, 1);
    CHECK(release.position == Point{11, 3});
    CHECK(release.action == PointerAction::release);
    CHECK(release.button == PointerButton::primary);
    CHECK(release.click_count == 1);
    CHECK(release.modifiers == KeyModifier::none);
}

TEST_CASE("AnsiInputDecoder maps SGR mouse buttons modifiers and motion without leaking held-button state") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(
        "\x1B[<29;3;2M"  // middle + Shift + Meta/Alt + Ctrl press
        "\x1B[<36;9;5M"  // motion + Shift, low button bits describe held primary
        "\x1B[<2;10;6M"); // secondary press

    CHECK(events.size() == 3);

    const PointerEvent& modified_press = pointerAt(events, 0);
    CHECK(modified_press.position == Point{2, 1});
    CHECK(modified_press.action == PointerAction::press);
    CHECK(modified_press.button == PointerButton::middle);
    CHECK(modified_press.modifiers ==
          (KeyModifier::shift | KeyModifier::alt | KeyModifier::control));

    const PointerEvent& motion = pointerAt(events, 1);
    CHECK(motion.position == Point{8, 4});
    CHECK(motion.action == PointerAction::move);
    CHECK(motion.button == PointerButton::none);
    CHECK(motion.click_count == 0);
    CHECK(motion.modifiers == KeyModifier::shift);

    const PointerEvent& secondary_press = pointerAt(events, 2);
    CHECK(secondary_press.position == Point{9, 5});
    CHECK(secondary_press.action == PointerAction::press);
    CHECK(secondary_press.button == PointerButton::secondary);
}

TEST_CASE("AnsiInputDecoder keeps split SGR mouse reports pending until the CSI final arrives") {
    AnsiInputDecoder decoder;

    CHECK(decoder.feed("\x1B[<0;20").empty());
    CHECK(decoder.hasPendingInput());

    const auto events = decoder.feed(";7M");

    CHECK(events.size() == 1);
    CHECK(pointerAt(events, 0).position == Point{19, 6});
    CHECK(pointerAt(events, 0).action == PointerAction::press);
    CHECK(pointerAt(events, 0).button == PointerButton::primary);
    CHECK(!decoder.hasPendingInput());
}

TEST_CASE("AnsiInputDecoder consumes unsupported or malformed SGR mouse reports atomically") {
    AnsiInputDecoder decoder;

    const auto events = decoder.feed(
        "\x1B[<64;5;5M"   // wheel: current PointerEvent has no wheel delta
        "\x1B[<128;5;5M"  // extended button family not represented yet
        "\x1B[<0;0;4M"    // terminal coordinates are one-based
        "\x1B[<0;4;0M"
        "\x1B[<0;4M"      // malformed: missing y coordinate
        "Z");

    /*
     * Complete-but-unsupported reports disappear as complete CSI units. Their private '<', decimal
     * parameters and final bytes must never leak into TextInputEvent, while later ordinary input keeps
     * its normal meaning.
     */
    CHECK(events.size() == 1);
    CHECK(textAt(events, 0).text == "Z");
    CHECK(!decoder.hasPendingInput());
}
