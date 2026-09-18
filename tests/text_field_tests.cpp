#include "test_framework.hpp"

#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/text_field.hpp>

#include <cstdint>
#include <string>
#include <string_view>

using namespace sasd::ui;

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

} // namespace

TEST_CASE("TextField is focusable and sanitizes initial single-line UTF-8") {
    TextField field{std::string{"A\nB\xC3("}};

    CHECK(field.isFocusable());
    CHECK(field.text() == std::string{"AB\xEF\xBF\xBD("});
    CHECK(field.cursorPosition() == 4);
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

TEST_CASE("TextField modified navigation remains available for future selection shortcuts") {
    TextField field{"abc"};
    FocusManager focus;
    CHECK(focus.requestFocus(field));

    const auto result =
        EventDispatcher::dispatch(field, KeyEvent{Key::left, true, KeyModifier::shift});

    CHECK(!result.handled());
    CHECK(field.cursorPosition() == 3);
}

TEST_CASE("TextField ignores editing input without logical focus") {
    TextField field{"abc"};

    CHECK(!EventDispatcher::dispatch(field, TextInputEvent{"X"}).handled());
    CHECK(!EventDispatcher::dispatch(
        field, KeyEvent{Key::backspace, true, KeyModifier::none}).handled());
    CHECK(field.text() == "abc");
}

TEST_CASE("TextField cursor movement invalidates presentation but not measurement") {
    TextField field{"abc"};
    TextFieldMeasurementContext context;

    (void)field.measure(context);
    field.acknowledgeVisualUpdate();
    CHECK(field.isMeasureValid());

    field.setCursorPosition(1);

    CHECK(field.isMeasureValid());
    CHECK(field.isVisualUpdatePending());
}
