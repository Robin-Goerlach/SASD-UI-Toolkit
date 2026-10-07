#include "test_framework.hpp"

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/window.hpp>

#include <string>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal ComboBox measurement reserves stable collapsed chrome") {
    TerminalMeasurementContext metrics;
    ComboBox empty;
    ComboBox combo{{std::string{"A\xE7\x95\x8C"}}}; // A + wide CJK => three text columns.

    CHECK(empty.measure(metrics) == Size{6, 1});
    CHECK(combo.measure(metrics) == Size{9, 1});
}

TEST_CASE("Terminal ComboBox renders empty and selected state with a fixed right indicator") {
    ScreenBuffer buffer{{24, 3}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 24, 3});

    auto& combo = window.emplace<ComboBox>(
        std::vector<std::string>{"One", "Longest"});
    const Coordinate width = combo.measure(metrics).width;
    combo.arrange({1, 1, width, 1});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    const Coordinate arrow_x = static_cast<Coordinate>(1 + width - 3);
    const Coordinate right_x = static_cast<Coordinate>(1 + width - 1);

    CHECK(buffer.at({1, 1}).code_point == U'[');
    CHECK(buffer.at({3, 1}).code_point == U' '); // no committed selection yet
    CHECK(buffer.at({arrow_x, 1}).code_point == U'v');
    CHECK(buffer.at({right_x, 1}).code_point == U']');

    CHECK(combo.setSelectedIndex(1U));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({3, 1}).code_point == U'L');
    CHECK(buffer.at({arrow_x, 1}).code_point == U'v');

    CHECK(combo.setSelectedIndex(0U));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({3, 1}).code_point == U'O');
    CHECK(buffer.at({4, 1}).code_point == U'n');
    CHECK(buffer.at({5, 1}).code_point == U'e');

    /*
     * "Longest" previously occupied more cells. Repainting the full arranged control must remove
     * that old suffix while the drop indicator remains anchored to the same right-side position.
     */
    CHECK(buffer.at({6, 1}).code_point == U' ');
    CHECK(buffer.at({arrow_x, 1}).code_point == U'v');
}

TEST_CASE("Terminal ComboBox overlays focus and disabled presentation without remeasurement") {
    ScreenBuffer buffer{{24, 3}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 24, 3});

    auto& combo = window.emplace<ComboBox>(std::vector<std::string>{"Choice"});
    CHECK(combo.setSelectedIndex(0U));

    TextStyle base;
    base.foreground = Color::bright_cyan;
    combo.setTextStyle(base);

    const Coordinate width = combo.measure(metrics).width;
    combo.arrange({1, 1, width, 1});
    const Coordinate arrow_x = static_cast<Coordinate>(1 + width - 3);
    const Coordinate right_x = static_cast<Coordinate>(1 + width - 1);

    CHECK(focus.requestFocus(combo));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    TextStyle focused = base;
    focused.inverse = true;
    CHECK(buffer.at({1, 1}).code_point == U'>');
    CHECK(buffer.at({right_x, 1}).code_point == U'<');
    CHECK(buffer.at({arrow_x, 1}).code_point == U'v');
    CHECK(buffer.at({arrow_x, 1}).style == focused);
    CHECK(combo.isMeasureValid());

    combo.setEnabled(false);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    TextStyle disabled = base;
    disabled.dim = true;
    CHECK(buffer.at({1, 1}).code_point == U'(');
    CHECK(buffer.at({right_x, 1}).code_point == U')');
    CHECK(buffer.at({arrow_x, 1}).style == disabled);
    CHECK(combo.isMeasureValid());
}

TEST_CASE("Terminal ComboBox defers unsupported selected text before changing the old frame") {
    ScreenBuffer buffer{{24, 3}};
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 24, 3});

    auto& combo = window.emplace<ComboBox>(
        std::vector<std::string>{"old", "new\nline"});
    CHECK(combo.setSelectedIndex(0U));
    combo.arrange({1, 1, 16, 1});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({3, 1}).code_point == U'o');
    CHECK(buffer.at({4, 1}).code_point == U'l');
    CHECK(buffer.at({5, 1}).code_point == U'd');
    CHECK(buffer.at({14, 1}).code_point == U'v');

    CHECK(combo.setSelectedIndex(1U));
    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.deferred == 1);
    CHECK(combo.isVisualUpdatePending());

    /* Deferred means the previous complete collapsed representation remains untouched. */
    CHECK(buffer.at({3, 1}).code_point == U'o');
    CHECK(buffer.at({4, 1}).code_point == U'l');
    CHECK(buffer.at({5, 1}).code_point == U'd');
    CHECK(buffer.at({14, 1}).code_point == U'v');
}
