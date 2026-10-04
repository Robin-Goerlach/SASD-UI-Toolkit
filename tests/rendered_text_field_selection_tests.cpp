#include "test_framework.hpp"

#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

/**
 * Deterministic scalar-boundary metrics for selection-presentation tests.
 *
 * Keeping this fake deliberately simple makes the expected selection rectangle obvious: every
 * Unicode scalar advances by eight logical units. The production contract still receives complete
 * UTF-8 text plus a scalar boundary, so these tests validate the same full-run metric path used by
 * real shaping providers without baking terminal-cell assumptions into Rendered presentation.
 */
class SelectionMeasurementContext : public RenderedMeasurementContext {
public:
    [[nodiscard]] Size measureText(std::string_view utf8_text) const override {
        const auto width = textAdvanceToScalar(utf8_text, utf8::scalarCount(utf8_text));
        return {width.value_or(0), lineHeight()};
    }

    [[nodiscard]] Coordinate lineHeight() const noexcept override {
        return 12;
    }

    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view utf8_text,
        std::size_t scalar_index) const override {
        const std::size_t count = utf8::scalarCount(utf8_text);
        const std::size_t clamped = std::min(scalar_index, count);
        return static_cast<Coordinate>(clamped * 8);
    }

    [[nodiscard]] std::uint64_t revision() const noexcept override {
        return 1;
    }
};

/**
 * Simulates a shaping backend that can place the cursor but cannot report one farther selection
 * boundary. The sink must discover that limitation before appending any repaint commands.
 */
class UnsupportedSelectionBoundaryContext final : public SelectionMeasurementContext {
public:
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view utf8_text,
        std::size_t scalar_index) const override {
        if (scalar_index == 3) {
            return std::nullopt;
        }
        return SelectionMeasurementContext::textAdvanceToScalar(utf8_text, scalar_index);
    }
};

} // namespace

TEST_CASE("Rendered TextField selection redraws the full shaped run through a narrow inverse clip") {
    DisplayList display;
    SelectionMeasurementContext metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};

    Window window;
    window.arrange({0, 0, 120, 50});

    auto& field = window.emplace<TextField>("abcd");
    field.arrange({10, 10, 50, 14});
    field.setSelection(1, 3);

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 5);

    const auto& base = std::get<DrawTextCommand>(commands[3]);
    const auto& selection = std::get<DrawTextCommand>(commands[4]);

    CHECK(base.origin == Point{11, 11});
    CHECK(base.text == "abcd");
    CHECK(!base.style.inverse);
    CHECK(base.clip_bounds == std::optional<Rect>{Rect{11, 11, 48, 12}});

    /*
     * Scalar 1 starts eight units after the text origin and scalar 3 sixteen units later. Replaying
     * the same complete shaped run with only this clip preserves shaping context while changing
     * appearance exclusively inside the semantic selection range.
     */
    CHECK(selection.origin == base.origin);
    CHECK(selection.text == base.text);
    CHECK(selection.style.inverse);
    CHECK(selection.clip_bounds == std::optional<Rect>{Rect{19, 11, 16, 12}});
}

TEST_CASE("Rendered focused TextField toggles selection contrast and preserves semantic style") {
    DisplayList display;
    SelectionMeasurementContext metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 120, 50});

    auto& field = window.emplace<TextField>("abcd");
    field.arrange({10, 10, 50, 14});

    TextStyle semantic_style;
    semantic_style.foreground = Color::bright_cyan;
    semantic_style.bold = true;
    field.setTextStyle(semantic_style);

    CHECK(focus.requestFocus(field));
    field.setSelection(1, 3);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 6);

    const auto& base = std::get<DrawTextCommand>(commands[3]);
    const auto& selection = std::get<DrawTextCommand>(commands[4]);

    CHECK(base.style.foreground == Color::bright_cyan);
    CHECK(base.style.bold);
    CHECK(base.style.inverse);

    /*
     * Focus already inverts the complete field. Selection therefore toggles that resolved visual
     * bit instead of forcing it on, otherwise the selected range would be indistinguishable from the
     * surrounding focused text. All unrelated style attributes remain intact.
     */
    CHECK(selection.style.foreground == Color::bright_cyan);
    CHECK(selection.style.bold);
    CHECK(!selection.style.inverse);
    CHECK(field.textStyle() == semantic_style);

    // Caret is emitted last so it remains visible over the selection replay.
    CHECK(std::holds_alternative<FillRectCommand>(commands[5]));
    CHECK(std::get<FillRectCommand>(commands[5]).role == FillRole::foreground);
}

TEST_CASE("Rendered TextField clips a selection that begins before the horizontal viewport") {
    DisplayList display;
    SelectionMeasurementContext metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 120, 50});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 20, 14}); // 18-unit interior, 17 units reserved for text.
    CHECK(focus.requestFocus(field));

    // The active end at scalar 4 scrolls away the first two eight-unit scalars.
    field.setSelection(0, 4);
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    const auto& base = std::get<DrawTextCommand>(commands[3]);
    const auto& selection = std::get<DrawTextCommand>(commands[4]);

    CHECK(base.origin == Point{-5, 11});
    CHECK(base.clip_bounds == std::optional<Rect>{Rect{11, 11, 18, 12}});

    /*
     * The semantic range covers scalars [0, 4), but only its visible tail from x=11 through x=27 is
     * replayed. Selection state remains scalar based; clipping is derived presentation geometry.
     */
    CHECK(selection.origin == base.origin);
    CHECK(selection.text == "abcdef");
    CHECK(selection.clip_bounds == std::optional<Rect>{Rect{11, 11, 16, 12}});
}

TEST_CASE("Rendered TextField selection metric failure defers before mutating DisplayList") {
    DisplayList display;
    UnsupportedSelectionBoundaryContext metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};

    display.drawText({1, 2}, "previous frame");

    Window window;
    auto& field = window.emplace<TextField>("abcd");
    field.arrange({10, 10, 50, 14});

    /*
     * Cursor 1 lets viewport/caret preflight succeed using boundaries 0 and 1. The stable anchor at
     * scalar 3 then requires a selection boundary the fake provider deliberately cannot express.
     */
    field.setSelection(3, 1);

    const auto result = sink.synchronize(field);

    CHECK(result == PresentationUpdateResult::deferred);
    CHECK(display.size() == 1);
    CHECK(std::get<DrawTextCommand>(display.commands()[0]).text == "previous frame");
    CHECK(field.isVisualUpdatePending());
}
