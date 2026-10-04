#include "test_framework.hpp"

#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal TextField highlights an unfocused scalar selection without changing base style") {
    ScreenBuffer buffer{{16, 2}};
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 16, 2});

    auto& field = window.emplace<TextField>("abcd");
    TextStyle base;
    base.foreground = Color::cyan;
    base.underline = true;
    field.setTextStyle(base);
    field.setSelection(1, 3);
    field.arrange({0, 0, 7, 1}); // chrome + four scalars + reserved caret cell

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    TextStyle selected = base;
    selected.inverse = true;

    CHECK(buffer.at({1, 0}).code_point == U'a');
    CHECK(buffer.at({1, 0}).style == base);
    CHECK(buffer.at({2, 0}).code_point == U'b');
    CHECK(buffer.at({2, 0}).style == selected);
    CHECK(buffer.at({3, 0}).code_point == U'c');
    CHECK(buffer.at({3, 0}).style == selected);
    CHECK(buffer.at({4, 0}).code_point == U'd');
    CHECK(buffer.at({4, 0}).style == base);
}

TEST_CASE("Terminal TextField selection remains distinct inside inverse focused presentation") {
    ScreenBuffer buffer{{16, 2}};
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 16, 2});

    auto& field = window.emplace<TextField>("abcd");
    TextStyle base;
    base.foreground = Color::yellow;
    base.bold = true;
    field.setTextStyle(base);
    field.setSelection(1, 3);
    field.arrange({0, 0, 7, 1});
    CHECK(focus.requestFocus(field));

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    TextStyle focused = base;
    focused.inverse = true;

    /*
     * Focus already inverts the complete TextField. The selection overlay deliberately toggles that
     * bit rather than forcing inverse=true, so selected text remains visible as a contrasting island
     * while every other style attribute supplied by the application is retained.
     */
    CHECK(buffer.at({1, 0}).style == focused);
    CHECK(buffer.at({2, 0}).style == base);
    CHECK(buffer.at({3, 0}).style == base);
    CHECK(buffer.at({4, 0}).style == focused);
    CHECK(buffer.at({0, 0}).style == focused); // focused chrome remains unchanged
    CHECK(sink.caretPosition().has_value());
}

TEST_CASE("Terminal TextField applies one selection style to both cells of a wide scalar") {
    ScreenBuffer buffer{{12, 2}};
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 12, 2});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"}); // A + U+754C + B
    field.setSelection(1, 2); // select exactly the wide U+754C scalar
    field.arrange({0, 0, 7, 1});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    TextStyle selected;
    selected.inverse = true;

    CHECK(buffer.at({2, 0}).code_point == U'\u754C');
    CHECK(buffer.at({2, 0}).role == CellRole::wide_lead);
    CHECK(buffer.at({2, 0}).style == selected);
    CHECK(buffer.at({3, 0}).role == CellRole::wide_continuation);
    CHECK(buffer.at({3, 0}).style == selected);
}

TEST_CASE("Terminal TextField selection follows the horizontally scrolled scalar viewport") {
    ScreenBuffer buffer{{12, 2}};
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 12, 2});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({0, 0, 5, 1}); // chrome + three interior cells
    CHECK(focus.requestFocus(field));

    /*
     * The active selection end is also the caret. Placing it at the text end forces the existing
     * viewport logic to scroll. Selection styling must be derived from semantic scalar indices after
     * that viewport decision, not from fixed screen columns, so the visible tail remains selected.
     */
    field.setSelection(2, 6);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    CHECK(buffer.at({1, 0}).code_point == U'e');
    CHECK(buffer.at({2, 0}).code_point == U'f');
    CHECK(!buffer.at({1, 0}).style.inverse);
    CHECK(!buffer.at({2, 0}).style.inverse);
    CHECK(sink.caretPosition().has_value());
    CHECK(*sink.caretPosition() == Point{3, 0});
}
