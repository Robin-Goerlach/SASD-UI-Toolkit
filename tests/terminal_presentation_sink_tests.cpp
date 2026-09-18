#include "test_framework.hpp"

#include <sasd/ui/container.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
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
    buffer.clear(Cell{U'.'});

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

    CHECK(buffer.at({0, 0}) == Cell{U' '});
    CHECK(buffer.at({2, 2}) == Cell{U'H'});
    CHECK(buffer.at({3, 2}) == Cell{U'e'});
    CHECK(buffer.at({4, 2}) == Cell{U'l'});
    CHECK(buffer.at({5, 2}) == Cell{U'l'});
    CHECK(buffer.at({6, 2}) == Cell{U'o'});
    CHECK(buffer.at({7, 2}) == Cell{U' '});
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

    // 1 (Window) + 2 (panel) + 1 (Label) = screen position 4,4.
    CHECK(buffer.at({4, 4}) == Cell{U'X'});
}

TEST_CASE("TerminalPresentationSink decodes UTF-8 into Unicode code points") {
    ScreenBuffer buffer{{10, 2}};

    Window window;
    window.arrange({0, 0, 10, 2});

    // ASCII A, Greek capital omega U+03A9, ASCII B.
    auto& label = window.emplace<Label>(std::string{"A\xCE\xA9" "B"});
    label.arrange({1, 0, 5, 1});

    TerminalPresentationSink sink{buffer};
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 0}) == Cell{U'A'});
    CHECK(buffer.at({2, 0}) == Cell{U'\u03A9'});
    CHECK(buffer.at({3, 0}) == Cell{U'B'});
}

TEST_CASE("TerminalPresentationSink replaces malformed UTF-8 safely") {
    ScreenBuffer buffer{{8, 2}};

    Window window;
    window.arrange({0, 0, 8, 2});

    // 0xC3 requires a continuation byte; '(' is not one. Recovery consumes one byte and paints U+FFFD.
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
