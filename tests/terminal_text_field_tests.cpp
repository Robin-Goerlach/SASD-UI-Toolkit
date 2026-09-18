#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("TerminalMeasurementContext reserves TextField chrome and end caret cell") {
    TerminalMeasurementContext context;

    TextField empty;
    CHECK(empty.measure(context) == Size{3, 1});

    TextField text{"abc"};
    CHECK(text.measure(context) == Size{6, 1});
}

TEST_CASE("Terminal TextField renders normal focused and disabled chrome") {
    ScreenBuffer buffer{{16, 3}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 16, 3});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({1, 1, field.measure(metrics).width, 1});

    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 1}) == Cell{U'['});
    CHECK(buffer.at({2, 1}) == Cell{U'a'});
    CHECK(buffer.at({4, 1}) == Cell{U'c'});
    CHECK(buffer.at({5, 1}) == Cell{U' '});
    CHECK(buffer.at({6, 1}) == Cell{U']'});
    CHECK(!sink.caretPosition().has_value());

    CHECK(focus.requestFocus(field));
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 1}).code_point == U'>');
    CHECK(buffer.at({6, 1}).code_point == U'<');
    CHECK(sink.caretPosition().has_value());
    CHECK(*sink.caretPosition() == Point{5, 1});

    field.setEnabled(false);
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 1}).code_point == U'(');
    CHECK(buffer.at({6, 1}).code_point == U')');
    CHECK(!sink.caretPosition().has_value());
}

TEST_CASE("Terminal TextField horizontal viewport follows scalar cursor") {
    ScreenBuffer buffer{{12, 2}};
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 12, 2});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({1, 0, 5, 1}); // chrome + three interior cells

    CHECK(focus.requestFocus(field));
    field.setCursorPosition(6);

    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 0}).code_point == U'>');
    CHECK(buffer.at({2, 0}) == Cell{U'e'});
    CHECK(buffer.at({3, 0}) == Cell{U'f'});
    CHECK(buffer.at({5, 0}) == Cell{U'<'});
    CHECK(sink.caretPosition().has_value());
    CHECK(*sink.caretPosition() == Point{4, 0});

    field.setCursorPosition(0);
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({2, 0}) == Cell{U'a'});
    CHECK(buffer.at({3, 0}) == Cell{U'b'});
    CHECK(buffer.at({4, 0}) == Cell{U'c'});
    CHECK(*sink.caretPosition() == Point{2, 0});
}

TEST_CASE("Terminal TextField viewport never renders half a wide scalar") {
    ScreenBuffer buffer{{10, 2}};
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 10, 2});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"});
    field.arrange({0, 0, 5, 1}); // three interior cells
    CHECK(focus.requestFocus(field));

    field.setCursorPosition(2); // immediately before B
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({0, 0}).code_point == U'>');
    // Scrolling starts on the wide scalar boundary rather than its continuation cell.
    CHECK(buffer.at({1, 0}) == Cell{U'\u754C', CellRole::wide_lead});
    CHECK(buffer.at({2, 0}) == Cell{U' ', CellRole::wide_continuation});
    CHECK(buffer.at({3, 0}) == Cell{U'B'});
    CHECK(sink.caretPosition().has_value());
    CHECK(*sink.caretPosition() == Point{3, 0});
}

TEST_CASE("Unrelated visual updates do not erase clean focused TextField caret") {
    ScreenBuffer buffer{{20, 4}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 20, 4});

    auto& box = window.emplace<VBox>();
    auto& field = box.emplace<TextField>("abc");
    auto& other = box.emplace<Button>("Run");

    (void)box.measure(metrics, {{0, 0}, {20, 4}});
    box.arrange({0, 0, 10, 3});

    CHECK(focus.requestFocus(field));
    (void)PresentationCoordinator::synchronize(window, sink);
    const auto before = sink.caretPosition();
    CHECK(before.has_value());

    other.setText("Go");
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(sink.caretPosition() == before);
}

TEST_CASE("Focus transfer between TextFields leaves caret on the new owner regardless of tree order") {
    ScreenBuffer buffer{{20, 4}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 20, 4});

    auto& box = window.emplace<VBox>();
    auto& first = box.emplace<TextField>("one");
    auto& second = box.emplace<TextField>("two");

    (void)box.measure(metrics, {{0, 0}, {20, 4}});
    box.arrange({0, 0, 8, 2});

    CHECK(focus.requestFocus(second));
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(focus.requestFocus(first));
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(sink.caretPosition().has_value());
    CHECK(sink.caretPosition()->y == first.bounds().y);
}

TEST_CASE("Combining TextField content is deferred without damaging previous cells") {
    ScreenBuffer buffer{{14, 2}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 14, 2});

    auto& field = window.emplace<TextField>("old");
    field.arrange({1, 0, field.measure(metrics).width, 1});
    (void)PresentationCoordinator::synchronize(window, sink);

    field.setText(std::string{"e\xCC\x81"}); // e + COMBINING ACUTE
    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.deferred == 1);
    CHECK(field.isVisualUpdatePending());
    CHECK(buffer.at({1, 0}) == Cell{U'['});
    CHECK(buffer.at({2, 0}) == Cell{U'o'});
}


TEST_CASE("Focused terminal TextField styles reserved caret space continuously") {
    ScreenBuffer buffer{{10, 2}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 10, 2});

    auto& field = window.emplace<TextField>("abc");
    TextStyle style;
    style.foreground = Color::cyan;
    field.setTextStyle(style);
    field.arrange({0, 0, field.measure(metrics).width, 1});

    CHECK(focus.requestFocus(field));
    (void)PresentationCoordinator::synchronize(window, sink);

    TextStyle expected = style;
    expected.inverse = true;

    // x=4 is the reserved end-caret cell between "abc" and the right chrome delimiter.
    CHECK(buffer.at({4, 0}).code_point == U' ');
    CHECK(buffer.at({4, 0}).style == expected);
}
