#include "test_framework.hpp"

#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/window.hpp>

#include <cstddef>
#include <optional>
#include <string_view>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

class RadioRenderedMetrics final : public RenderedMeasurementContext {
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

TEST_CASE("Rendered RadioButton measurement reserves selector and caption gap") {
    RadioRenderedMetrics metrics;
    RadioButton radio{"abc"};

    CHECK(radio.measure(metrics) == Size{37, 12});
}

TEST_CASE("Rendered RadioButton draws compact centered selection mark") {
    DisplayList display;
    RadioRenderedMetrics metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};

    Window window;
    window.arrange({0, 0, 160, 60});
    auto& radio = window.emplace<RadioButton>("abc");
    radio.arrange({10, 10, 80, 20});
    CHECK(radio.setSelected(true));

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 5);
    CHECK(std::get<FillRectCommand>(commands[1]) ==
          FillRectCommand{Rect{10, 10, 80, 20}, Color::black});
    CHECK(std::get<StrokeRectCommand>(commands[2]) ==
          StrokeRectCommand{Rect{10, 10, 12, 12}, Color::default_color, 1});

    /*
     * A 12-unit selector uses a 4x4 centered "dot" with the current rectangle-only DisplayList.
     * The semantic RadioButton does not know or depend on this temporary visual representation.
     */
    CHECK(std::get<FillRectCommand>(commands[3]) ==
          FillRectCommand{
              Rect{14, 14, 4, 4},
              Color::default_color,
              FillRole::foreground});

    const auto& text = std::get<DrawTextCommand>(commands[4]);
    CHECK(text.origin == Point{23, 10});
    CHECK(text.text == "abc");
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{23, 10, 67, 20}});
}

TEST_CASE("Rendered RadioButton group transition removes old mark and adds new mark") {
    DisplayList display;
    RadioRenderedMetrics metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};

    RadioGroup group;
    Window window;
    window.arrange({0, 0, 180, 80});
    auto& first = window.emplace<RadioButton>(group, "First");
    auto& second = window.emplace<RadioButton>(group, "Second");
    first.arrange({10, 10, 100, 20});
    second.arrange({10, 40, 100, 20});

    CHECK(first.setSelected(true));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    first.acknowledgeVisualUpdate();
    second.acknowledgeVisualUpdate();
    CHECK(second.setSelected(true));

    CHECK(first.isVisualUpdatePending());
    CHECK(second.isVisualUpdatePending());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    int foreground_marks = 0;
    for (const auto& command : display.commands()) {
        if (const auto* fill = std::get_if<FillRectCommand>(&command);
            fill != nullptr && fill->role == FillRole::foreground) {
            ++foreground_marks;
            CHECK(fill->bounds.y >= second.bounds().y);
        }
    }

    CHECK(foreground_marks == 1);
}

TEST_CASE("Rendered RadioButton focus hover and press are presentation overlays") {
    DisplayList display;
    RadioRenderedMetrics metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;
    PointerRouter pointer;

    Window window;
    window.arrange({0, 0, 160, 60});
    auto& radio = window.emplace<RadioButton>("abc");
    radio.arrange({10, 10, 80, 20});

    TextStyle base;
    base.foreground = Color::bright_green;
    radio.setTextStyle(base);

    CHECK(focus.requestFocus(radio));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    display.clear();

    (void)pointer.route(
        window,
        PointerEvent{{30, 15}, PointerAction::move, PointerButton::none, 0});
    CHECK(radio.isPointerOver());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto& hover_text =
        std::get<DrawTextCommand>(display.commands().back());
    CHECK(hover_text.style.inverse);
    CHECK(hover_text.style.underline);
    CHECK(radio.textStyle() == base);

    display.clear();
    CHECK(pointer.route(
        window,
        PointerEvent{{30, 15}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(radio.isPressed());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto& pressed_text =
        std::get<DrawTextCommand>(display.commands().back());
    CHECK(pressed_text.style.inverse);
    CHECK(!pressed_text.style.underline);
    CHECK(radio.textStyle() == base);
}

TEST_CASE("Rendered RadioButton without metrics defers transactionally") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};
    display.drawText({1, 2}, "previous");

    RadioButton radio{"abc"};
    radio.arrange({10, 10, 80, 20});

    const auto result = sink.synchronize(radio);

    CHECK(result == PresentationUpdateResult::deferred);
    CHECK(display.size() == 1);
    CHECK(std::get<DrawTextCommand>(display.commands()[0]).text == "previous");
    CHECK(radio.isVisualUpdatePending());
}
