#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/terminal/terminal_text_field_hit_test.hpp>
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

TEST_CASE("Terminal presentation sink captures an owned frame with current caret metadata") {
    ScreenBuffer buffer{{12, 2}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 12, 2});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({1, 0, field.measure(metrics).width, 1});
    CHECK(focus.requestFocus(field));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(sink.caretPosition().has_value());

    const auto captured = sink.captureFrame();
    CHECK(captured.caret == sink.caretPosition());
    CHECK(captured.buffer.at({2, 0}).code_point == U'a');

    /*
     * captureFrame() is an ownership boundary, not a view. The widget presentation buffer remains the
     * mutable work surface for later synchronization passes, while consumers can safely retain the captured
     * frame as immutable base content for overlays or transport. Proving independence in both directions
     * protects that lifetime contract from a future shortcut that accidentally aliases ScreenBuffer storage.
     */
    buffer.set({2, 0}, Cell{U'X'});
    CHECK(captured.buffer.at({2, 0}).code_point == U'a');

    auto independent = captured;
    independent.buffer.set({2, 0}, Cell{U'Y'});
    CHECK(captured.buffer.at({2, 0}).code_point == U'a');
    CHECK(buffer.at({2, 0}).code_point == U'X');
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
    CHECK(buffer.at({2, 0}).code_point == U'e');
    CHECK(buffer.at({3, 0}).code_point == U'f');
    CHECK(buffer.at({5, 0}).code_point == U'<');
    CHECK(sink.caretPosition().has_value());
    CHECK(*sink.caretPosition() == Point{4, 0});

    field.setCursorPosition(0);
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({2, 0}).code_point == U'a');
    CHECK(buffer.at({3, 0}).code_point == U'b');
    CHECK(buffer.at({4, 0}).code_point == U'c');
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
    CHECK(buffer.at({1, 0}).code_point == U'\u754C');
    CHECK(buffer.at({1, 0}).role == CellRole::wide_lead);
    CHECK(buffer.at({2, 0}).code_point == U' ');
    CHECK(buffer.at({2, 0}).role == CellRole::wide_continuation);
    CHECK(buffer.at({3, 0}).code_point == U'B');
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

TEST_CASE("Terminal TextField hit testing maps discrete cells to scalar caret boundaries") {
    Window window;
    window.arrange({0, 0, 12, 3});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({2, 1, 6, 1}); // chrome + "abc" + one end-caret cell

    /*
     * Terminal pointers identify whole cells. The hit tester evaluates the center of each interior
     * cell, so a one-cell scalar lands on its following caret boundary. Chrome has no text-center
     * meaning and therefore clamps directly to the first/last representable boundary.
     */
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {2, 1}) ==
          std::optional<std::size_t>{0});
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {3, 1}) ==
          std::optional<std::size_t>{1});
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {4, 1}) ==
          std::optional<std::size_t>{2});
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {5, 1}) ==
          std::optional<std::size_t>{3});
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {6, 1}) ==
          std::optional<std::size_t>{3});
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {7, 1}) ==
          std::optional<std::size_t>{3});

    CHECK(!TerminalTextFieldHitTest::caretIndexAt(field, {8, 1}).has_value());
    CHECK(!TerminalTextFieldHitTest::caretIndexAt(field, {4, 0}).has_value());
}

TEST_CASE("Terminal TextField hit testing uses lead and continuation cells of wide scalars") {
    Window window;
    window.arrange({0, 0, 12, 2});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"});
    field.arrange({0, 0, 7, 1}); // five interior cells: A, wide glyph, B, caret room

    /*
     * The two physical cells of U+754C provide exactly the sub-scalar precision a terminal can
     * honestly report. Its lead-cell center lies before the scalar midpoint; its continuation-cell
     * center lies after it. No grapheme or pixel precision is fabricated.
     */
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {1, 0}) ==
          std::optional<std::size_t>{1}); // A
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {2, 0}) ==
          std::optional<std::size_t>{1}); // wide lead -> before U+754C
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {3, 0}) ==
          std::optional<std::size_t>{2}); // continuation -> after U+754C
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {4, 0}) ==
          std::optional<std::size_t>{3}); // B
}

TEST_CASE("Terminal TextField hit testing follows the presentation horizontal viewport") {
    ScreenBuffer buffer{{10, 2}};
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 10, 2});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({1, 0, 5, 1}); // three interior cells
    CHECK(focus.requestFocus(field));
    field.setCursorPosition(6);

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({2, 0}).code_point == U'e');
    CHECK(buffer.at({3, 0}).code_point == U'f');
    CHECK(*sink.caretPosition() == Point{4, 0});

    /*
     * Presentation has scrolled away a..d. The left chrome therefore clamps to scalar boundary 4;
     * clicks in the visible e/f cells advance only within the represented suffix. This couples the
     * regression to actual painted cells so a future viewport-policy change cannot silently leave
     * pointer geometry on the old rule.
     */
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {1, 0}) ==
          std::optional<std::size_t>{4});
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {2, 0}) ==
          std::optional<std::size_t>{5});
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {3, 0}) ==
          std::optional<std::size_t>{6});
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {4, 0}) ==
          std::optional<std::size_t>{6});
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {5, 0}) ==
          std::optional<std::size_t>{6});
}

TEST_CASE("Terminal TextField captured drag clamps to current visible caret boundaries") {
    Window window;
    window.arrange({0, 0, 10, 2});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({1, 0, 5, 1});
    field.setCursorPosition(6);

    CHECK(!TerminalTextFieldHitTest::caretIndexAt(field, {-100, 20}).has_value());
    CHECK(TerminalTextFieldHitTest::caretIndexForDrag(field, {-100, 20}) ==
          std::optional<std::size_t>{4});
    CHECK(TerminalTextFieldHitTest::caretIndexForDrag(field, {100, -20}) ==
          std::optional<std::size_t>{6});
}

TEST_CASE("Terminal TextField hit testing honors ancestor offsets and rejects unsupported state") {
    Window window;
    window.arrange({10, 5, 20, 4});

    auto& box = window.emplace<VBox>();
    box.arrange({2, 1, 10, 2});
    auto& field = box.emplace<TextField>("abc");
    field.arrange({1, 0, 6, 1});

    // Absolute field x is 10 + 2 + 1 = 13; content begins at x=14.
    CHECK(TerminalTextFieldHitTest::caretIndexAt(field, {14, 6}) ==
          std::optional<std::size_t>{1});

    field.setEnabled(false);
    CHECK(!TerminalTextFieldHitTest::caretIndexAt(field, {14, 6}).has_value());
    field.setEnabled(true);

    field.setVisible(false);
    CHECK(!TerminalTextFieldHitTest::caretIndexAt(field, {14, 6}).has_value());
    field.setVisible(true);

    field.setText(std::string{"e\xCC\x81"}); // unsupported combining sequence in current Cell model
    CHECK(!TerminalTextFieldHitTest::caretIndexAt(field, {14, 6}).has_value());
    CHECK(!TerminalTextFieldHitTest::caretIndexForDrag(field, {100, 100}).has_value());
}
