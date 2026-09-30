#include "test_framework.hpp"

#include <sasd/ui/check_box.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/window.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

class CheckBoxRenderedMetrics final : public RenderedMeasurementContext {
public:
    [[nodiscard]] Size measureText(std::string_view text) const override {
        return {
            static_cast<Coordinate>(utf8::scalarCount(text) * 8),
            lineHeight()};
    }

    [[nodiscard]] Coordinate lineHeight() const noexcept override {
        return 12;
    }

    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view text,
        std::size_t scalar_index) const override {
        const std::size_t count = utf8::scalarCount(text);
        if (scalar_index > count) {
            return std::nullopt;
        }
        return static_cast<Coordinate>(scalar_index * 8);
    }

    [[nodiscard]] std::uint64_t revision() const noexcept override {
        return 1;
    }
};

} // namespace

TEST_CASE("Rendered CheckBox measurement reserves line-height indicator and gap") {
    CheckBoxRenderedMetrics metrics;
    CheckBox check{"abc"};

    // 12-unit indicator + one-unit gap + 24-unit caption.
    CHECK(check.measure(metrics) == Size{37, 12});
}

TEST_CASE("Rendered CheckBox draws indicator mark and caption from one metric policy") {
    DisplayList display;
    CheckBoxRenderedMetrics metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};

    Window window;
    window.arrange({0, 0, 160, 60});
    auto& check = window.emplace<CheckBox>("abc", true);
    check.arrange({10, 10, 80, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 5);

    CHECK(std::get<FillRectCommand>(commands[1]) ==
          FillRectCommand{Rect{10, 10, 80, 20}, Color::black});
    CHECK(std::get<StrokeRectCommand>(commands[2]) ==
          StrokeRectCommand{Rect{10, 10, 12, 12}, Color::default_color, 1});
    CHECK(std::get<FillRectCommand>(commands[3]) ==
          FillRectCommand{
              Rect{11, 11, 10, 10},
              Color::default_color,
              FillRole::foreground});

    const auto& text = std::get<DrawTextCommand>(commands[4]);
    CHECK(text.origin == Point{23, 10});
    CHECK(text.text == "abc");
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{23, 10, 67, 20}});
}

TEST_CASE("Rendered CheckBox focus hover and press are presentation overlays only") {
    DisplayList display;
    CheckBoxRenderedMetrics metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;
    PointerRouter pointer;

    Window window;
    window.arrange({0, 0, 160, 60});
    auto& check = window.emplace<CheckBox>("abc");
    check.arrange({10, 10, 80, 20});

    TextStyle base;
    base.foreground = Color::bright_green;
    check.setTextStyle(base);

    CHECK(focus.requestFocus(check));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    display.clear();

    (void)pointer.route(
        window,
        PointerEvent{{30, 15}, PointerAction::move, PointerButton::none, 0});
    CHECK(check.isPointerOver());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto hover_commands = display.commands();
    const auto& hover_text =
        std::get<DrawTextCommand>(hover_commands.back());
    CHECK(hover_text.style.inverse);
    CHECK(hover_text.style.underline);
    CHECK(check.textStyle() == base);

    display.clear();
    CHECK(pointer.route(
        window,
        PointerEvent{{30, 15}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(check.isPressed());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto& pressed_text =
        std::get<DrawTextCommand>(display.commands().back());
    CHECK(pressed_text.style.inverse);
    CHECK(!pressed_text.style.underline);
    CHECK(check.textStyle() == base);
}

TEST_CASE("Rendered CheckBox without metrics defers transactionally") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    // Seed an earlier command to prove the failing preflight does not mutate the display list.
    display.drawText({1, 2}, "previous");

    CheckBox check{"abc"};
    check.arrange({10, 10, 80, 20});

    const auto result = sink.synchronize(check);

    CHECK(result == PresentationUpdateResult::deferred);
    CHECK(display.size() == 1);
    CHECK(std::get<DrawTextCommand>(display.commands()[0]).text == "previous");
    CHECK(check.isVisualUpdatePending());
}
