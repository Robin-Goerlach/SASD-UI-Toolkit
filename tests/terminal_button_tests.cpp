#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("TerminalMeasurementContext includes Button chrome in desired size") {
    TerminalMeasurementContext context;
    Button button{"OK"};

    CHECK(button.measure(context) == Size{6, 1});
}

TEST_CASE("Terminal Button renders normal focused and disabled ASCII chrome") {
    ScreenBuffer buffer{{16, 3}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 16, 3});

    auto& button = window.emplace<Button>("OK");
    const Size desired = button.measure(metrics);
    button.arrange({1, 1, desired.width, desired.height});

    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 1}) == Cell{U'['});
    CHECK(buffer.at({2, 1}) == Cell{U' '});
    CHECK(buffer.at({3, 1}) == Cell{U'O'});
    CHECK(buffer.at({4, 1}) == Cell{U'K'});
    CHECK(buffer.at({5, 1}) == Cell{U' '});
    CHECK(buffer.at({6, 1}) == Cell{U']'});

    CHECK(focus.requestFocus(button));
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 1}) == Cell{U'>'});
    CHECK(buffer.at({6, 1}) == Cell{U'<'});

    button.setEnabled(false);
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(!button.hasFocus());
    CHECK(buffer.at({1, 1}) == Cell{U'('});
    CHECK(buffer.at({6, 1}) == Cell{U')'});
}

TEST_CASE("Terminal Button preserves wide caption cell occupancy") {
    ScreenBuffer buffer{{12, 2}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 12, 2});

    auto& button = window.emplace<Button>(std::string{"\xE7\x95\x8C"}); // U+754C
    const Size desired = button.measure(metrics);
    CHECK(desired == Size{6, 1});
    button.arrange({0, 0, desired.width, desired.height});

    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({0, 0}) == Cell{U'['});
    CHECK(buffer.at({2, 0}) == Cell{U'\u754C', CellRole::wide_lead});
    CHECK(buffer.at({3, 0}) == Cell{U' ', CellRole::wide_continuation});
    CHECK(buffer.at({5, 0}) == Cell{U']'});
}

TEST_CASE("Terminal Button defers unsupported combining caption before modifying old cells") {
    ScreenBuffer buffer{{16, 2}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 16, 2});

    auto& button = window.emplace<Button>("Old");
    button.arrange({1, 0, button.measure(metrics).width, 1});
    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 0}) == Cell{U'['});

    button.setText(std::string{"e\xCC\x81"}); // e + COMBINING ACUTE
    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.deferred == 1);
    CHECK(button.isVisualUpdatePending());

    // The previous synchronized button stays intact because validation happens before clearRect().
    CHECK(buffer.at({1, 0}) == Cell{U'['});
    CHECK(buffer.at({3, 0}) == Cell{U'O'});
}

TEST_CASE("VBox lays out Label and Button using the same terminal MeasurementContext") {
    ScreenBuffer buffer{{20, 6}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 20, 6});

    auto& box = window.emplace<VBox>();
    box.setSpacing(1);
    box.emplace<Label>("Name");
    auto& button = box.emplace<Button>("Save");

    const Size desired = box.measure(metrics, {{0, 0}, {20, 6}});
    CHECK(desired == Size{8, 3});
    box.arrange({1, 1, desired.width, desired.height});

    (void)PresentationCoordinator::synchronize(window, sink);

    CHECK(buffer.at({1, 1}) == Cell{U'N'});
    CHECK(buffer.at({1, 3}) == Cell{U'['});
    CHECK(buffer.at({3, 3}) == Cell{U'S'});
    CHECK(button.bounds() == Rect{0, 2, 8, 1});
}
