#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/window.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

namespace {

class UnsupportedWidget final : public Widget {};

} // namespace

TEST_CASE("TerminalPresentationSink renders Window and Label through PresentationCoordinator") {
    ScreenBuffer buffer{{20, 6}};

    Window window;
    window.arrange({0, 0, 20, 6});

    auto& label = window.emplace<Label>("Hello");
    label.arrange({2, 2, 8, 1});

    TerminalPresentationSink sink{buffer};
    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.visited == 2);
    CHECK(pass.requested == 2);
    CHECK(pass.synchronized == 2);
    CHECK(pass.deferred == 0);

    CHECK(buffer.at({2, 2}) == Cell{U'H'});
    CHECK(buffer.at({3, 2}) == Cell{U'e'});
    CHECK(buffer.at({4, 2}) == Cell{U'l'});
    CHECK(buffer.at({5, 2}) == Cell{U'l'});
    CHECK(buffer.at({6, 2}) == Cell{U'o'});
}

TEST_CASE("TerminalPresentationSink resolves child bounds through visual parent offsets") {
    ScreenBuffer buffer{{20, 8}};

    Window window;
    window.arrange({1, 1, 18, 6});

    auto& panel = window.emplace<Container>();
    panel.arrange({2, 1, 10, 4});

    auto& label = panel.emplace<Label>("X");
    label.arrange({1, 2, 3, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({4, 4}) == Cell{U'X'});
}

TEST_CASE("TerminalPresentationSink uses Unicode cell width for wide glyphs") {
    ScreenBuffer buffer{{12, 2}};

    Window window;
    window.arrange({0, 0, 12, 2});

    // A + U+754C (wide) + B. B must start three columns after A, not two bytes/code points later.
    auto& label = window.emplace<Label>(std::string{"A\xE7\x95\x8C" "B"});
    label.arrange({1, 0, 8, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 0}) == Cell{U'A', CellRole::normal});
    CHECK(buffer.at({2, 0}) == Cell{U'\u754C', CellRole::wide_lead});
    CHECK(buffer.at({3, 0}) == Cell{U' ', CellRole::wide_continuation});
    CHECK(buffer.at({4, 0}) == Cell{U'B', CellRole::normal});
}

TEST_CASE("TerminalPresentationSink never emits half of a clipped wide glyph") {
    ScreenBuffer buffer{{5, 1}};

    Window window;
    window.arrange({0, 0, 5, 1});

    auto& label = window.emplace<Label>(std::string{"\xE7\x95\x8C"});
    label.arrange({4, 0, 1, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({4, 0}) == Cell{});
}

TEST_CASE("TerminalPresentationSink honors configurable East Asian ambiguous width") {
    const std::string inverted_exclamation{"\xC2\xA1"};

    ScreenBuffer narrow_buffer{{6, 1}};
    Window narrow_window;
    narrow_window.arrange({0, 0, 6, 1});
    auto& narrow_label = narrow_window.emplace<Label>(inverted_exclamation + "X");
    narrow_label.arrange({0, 0, 5, 1});

    TerminalPresentationSink narrow_sink{narrow_buffer, AmbiguousWidthMode::narrow};
    (void)PresentationCoordinator::synchronize(narrow_window, narrow_sink);
    CHECK(narrow_buffer.at({1, 0}) == Cell{U'X'});

    ScreenBuffer wide_buffer{{6, 1}};
    Window wide_window;
    wide_window.arrange({0, 0, 6, 1});
    auto& wide_label = wide_window.emplace<Label>(inverted_exclamation + "X");
    wide_label.arrange({0, 0, 5, 1});

    TerminalPresentationSink wide_sink{wide_buffer, AmbiguousWidthMode::wide};
    (void)PresentationCoordinator::synchronize(wide_window, wide_sink);
    CHECK(wide_buffer.at({0, 0}) == Cell{U'\u00A1', CellRole::wide_lead});
    CHECK(wide_buffer.at({1, 0}) == Cell{U' ', CellRole::wide_continuation});
    CHECK(wide_buffer.at({2, 0}) == Cell{U'X'});
}

TEST_CASE("TerminalPresentationSink decodes malformed UTF-8 as replacement glyph") {
    ScreenBuffer buffer{{8, 2}};

    Window window;
    window.arrange({0, 0, 8, 2});

    auto& label = window.emplace<Label>(std::string{"\xC3("});
    label.arrange({1, 0, 4, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 0}) == Cell{U'\uFFFD'});
    CHECK(buffer.at({2, 0}) == Cell{U'('});
}

TEST_CASE("TerminalPresentationSink clips Label text to its arranged rectangle") {
    ScreenBuffer buffer{{10, 3}};

    Window window;
    window.arrange({0, 0, 10, 3});

    auto& label = window.emplace<Label>("abcdef");
    label.arrange({7, 1, 2, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({7, 1}) == Cell{U'a'});
    CHECK(buffer.at({8, 1}) == Cell{U'b'});
    CHECK(buffer.at({9, 1}) == Cell{U' '});
}

TEST_CASE("TerminalPresentationSink handles Label newlines within arranged height") {
    ScreenBuffer buffer{{10, 4}};

    Window window;
    window.arrange({0, 0, 10, 4});

    auto& label = window.emplace<Label>("ab\ncd");
    label.arrange({2, 1, 4, 2});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({2, 1}) == Cell{U'a'});
    CHECK(buffer.at({3, 1}) == Cell{U'b'});
    CHECK(buffer.at({2, 2}) == Cell{U'c'});
    CHECK(buffer.at({3, 2}) == Cell{U'd'});
}

TEST_CASE("Hiding a Label clears its current terminal rectangle") {
    ScreenBuffer buffer{{12, 3}};

    Window window;
    window.arrange({0, 0, 12, 3});

    auto& label = window.emplace<Label>("Visible");
    label.arrange({2, 1, 7, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);
    CHECK(buffer.at({2, 1}) == Cell{U'V'});

    label.setVisible(false);
    const auto second = PresentationCoordinator::synchronize(window, sink);

    CHECK(second.complete());
    for (Coordinate x = 2; x < 9; ++x) {
        CHECK(buffer.at({x, 1}) == Cell{U' '});
    }
}

TEST_CASE("Updating one Label does not erase clean sibling presentation") {
    ScreenBuffer buffer{{20, 3}};

    Window window;
    window.arrange({0, 0, 20, 3});

    auto& left = window.emplace<Label>("Left");
    left.arrange({1, 1, 6, 1});
    auto& right = window.emplace<Label>("Right");
    right.arrange({10, 1, 6, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    left.setText("New");
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 1}) == Cell{U'N'});
    CHECK(buffer.at({10, 1}) == Cell{U'R'});
    CHECK(buffer.at({14, 1}) == Cell{U't'});
}

TEST_CASE("Combining text is deferred before existing terminal cells are modified") {
    ScreenBuffer buffer{{12, 2}};

    Window window;
    window.arrange({0, 0, 12, 2});

    auto& label = window.emplace<Label>("old");
    label.arrange({1, 0, 6, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    label.setText(std::string{"e\xCC\x81"}); // e + COMBINING ACUTE ACCENT
    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.deferred == 1);
    CHECK(label.isVisualUpdatePending());

    // Deferred means "keep the last synchronized representation", not "partially clear and fail".
    CHECK(buffer.at({1, 0}) == Cell{U'o'});
    CHECK(buffer.at({2, 0}) == Cell{U'l'});
    CHECK(buffer.at({3, 0}) == Cell{U'd'});
}

TEST_CASE("Unknown concrete terminal widgets stay pending instead of being silently acknowledged") {
    ScreenBuffer buffer{{10, 3}};

    Window window;
    window.arrange({0, 0, 10, 3});
    auto& unknown = window.emplace<UnsupportedWidget>();
    unknown.arrange({1, 1, 3, 1});

    TerminalPresentationSink sink{buffer};
    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.deferred == 1);
    CHECK(unknown.isVisualUpdatePending());
    CHECK(!window.isVisualUpdatePending());
}


TEST_CASE("Moving a Label clears its old terminal representation") {
    ScreenBuffer buffer{{20, 3}};

    Window window;
    window.arrange({0, 0, 20, 3});

    auto& label = window.emplace<Label>("Move");
    label.arrange({1, 1, 6, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);
    CHECK(buffer.at({1, 1}) == Cell{U'M'});

    label.arrange({10, 1, 6, 1});
    const auto moved = PresentationCoordinator::synchronize(window, sink);

    CHECK(moved.complete());
    CHECK(buffer.at({1, 1}) == Cell{U' '});
    CHECK(buffer.at({10, 1}) == Cell{U'M'});
    CHECK(buffer.at({13, 1}) == Cell{U'e'});
}

TEST_CASE("Moving a Container replays clean descendants at their new absolute positions") {
    ScreenBuffer buffer{{24, 5}};

    Window window;
    window.arrange({0, 0, 24, 5});

    auto& panel = window.emplace<Container>();
    panel.arrange({1, 1, 10, 2});
    auto& label = panel.emplace<Label>("Child");
    label.arrange({1, 0, 6, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);
    CHECK(buffer.at({2, 1}) == Cell{U'C'});

    panel.arrange({10, 2, 10, 2});
    const auto moved = PresentationCoordinator::synchronize(window, sink);

    CHECK(moved.complete());
    CHECK(moved.forced >= 1);
    CHECK(buffer.at({2, 1}) == Cell{U' '});
    CHECK(buffer.at({11, 2}) == Cell{U'C'});
}

TEST_CASE("Removing a Label clears its old terminal representation and replays siblings") {
    ScreenBuffer buffer{{24, 4}};

    Window window;
    window.arrange({0, 0, 24, 4});

    auto& removed = window.emplace<Label>("Gone");
    removed.arrange({1, 1, 6, 1});
    auto& survivor = window.emplace<Label>("Stay");
    survivor.arrange({12, 1, 6, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    auto released = window.release(removed);
    CHECK(released != nullptr);

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.complete());
    CHECK(buffer.at({1, 1}) == Cell{U' '});
    CHECK(buffer.at({12, 1}) == Cell{U'S'});
    CHECK(buffer.at({15, 1}) == Cell{U'y'});
}


TEST_CASE("TerminalPresentationSink stores Label TextStyle in rendered cells") {
    ScreenBuffer buffer{{12, 2}};

    Window window;
    window.arrange({0, 0, 12, 2});

    auto& label = window.emplace<Label>("Styled");
    TextStyle style;
    style.foreground = Color::bright_cyan;
    style.bold = true;
    style.underline = true;
    label.setTextStyle(style);
    label.arrange({1, 0, 8, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 0}).code_point == U'S');
    CHECK(buffer.at({1, 0}).style == style);
    CHECK(buffer.at({6, 0}).style == style);
}

TEST_CASE("TerminalPresentationSink overlays focus and disabled attributes on Button style") {
    ScreenBuffer buffer{{14, 2}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 14, 2});

    auto& button = window.emplace<Button>("Go");
    TextStyle base;
    base.foreground = Color::green;
    base.bold = true;
    button.setTextStyle(base);
    button.arrange({0, 0, button.measure(metrics).width, 1});

    (void)PresentationCoordinator::synchronize(window, sink);
    CHECK(buffer.at({0, 0}).style == base);

    CHECK(focus.requestFocus(button));
    (void)PresentationCoordinator::synchronize(window, sink);

    TextStyle focused = base;
    focused.inverse = true;
    CHECK(buffer.at({0, 0}).style == focused);
    CHECK(buffer.at({2, 0}).style == focused);

    button.setEnabled(false);
    (void)PresentationCoordinator::synchronize(window, sink);

    TextStyle disabled = base;
    disabled.dim = true;
    CHECK(buffer.at({0, 0}).style == disabled);
    CHECK(buffer.at({2, 0}).style == disabled);
}
