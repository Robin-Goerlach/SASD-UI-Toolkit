#include "test_framework.hpp"

#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/window.hpp>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("GridLayout measures and renders terminal Labels through one metric policy") {
    ScreenBuffer buffer{{20, 6}};
    TerminalMeasurementContext metrics;

    Window window;
    window.arrange({0, 0, 20, 6});

    auto& grid = window.emplace<GridLayout>(2);
    grid.setColumnSpacing(1);
    grid.setRowSpacing(1);

    grid.emplace<Label>("A");
    grid.emplace<Label>("BBBB");
    grid.emplace<Label>("CCC");

    const Size desired =
        grid.measure(metrics, {{0, 0}, {20, 6}});

    // Column widths [3,4], row heights [1,1], plus one cell gap on each axis.
    CHECK(desired == Size{8, 3});

    grid.arrange({1, 1, desired.width, desired.height});

    TerminalPresentationSink sink{buffer};
    const auto pass =
        PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.complete());

    /*
     * The first column stretches A to the width required by CCC, while presentation still draws only
     * A's own text. The second-row third label begins at the first column's x origin.
     */
    CHECK(buffer.at({1, 1}) == Cell{U'A'});
    CHECK(buffer.at({5, 1}) == Cell{U'B'});
    CHECK(buffer.at({1, 3}) == Cell{U'C'});
}
