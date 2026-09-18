#include "test_framework.hpp"

#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("VBox measures and renders terminal Labels through one measurement policy") {
    ScreenBuffer buffer{{20, 6}};
    TerminalMeasurementContext metrics;

    Window window;
    window.arrange({0, 0, 20, 6});

    auto& box = window.emplace<VBox>();
    box.setSpacing(1);
    box.emplace<Label>("One");
    box.emplace<Label>("Two");

    const Size desired = box.measure(metrics, {{0, 0}, {20, 6}});
    CHECK(desired == Size{3, 3});

    box.arrange({1, 1, desired.width, desired.height});

    TerminalPresentationSink sink{buffer};
    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.complete());
    CHECK(buffer.at({1, 1}) == Cell{U'O'});
    CHECK(buffer.at({1, 2}) == Cell{U' '});
    CHECK(buffer.at({1, 3}) == Cell{U'T'});
}
