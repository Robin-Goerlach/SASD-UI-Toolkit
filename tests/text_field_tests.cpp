#include "test_framework.hpp"

#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/testing/memory_clipboard.hpp>
#include <sasd/ui/text_field.hpp>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace sasd::ui;
using sasd::ui::testing::MemoryClipboard;

namespace {

class TextFieldMeasurementContext final : public MeasurementContext {
public:
    Size measureText(std::string_view) const override {
        return {1, 1};
    }

    Size measureTextField(std::string_view) const override {
        ++field_calls_;
        return {11, 1};
    }

    std::uint64_t revision() const noexcept override { return 0; }
    [[nodiscard]] int fieldCalls() const noexcept { return field_calls_; }

private:
    mutable int field_calls_{0};
};

class ThrowingReadClipboard final : public Clipboard {
public:
    [[nodiscard]] std::optional<std::string> readText() const override {
        throw std::runtime_error{"synthetic clipboard read failure"};
    }

    void writeText(std::string) override {}
    void clear() override {}
};

} // namespace

TEST_CASE("TextField is focusable and sanitizes initial single-line UTF-8") {
    TextField field{std::string{"A\nB\xC3("}};

    CHECK(field.isFocusable());
    CHECK(field.text() == std::string{"AB\xEF\xBF\xBD("});
    CHECK(field.cursorPosition() == 4);
    CHECK(field.selectionAnchor() == 4);
    CHECK(!field.hasSelection());
}

TEST_CASE("TextField uses control-specific measurement hook") {
    TextField field{"abc"};
    TextFieldMeasurementContext context;

    CHECK(field.measure(context) == Size{11, 1});
    CHECK(context.fieldCalls() == 1);
}

TEST_CASE("TextField inserts TextInputEvent at Unicode scalar cursor") {
    TextField field{std::string{"A\xE7\x95\x8C" "B"}}; // A + U+754C + B
    FocusManager focus;

    CHECK(focus.requestFocus(field));
    field.setCursorPosition(2);

    const auto result =
        EventDispatcher::dispatch(field, TextInputEvent{"X"});

    CHECK(result.handled());
    CHECK(field.text() == std::string{"A\xE7\x95\x8C" "XB"});
    CHECK(field.cursorPosition() == 3);
    CHECK(!field.hasSelection());
}

TEST_CASE("TextField filters pasted line controls but owns focused text input") {
    TextField field{"ab"};
    FocusManager focus;

    CHECK(focus.requestFocus(field));

    const auto result =
        EventDispatcher::dispatch(field, TextInputEvent{"\n\t"});

    CHECK(result.handled());
    CHECK(field.text() == "ab");
}

TEST_CASE("TextField selection preserves anchor direction and returns Unicode text") {
    TextField field{std::string{"A\xCE\xA9\xE7\x95\x8C" "B"}}; // A + Omega + CJK + B

    field.setSelection(3, 1);

    CHECK(field.hasSelection());
    CHECK(field.selectionAnchor() == 3);
    CHECK(field.cursorPosition() == 1);
    CHECK(field.selectionStart() == 1);
    CHECK(field.selectionEnd() == 3);
    CHECK(field.selectedText() == std::string{"\xCE\xA9\xE7\x95\x8C"});
}

TEST_CASE("TextField selection endpoints clamp independently and cursor movement collapses it") {
    TextField field{"abcd"};

    field.setSelection(99, 1);
    CHECK(field.selectionAnchor() == 4);
    CHECK(field.cursorPosition() == 1);
    CHECK(field.selectionStart() == 1);
    CHECK(field.selectionEnd() == 4);

    field.setCursorPosition(2);
    CHECK(!field.hasSelection());
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 2);
}

TEST_CASE("TextField text input replaces the selected scalar range") {
    TextField field{std::string{"A\xCE\xA9\xE7\x95\x8C" "B"}};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    field.setSelection(3, 1);
    const auto result = EventDispatcher::dispatch(field, TextInputEvent{"XY"});

    CHECK(result.handled());
    CHECK(field.text() == "AXYB");
    CHECK(field.cursorPosition() == 3);
    CHECK(field.selectionAnchor() == 3);
    CHECK(!field.hasSelection());
}

TEST_CASE("TextField rejected input does not destroy an existing selection") {
    TextField field{"abcd"};
    FocusManager focus;
    CHECK(focus.requestFocus(field));
    field.setSelection(1, 3);

    const auto result = EventDispatcher::dispatch(field, TextInputEvent{"\n\t"});

    CHECK(result.handled());
    CHECK(field.text() == "abcd");
    CHECK(field.selectionStart() == 1);
    CHECK(field.selectionEnd() == 3);
    CHECK(field.hasSelection());
}

TEST_CASE("TextField pastes clipboard text through the normal sanitized insertion path") {
    TextField field{std::string{"A\xCE\xA9" "B"}}; // A + U+03A9 + B
    MemoryClipboard clipboard;

    field.setCursorPosition(2);
    clipboard.writeText(std::string{"X\n\xE7\x95\x8C"}); // X + newline + U+754C

    /*
     * Programmatic paste deliberately does not require focus. A menu/command layer may already have
     * established the editing target while transient menu interaction owns keyboard focus routing.
     */
    CHECK(field.pasteFromClipboard(clipboard));
    CHECK(field.text() == std::string{"A\xCE\xA9X\xE7\x95\x8C" "B"});
    CHECK(field.cursorPosition() == 4);
}

TEST_CASE("TextField clipboard paste replaces selection only after insertable text is available") {
    TextField field{"abcd"};
    MemoryClipboard clipboard;
    field.setSelection(1, 3);

    clipboard.writeText("\n\t");
    CHECK(!field.pasteFromClipboard(clipboard));
    CHECK(field.text() == "abcd");
    CHECK(field.hasSelection());

    clipboard.writeText("XY");
    CHECK(field.pasteFromClipboard(clipboard));
    CHECK(field.text() == "aXYd");
    CHECK(field.cursorPosition() == 3);
    CHECK(!field.hasSelection());
}

TEST_CASE("TextField clipboard paste is a no-op when no insertable text exists") {
    TextField field{"abc"};
    TextFieldMeasurementContext context;
    MemoryClipboard clipboard;

    (void)field.measure(context);
    field.acknowledgeVisualUpdate();
    field.setCursorPosition(1);
    field.acknowledgeVisualUpdate();
    CHECK(field.isMeasureValid());
    CHECK(!field.isVisualUpdatePending());

    CHECK(!field.pasteFromClipboard(clipboard));
    CHECK(field.text() == "abc");
    CHECK(field.cursorPosition() == 1);
    CHECK(field.isMeasureValid());
    CHECK(!field.isVisualUpdatePending());

    clipboard.writeText("\n\t");
    CHECK(!field.pasteFromClipboard(clipboard));
    CHECK(field.text() == "abc");
    CHECK(field.cursorPosition() == 1);
    CHECK(field.isMeasureValid());
    CHECK(!field.isVisualUpdatePending());
}

TEST_CASE("TextField leaves state unchanged when clipboard reading fails") {
    TextField field{"abc"};
    ThrowingReadClipboard clipboard;
    field.setSelection(0, 2);

    bool threw = false;
    try {
        (void)field.pasteFromClipboard(clipboard);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(field.text() == "abc");
    CHECK(field.selectionStart() == 0);
    CHECK(field.selectionEnd() == 2);
}

TEST_CASE("TextField Left Right Home End navigate Unicode scalars") {
    TextField field{std::string{"A\xE7\x95\x8C" "B"}};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    CHECK(field.cursorPosition() == 3);

    (void)EventDispatcher::dispatch(field, KeyEvent{Key::left, true, KeyModifier::none});
    CHECK(field.cursorPosition() == 2);

    (void)EventDispatcher::dispatch(field, KeyEvent{Key::left, true, KeyModifier::none});
    CHECK(field.cursorPosition() == 1);

    (void)EventDispatcher::dispatch(field, KeyEvent{Key::home, true, KeyModifier::none});
    CHECK(field.cursorPosition() == 0);

    (void)EventDispatcher::dispatch(field, KeyEvent{Key::end, true, KeyModifier::none});
    CHECK(field.cursorPosition() == 3);

    (void)EventDispatcher::dispatch(field, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(field.cursorPosition() == 3);
}

TEST_CASE("TextField unmodified arrows collapse selection toward the requested edge") {
    TextField field{"abcd"};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    field.setSelection(1, 3);
    (void)EventDispatcher::dispatch(field, KeyEvent{Key::left, true, KeyModifier::none});
    CHECK(field.cursorPosition() == 1);
    CHECK(!field.hasSelection());

    field.setSelection(1, 3);
    (void)EventDispatcher::dispatch(field, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(field.cursorPosition() == 3);
    CHECK(!field.hasSelection());
}

TEST_CASE("TextField Shift arrows extend shrink and cross a stable selection anchor") {
    TextField field{std::string{"A\xE7\x95\x8C" "BC"}}; // A + U+754C + B + C
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    field.setCursorPosition(2);

    const auto first_left =
        EventDispatcher::dispatch(field, KeyEvent{Key::left, true, KeyModifier::shift});
    CHECK(first_left.handled());
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 1);
    CHECK(field.selectedText() == std::string{"\xE7\x95\x8C"});

    (void)EventDispatcher::dispatch(field, KeyEvent{Key::left, true, KeyModifier::shift});
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 0);
    CHECK(field.selectionStart() == 0);
    CHECK(field.selectionEnd() == 2);

    /*
     * Moving the active end back to the anchor collapses the selection without losing the anchor. One
     * more Shift+Right then creates a forward selection from that same anchor, proving direction is a
     * consequence of the active cursor and not a rewritten normalized range.
     */
    (void)EventDispatcher::dispatch(field, KeyEvent{Key::right, true, KeyModifier::shift});
    (void)EventDispatcher::dispatch(field, KeyEvent{Key::right, true, KeyModifier::shift});
    CHECK(!field.hasSelection());
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 2);

    (void)EventDispatcher::dispatch(field, KeyEvent{Key::right, true, KeyModifier::shift});
    CHECK(field.hasSelection());
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 3);
    CHECK(field.selectedText() == "B");
}

TEST_CASE("TextField Shift Home and End extend from the existing anchor") {
    TextField field{"abcdef"};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    field.setCursorPosition(3);
    const auto home =
        EventDispatcher::dispatch(field, KeyEvent{Key::home, true, KeyModifier::shift});
    CHECK(home.handled());
    CHECK(field.selectionAnchor() == 3);
    CHECK(field.cursorPosition() == 0);
    CHECK(field.selectedText() == "abc");

    const auto end =
        EventDispatcher::dispatch(field, KeyEvent{Key::end, true, KeyModifier::shift});
    CHECK(end.handled());
    CHECK(field.selectionAnchor() == 3);
    CHECK(field.cursorPosition() == 6);
    CHECK(field.selectedText() == "def");
}

TEST_CASE("TextField Shift navigation releases are consumed without changing selection") {
    TextField field{"abc"};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    field.setCursorPosition(2);
    CHECK(EventDispatcher::dispatch(
        field, KeyEvent{Key::left, true, KeyModifier::shift}).handled());
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 1);

    CHECK(EventDispatcher::dispatch(
        field, KeyEvent{Key::left, false, KeyModifier::shift}).handled());
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 1);
}

TEST_CASE("TextField leaves non-Shift modifier combinations to higher-level policy") {
    TextField field{"abc"};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    const auto control_left = EventDispatcher::dispatch(
        field, KeyEvent{Key::left, true, KeyModifier::control});
    const auto shift_delete = EventDispatcher::dispatch(
        field, KeyEvent{Key::delete_forward, true, KeyModifier::shift});

    CHECK(!control_left.handled());
    CHECK(!shift_delete.handled());
    CHECK(field.cursorPosition() == 3);
    CHECK(!field.hasSelection());
    CHECK(field.text() == "abc");
}

TEST_CASE("TextField Backspace and Delete remove complete UTF-8 scalars") {
    TextField field{std::string{"A\xE7\x95\x8C" "B"}};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    field.setCursorPosition(2);
    (void)EventDispatcher::dispatch(field, KeyEvent{Key::backspace, true, KeyModifier::none});

    CHECK(field.text() == "AB");
    CHECK(field.cursorPosition() == 1);

    (void)EventDispatcher::dispatch(field, KeyEvent{Key::delete_forward, true, KeyModifier::none});

    CHECK(field.text() == "A");
    CHECK(field.cursorPosition() == 1);
}

TEST_CASE("TextField Backspace and Delete remove the whole selection before scalar deletion") {
    TextField field{std::string{"A\xCE\xA9\xE7\x95\x8C" "B"}};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    field.setSelection(1, 3);
    (void)EventDispatcher::dispatch(field, KeyEvent{Key::backspace, true, KeyModifier::none});
    CHECK(field.text() == "AB");
    CHECK(field.cursorPosition() == 1);
    CHECK(!field.hasSelection());

    field.setText(std::string{"A\xCE\xA9\xE7\x95\x8C" "B"});
    field.setSelection(3, 1);
    (void)EventDispatcher::dispatch(field, KeyEvent{Key::delete_forward, true, KeyModifier::none});
    CHECK(field.text() == "AB");
    CHECK(field.cursorPosition() == 1);
    CHECK(!field.hasSelection());
}

TEST_CASE("TextField editing key releases are consumed without repeating mutation") {
    TextField field{"ab"};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    const auto press =
        EventDispatcher::dispatch(field, KeyEvent{Key::backspace, true, KeyModifier::none});
    CHECK(press.handled());
    CHECK(field.text() == "a");

    const auto release =
        EventDispatcher::dispatch(field, KeyEvent{Key::backspace, false, KeyModifier::none});
    CHECK(release.handled());
    CHECK(field.text() == "a");
}

TEST_CASE("TextField ignores editing input without logical focus") {
    TextField field{"abc"};

    CHECK(!EventDispatcher::dispatch(field, TextInputEvent{"X"}).handled());
    CHECK(!EventDispatcher::dispatch(
        field, KeyEvent{Key::backspace, true, KeyModifier::none}).handled());
    CHECK(!EventDispatcher::dispatch(
        field, KeyEvent{Key::left, true, KeyModifier::shift}).handled());
    CHECK(field.text() == "abc");
    CHECK(!field.hasSelection());
}

TEST_CASE("TextField cursor and selection movement invalidate presentation but not measurement") {
    TextField field{"abc"};
    TextFieldMeasurementContext context;
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    (void)field.measure(context);
    field.acknowledgeVisualUpdate();
    CHECK(field.isMeasureValid());

    field.setSelection(0, 2);
    CHECK(field.isMeasureValid());
    CHECK(field.isVisualUpdatePending());

    field.acknowledgeVisualUpdate();
    field.clearSelection();
    CHECK(field.isMeasureValid());
    CHECK(field.isVisualUpdatePending());

    field.acknowledgeVisualUpdate();
    (void)EventDispatcher::dispatch(field, KeyEvent{Key::left, true, KeyModifier::shift});
    CHECK(field.isMeasureValid());
    CHECK(field.isVisualUpdatePending());
}

TEST_CASE("TextField text style is presentation-only") {
    TextField field{"abc"};
    TextFieldMeasurementContext context;

    (void)field.measure(context);
    field.acknowledgeVisualUpdate();

    TextStyle style;
    style.foreground = Color::yellow;
    style.underline = true;
    field.setTextStyle(style);

    CHECK(field.textStyle() == style);
    CHECK(field.isMeasureValid());
    CHECK(field.isVisualUpdatePending());
}
