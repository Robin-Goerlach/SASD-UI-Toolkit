#include "test_framework.hpp"

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/window.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

class ComboBoxRenderedMetrics final : public RenderedMeasurementContext {
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

TEST_CASE("Rendered ComboBox measurement reserves border gap and stable indicator lane") {
    ComboBoxRenderedMetrics metrics;
    ComboBox empty;
    ComboBox combo{{"abc", "longer"}};

    /*
     * Default theme: one-unit border on both sides, one-unit content gap, and a twelve-unit
     * line-height indicator lane. The widest six-character item contributes 48 logical units.
     */
    CHECK(empty.measure(metrics) == Size{15, 14});
    CHECK(combo.measure(metrics) == Size{63, 14});
}

TEST_CASE("Rendered ComboBox draws selected text and right-anchored drop indicator") {
    DisplayList display;
    ComboBoxRenderedMetrics metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};

    Window window;
    window.arrange({0, 0, 160, 60});
    auto& combo = window.emplace<ComboBox>(
        std::vector<std::string>{"abc", "longer"});
    CHECK(combo.setSelectedIndex(0U));
    combo.arrange({10, 10, 80, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 5);
    CHECK(std::get<FillRectCommand>(commands[1]) ==
          FillRectCommand{Rect{10, 10, 80, 20}, Color::black});
    CHECK(std::get<StrokeRectCommand>(commands[2]) ==
          StrokeRectCommand{Rect{10, 10, 80, 20}, Color::default_color, 1});

    const auto& selected = std::get<DrawTextCommand>(commands[3]);
    CHECK(selected.origin == Point{11, 11});
    CHECK(selected.text == "abc");
    CHECK(selected.clip_bounds == std::optional<Rect>{Rect{11, 11, 65, 18}});

    /*
     * The indicator lane occupies the final twelve content units. The eight-unit "v" is centered in
     * that lane, independently of the currently selected text width.
     */
    const auto& indicator = std::get<DrawTextCommand>(commands[4]);
    CHECK(indicator.origin == Point{79, 14});
    CHECK(indicator.text == "v");
    CHECK(indicator.clip_bounds == std::optional<Rect>{Rect{77, 11, 12, 18}});
}

TEST_CASE("Rendered ComboBox selection repaint clears old content without changing geometry") {
    DisplayList display;
    ComboBoxRenderedMetrics metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};

    Window window;
    window.arrange({0, 0, 180, 60});
    auto& combo = window.emplace<ComboBox>(
        std::vector<std::string>{"Longest", "One"});
    CHECK(combo.setSelectedIndex(0U));
    combo.arrange({10, 10, 100, 20});

    const Size measured_before = combo.measure(metrics);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    CHECK(combo.setSelectedIndex(1U));
    CHECK(combo.isMeasureValid());
    CHECK(combo.measure(metrics) == measured_before);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 4);
    CHECK(std::get<FillRectCommand>(commands[0]) ==
          FillRectCommand{Rect{10, 10, 100, 20}, Color::black});

    const auto& selected = std::get<DrawTextCommand>(commands[2]);
    CHECK(selected.text == "One");

    const auto& indicator = std::get<DrawTextCommand>(commands[3]);
    CHECK(indicator.text == "v");
    CHECK(indicator.clip_bounds == std::optional<Rect>{Rect{97, 11, 12, 18}});
}

TEST_CASE("Rendered ComboBox focus and disabled state are presentation overlays") {
    DisplayList display;
    ComboBoxRenderedMetrics metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 160, 60});
    auto& combo = window.emplace<ComboBox>(std::vector<std::string>{"Choice"});
    CHECK(combo.setSelectedIndex(0U));
    combo.arrange({10, 10, 80, 20});

    TextStyle base;
    base.foreground = Color::bright_cyan;
    combo.setTextStyle(base);

    CHECK(focus.requestFocus(combo));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto focused_commands = display.commands();
    const auto& focused_text =
        std::get<DrawTextCommand>(focused_commands[3]);
    const auto& focused_indicator =
        std::get<DrawTextCommand>(focused_commands[4]);
    CHECK(focused_text.style.inverse);
    CHECK(focused_indicator.style.inverse);
    CHECK(combo.textStyle() == base);

    display.clear();
    combo.setEnabled(false);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto disabled_commands = display.commands();
    const auto& disabled_text =
        std::get<DrawTextCommand>(disabled_commands[2]);
    const auto& disabled_indicator =
        std::get<DrawTextCommand>(disabled_commands[3]);
    CHECK(disabled_text.style.dim);
    CHECK(!disabled_text.style.inverse);
    CHECK(disabled_indicator.style.dim);
    CHECK(combo.textStyle() == base);
}

TEST_CASE("Rendered ComboBox without metrics defers transactionally") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};
    display.drawText({1, 2}, "previous");

    ComboBox combo{{"abc"}};
    CHECK(combo.setSelectedIndex(0U));
    combo.arrange({10, 10, 80, 20});

    const auto result = sink.synchronize(combo);

    CHECK(result == PresentationUpdateResult::deferred);
    CHECK(display.size() == 1);
    CHECK(std::get<DrawTextCommand>(display.commands()[0]).text == "previous");
    CHECK(combo.isVisualUpdatePending());
}

TEST_CASE("Rendered ComboBox flips indicator inside unchanged right-side geometry when open") {
    DisplayList display;
    ComboBoxRenderedMetrics metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 160, 60});
    auto& combo = window.emplace<ComboBox>(std::vector<std::string>{"Choice"});
    CHECK(combo.setSelectedIndex(0U));
    combo.arrange({10, 10, 80, 20});
    CHECK(focus.requestFocus(combo));

    const Size measured = combo.measure(metrics);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    const auto& closed_indicator =
        std::get<DrawTextCommand>(display.commands().back());
    CHECK(closed_indicator.text == "v");
    const auto stable_clip = closed_indicator.clip_bounds;
    const Point stable_origin = closed_indicator.origin;

    display.clear();
    combo.acknowledgeVisualUpdate();
    CHECK(combo.setDropDownOpen(true));
    CHECK(combo.isMeasureValid());
    CHECK(combo.measure(metrics) == measured);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto& open_indicator =
        std::get<DrawTextCommand>(display.commands().back());
    CHECK(open_indicator.text == "^");
    CHECK(open_indicator.clip_bounds == stable_clip);
    CHECK(open_indicator.origin == stable_origin);
}
