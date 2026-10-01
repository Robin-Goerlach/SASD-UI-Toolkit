#include "test_framework.hpp"

#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/stack_layout.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/window.hpp>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("StackLayout is structural and later terminal layers paint last") {
    ScreenBuffer buffer{{20, 6}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 20, 6});

    auto& stack = window.emplace<StackLayout>();
    auto& bottom = stack.emplace<Label>("bottom");
    auto& top = stack.emplace<Label>("TOP");

    /*
     * Measure and arrange through the real terminal metric path. StackLayout itself owns no terminal
     * presentation; it only establishes identical parent-relative geometry for its children.
     */
    const Size desired = stack.measure(metrics, {{0, 0}, {12, 3}});
    CHECK(desired == Size{6, 1});

    stack.arrange({2, 2, 12, 2});
    CHECK(bottom.bounds() == Rect{0, 0, 12, 2});
    CHECK(top.bounds() == Rect{0, 0, 12, 2});

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.complete());
    CHECK(!stack.isVisualUpdatePending());

    /*
     * Presentation traverses visual children in adoption order. Each current terminal Label is opaque
     * inside its arranged bounds, so the later top layer clears the shared area after the bottom layer
     * and then draws its own text. This is the presentation counterpart of reverse-order HitTest.
     */
    CHECK(buffer.at({2, 2}) == Cell{U'T'});
    CHECK(buffer.at({3, 2}) == Cell{U'O'});
    CHECK(buffer.at({4, 2}) == Cell{U'P'});
    CHECK(buffer.at({5, 2}) == Cell{U' '});
}

TEST_CASE("Hiding the top StackLayout layer reveals the lower terminal layer after replay") {
    ScreenBuffer buffer{{20, 6}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 20, 6});

    auto& stack = window.emplace<StackLayout>();
    auto& bottom = stack.emplace<Label>("bottom");
    auto& top = stack.emplace<Label>("TOP");

    (void)stack.measure(metrics, {{0, 0}, {12, 3}});
    stack.arrange({2, 2, 12, 2});
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({2, 2}) == Cell{U'T'});

    /*
     * Visibility is the initial page-switching mechanism promised by ADR 0042. The structural damage
     * forces a conservative subtree replay, so content previously covered by the top layer is rebuilt
     * rather than depending on incremental overdraw history.
     */
    top.setVisible(false);
    (void)stack.measure(metrics, {{0, 0}, {12, 3}});
    stack.arrange({2, 2, 12, 2});

    const auto pass = PresentationCoordinator::synchronize(window, sink);
    CHECK(pass.complete());
    CHECK(pass.forced >= 1);
    CHECK(buffer.at({2, 2}) == Cell{U'b'});
    CHECK(buffer.at({7, 2}) == Cell{U'm'});
    CHECK(bottom.isVisible());
}
