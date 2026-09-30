#include "test_framework.hpp"

#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/window.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal RadioButton measurement reserves stable selector geometry") {
    TerminalMeasurementContext metrics;
    RadioButton radio{std::string{"A\xE7\x95\x8C"}}; // A + wide CJK => three caption columns.

    CHECK(radio.measure(metrics) == Size{7, 1});
}

TEST_CASE("Terminal RadioButton renders selection without changing width") {
    ScreenBuffer buffer{{24, 3}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 24, 3});

    auto& radio = window.emplace<RadioButton>("Feature");
    radio.arrange({1, 1, radio.measure(metrics).width, 1});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({1, 1}).code_point == U'(');
    CHECK(buffer.at({2, 1}).code_point == U' ');
    CHECK(buffer.at({3, 1}).code_point == U')');
    CHECK(buffer.at({5, 1}).code_point == U'F');

    radio.acknowledgeVisualUpdate();
    CHECK(radio.setSelected(true));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    CHECK(buffer.at({1, 1}).code_point == U'(');
    CHECK(buffer.at({2, 1}).code_point == U'o');
    CHECK(buffer.at({3, 1}).code_point == U')');
    CHECK(radio.measure(metrics).width == 11);
}

TEST_CASE("Terminal RadioButton overlays focus disabled and pointer-pressed presentation") {
    ScreenBuffer buffer{{24, 3}};
    TerminalPresentationSink sink{buffer};
    FocusManager focus;
    PointerRouter pointer;

    Window window;
    window.arrange({0, 0, 24, 3});

    auto& radio = window.emplace<RadioButton>("Option");
    radio.arrange({1, 1, 12, 1});

    TextStyle base;
    base.foreground = Color::bright_cyan;
    radio.setTextStyle(base);

    CHECK(focus.requestFocus(radio));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    TextStyle focused = base;
    focused.inverse = true;
    CHECK(buffer.at({1, 1}).style == focused);

    radio.acknowledgeVisualUpdate();
    CHECK(pointer.route(
        window,
        PointerEvent{{2, 1}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(radio.isPressed());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({1, 1}).code_point == U'<');
    CHECK(buffer.at({3, 1}).code_point == U'>');

    pointer.releaseCapture();
    radio.setEnabled(false);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    TextStyle disabled = base;
    disabled.dim = true;
    CHECK(buffer.at({1, 1}).code_point == U'{');
    CHECK(buffer.at({3, 1}).code_point == U'}');
    CHECK(buffer.at({1, 1}).style == disabled);
}

TEST_CASE("Terminal RadioButton group selection repaints old and new members") {
    ScreenBuffer buffer{{30, 4}};
    TerminalPresentationSink sink{buffer};

    RadioGroup group;
    Window window;
    window.arrange({0, 0, 30, 4});

    auto& first = window.emplace<RadioButton>(group, "First");
    auto& second = window.emplace<RadioButton>(group, "Second");
    first.arrange({1, 1, 12, 1});
    second.arrange({1, 2, 12, 1});

    CHECK(first.setSelected(true));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({2, 1}).code_point == U'o');
    CHECK(buffer.at({2, 2}).code_point == U' ');

    first.acknowledgeVisualUpdate();
    second.acknowledgeVisualUpdate();
    CHECK(second.setSelected(true));

    /*
     * RadioGroup updates both semantic members before returning. Both controls therefore become
     * presentation-dirty and one incremental synchronization clears the old mark and draws the new.
     */
    CHECK(first.isVisualUpdatePending());
    CHECK(second.isVisualUpdatePending());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({2, 1}).code_point == U' ');
    CHECK(buffer.at({2, 2}).code_point == U'o');
}

TEST_CASE("Terminal RadioButton defers multiline caption without destroying old frame") {
    ScreenBuffer buffer{{24, 3}};
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 24, 3});
    auto& radio = window.emplace<RadioButton>("old");
    radio.arrange({1, 1, 12, 1});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({5, 1}).code_point == U'o');

    radio.setText("new\nline");
    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.deferred == 1);
    CHECK(radio.isVisualUpdatePending());
    CHECK(buffer.at({5, 1}).code_point == U'o');
}
