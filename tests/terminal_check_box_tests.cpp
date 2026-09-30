#include "test_framework.hpp"

#include <sasd/ui/check_box.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>
#include <sasd/ui/terminal/terminal_presentation_sink.hpp>
#include <sasd/ui/window.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal CheckBox measurement reserves stable indicator geometry") {
    TerminalMeasurementContext metrics;
    CheckBox check{std::string{"A\xE7\x95\x8C"}}; // A + wide CJK => three caption columns.

    CHECK(check.measure(metrics) == Size{7, 1});
}

TEST_CASE("Terminal CheckBox renders unchecked and checked state without changing width") {
    ScreenBuffer buffer{{24, 3}};
    TerminalMeasurementContext metrics;
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 24, 3});

    auto& check = window.emplace<CheckBox>("Feature");
    check.arrange({1, 1, check.measure(metrics).width, 1});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({1, 1}).code_point == U'[');
    CHECK(buffer.at({2, 1}).code_point == U' ');
    CHECK(buffer.at({3, 1}).code_point == U']');
    CHECK(buffer.at({5, 1}).code_point == U'F');

    check.acknowledgeVisualUpdate();
    CHECK(check.setChecked(true));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    CHECK(buffer.at({1, 1}).code_point == U'[');
    CHECK(buffer.at({2, 1}).code_point == U'x');
    CHECK(buffer.at({3, 1}).code_point == U']');
    CHECK(check.measure(metrics).width == 11);
}

TEST_CASE("Terminal CheckBox overlays focus disabled and pointer-pressed presentation") {
    ScreenBuffer buffer{{24, 3}};
    TerminalPresentationSink sink{buffer};
    FocusManager focus;
    PointerRouter pointer;

    Window window;
    window.arrange({0, 0, 24, 3});

    auto& check = window.emplace<CheckBox>("Option", true);
    check.arrange({1, 1, 12, 1});

    TextStyle base;
    base.foreground = Color::bright_cyan;
    check.setTextStyle(base);

    CHECK(focus.requestFocus(check));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    TextStyle focused = base;
    focused.inverse = true;
    CHECK(buffer.at({1, 1}).style == focused);
    CHECK(buffer.at({2, 1}).code_point == U'x');

    check.acknowledgeVisualUpdate();
    CHECK(pointer.route(
        window,
        PointerEvent{{2, 1}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(check.isPressed());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({1, 1}).code_point == U'<');
    CHECK(buffer.at({2, 1}).code_point == U'x');
    CHECK(buffer.at({3, 1}).code_point == U'>');

    pointer.releaseCapture();
    check.setEnabled(false);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    TextStyle disabled = base;
    disabled.dim = true;
    CHECK(buffer.at({1, 1}).code_point == U'(');
    CHECK(buffer.at({2, 1}).code_point == U'x');
    CHECK(buffer.at({3, 1}).code_point == U')');
    CHECK(buffer.at({1, 1}).style == disabled);
}

TEST_CASE("Terminal CheckBox defers unsupported multiline caption without destroying old frame") {
    ScreenBuffer buffer{{24, 3}};
    TerminalPresentationSink sink{buffer};

    Window window;
    window.arrange({0, 0, 24, 3});
    auto& check = window.emplace<CheckBox>("old");
    check.arrange({1, 1, 12, 1});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(buffer.at({5, 1}).code_point == U'o');

    check.setText("new\nline");
    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.deferred == 1);
    CHECK(check.isVisualUpdatePending());

    // Deferred means the last complete representation remains untouched.
    CHECK(buffer.at({5, 1}).code_point == U'o');
}
