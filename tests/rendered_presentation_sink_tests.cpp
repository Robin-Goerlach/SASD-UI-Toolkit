#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

/**
 * Deterministic proportional-ish font metrics for rendered presentation tests.
 *
 * The fake deliberately reports a wider CJK scalar and a zero-advance combining mark. That is enough
 * to prove RenderedPresentationSink consumes scalar-boundary advances supplied by a shaping/metric
 * provider instead of assuming UTF-8 byte count, scalar count or terminal cell width.
 */
class TestRenderedMeasurementContext : public RenderedMeasurementContext {
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
        const std::size_t wanted = std::min(scalar_index, utf8::scalarCount(utf8_text));

        Coordinate advance = 0;
        std::size_t offset = 0;
        std::size_t scalar = 0;

        while (offset < utf8_text.size() && scalar < wanted) {
            const utf8::DecodedScalar decoded = utf8::decodeOne(utf8_text, offset);
            if (decoded.consumed == 0) {
                break;
            }

            offset += decoded.consumed;
            ++scalar;

            if (decoded.value == U'\u0301') {
                // Combining acute accent extends the previous glyph without advancing the caret.
                continue;
            }

            advance = static_cast<Coordinate>(
                advance + (decoded.value == U'\u754C' ? 16 : 8));
        }

        return advance;
    }

    [[nodiscard]] std::uint64_t revision() const noexcept override {
        return 1;
    }
};

class WideChromeRenderedMeasurementContext final
    : public TestRenderedMeasurementContext {
public:
    [[nodiscard]] RenderedThemeMetrics themeMetrics() const noexcept override {
        return {
            2, // two logical units of rendered border
            2, // two-unit pressed caption offset
            2, // two-unit TextField caret width
        };
    }

    [[nodiscard]] std::uint64_t revision() const noexcept override {
        // A distinct revision documents that theme geometry participates in measurement identity.
        return 2;
    }
};

class UnsupportedCaretMeasurementContext final : public TestRenderedMeasurementContext {
public:
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view utf8_text,
        std::size_t scalar_index) const override {
        if (scalar_index == 1) {
            return std::nullopt;
        }
        return TestRenderedMeasurementContext::textAdvanceToScalar(utf8_text, scalar_index);
    }
};

} // namespace

TEST_CASE("RenderedPresentationSink builds deterministic Window and Label commands") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 320, 200});

    auto& panel = window.emplace<Container>();
    panel.arrange({10, 20, 200, 100});

    auto& label = panel.emplace<Label>("Hello");
    label.arrange({5, 6, 80, 20});

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.complete());
    CHECK(display.size() == 3);

    const auto commands = display.commands();

    // Window geometry changed before the first pass, so subtree refresh starts from a clean surface.
    CHECK(std::get<FillRectCommand>(commands[0]) ==
          FillRectCommand{Rect{0, 0, 320, 200}, Color::black});

    // Label coordinates are resolved through the visual-parent chain: (10 + 5, 20 + 6).
    CHECK(std::get<FillRectCommand>(commands[1]) ==
          FillRectCommand{Rect{15, 26, 80, 20}, Color::black});

    const auto& text = std::get<DrawTextCommand>(commands[2]);
    CHECK(text.origin == Point{15, 26});
    CHECK(text.text == "Hello");
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{15, 26, 80, 20}});
}

TEST_CASE("RenderedPresentationSink keeps ordinary Label repaint incremental") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 300, 120});
    auto& label = window.emplace<Label>("long old value");
    label.arrange({20, 10, 120, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    label.setText("new");

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(display.size() == 2);

    /*
     * Label::invalidateVisual() also marks Window pending, but it is not a subtree refresh. The sink
     * therefore clears only the Label rectangle, not the entire Window and unrelated clean siblings.
     */
    CHECK(std::get<FillRectCommand>(display.commands()[0]).bounds ==
          Rect{20, 10, 120, 20});
    CHECK(std::get<DrawTextCommand>(display.commands()[1]).text == "new");
}

TEST_CASE("RenderedPresentationSink rebuilds complete surface after geometry damage") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 300, 120});

    auto& first = window.emplace<Label>("first");
    first.arrange({10, 10, 80, 20});

    auto& second = window.emplace<Label>("second");
    second.arrange({10, 40, 80, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    first.arrange({30, 10, 80, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 5);

    // Root clear followed by both Labels proves the clean sibling was replayed by subtree refresh.
    CHECK(std::get<FillRectCommand>(commands[0]).bounds == Rect{0, 0, 300, 120});
    CHECK(std::get<FillRectCommand>(commands[1]).bounds == Rect{30, 10, 80, 20});
    CHECK(std::get<DrawTextCommand>(commands[2]).text == "first");
    CHECK(std::get<FillRectCommand>(commands[3]).bounds == Rect{10, 40, 80, 20});
    CHECK(std::get<DrawTextCommand>(commands[4]).text == "second");
}

TEST_CASE("RenderedPresentationSink renders Button chrome without mutating semantic style") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 200, 80});

    auto& button = window.emplace<Button>("Run");
    button.arrange({10, 10, 80, 30});

    TextStyle base;
    base.foreground = Color::bright_green;
    base.bold = true;
    button.setTextStyle(base);

    CHECK(focus.requestFocus(button));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 4);

    CHECK(std::get<FillRectCommand>(commands[1]).bounds == Rect{10, 10, 80, 30});
    CHECK(std::get<StrokeRectCommand>(commands[2]) ==
          StrokeRectCommand{Rect{10, 10, 80, 30}, Color::bright_green, 1});

    const auto& text = std::get<DrawTextCommand>(commands[3]);
    CHECK(text.origin == Point{11, 11});
    CHECK(text.text == "Run");
    CHECK(text.style.bold);
    CHECK(text.style.inverse);
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{11, 11, 78, 28}});

    // Focus is a presentation overlay. User-supplied semantic style remains unchanged.
    CHECK(button.textStyle() == base);
}

TEST_CASE("RenderedPresentationSink overlays Button hover without mutating semantic style") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};
    PointerRouter pointer;

    Window window;
    window.arrange({0, 0, 200, 80});

    auto& button = window.emplace<Button>("Run");
    button.arrange({10, 10, 80, 30});

    TextStyle base;
    base.foreground = Color::bright_green;
    button.setTextStyle(base);

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    display.clear();

    (void)pointer.route(
        window,
        PointerEvent{{20, 20}, PointerAction::move, PointerButton::none, 0});

    CHECK(button.isPointerOver());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 3);

    const auto& text = std::get<DrawTextCommand>(commands[2]);
    CHECK(text.style.foreground == Color::bright_green);
    CHECK(text.style.underline);

    // Hover is an ephemeral presentation overlay; user style remains untouched.
    CHECK(button.textStyle() == base);
}

TEST_CASE("RenderedPresentationSink offsets Button caption while pressed") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};
    PointerRouter pointer;

    Window window;
    window.arrange({0, 0, 200, 80});

    auto& button = window.emplace<Button>("Run");
    button.arrange({10, 10, 80, 30});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    CHECK(pointer.route(
        window,
        PointerEvent{{20, 20}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(button.isPressed());
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 3);
    const auto& text = std::get<DrawTextCommand>(commands[2]);
    CHECK(text.origin == Point{12, 12});
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{11, 11, 78, 28}});
    CHECK(button.textStyle() == TextStyle{});
}

TEST_CASE("RenderedPresentationSink rebuilds the surface when a Label becomes hidden") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 200, 80});
    auto& label = window.emplace<Label>("visible");
    label.arrange({15, 20, 100, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    label.setVisible(false);

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(display.size() == 1);

    /*
     * Visibility can uncover overlapping content, so it now requests the same conservative subtree
     * refresh as geometry damage. The Window therefore establishes one clean presentation surface;
     * the hidden Label is pruned from the forced replay instead of erasing content replayed below it.
     */
    CHECK(std::get<FillRectCommand>(display.commands()[0]) ==
          FillRectCommand{Rect{0, 0, 200, 80}, Color::black});
}

TEST_CASE("RenderedMeasurementContext keeps rendered control chrome consistent with metrics") {
    TestRenderedMeasurementContext metrics;

    Button button{"abc"};
    CHECK(button.measure(metrics) == Size{26, 14});

    TextField field{"abc"};
    CHECK(field.measure(metrics) == Size{27, 14});

    TextField empty;
    CHECK(empty.measure(metrics) == Size{3, 14});
}

TEST_CASE("Rendered theme metrics drive control measurement and Button presentation together") {
    WideChromeRenderedMeasurementContext metrics;

    Button measured_button{"abc"};
    CHECK(measured_button.measure(metrics) == Size{28, 16});

    TextField measured_field{"abc"};
    CHECK(measured_field.measure(metrics) == Size{30, 16});

    DisplayList display;
    RenderedPresentationSink sink{display, metrics, Color::black};
    PointerRouter pointer;

    Window window;
    window.arrange({0, 0, 200, 80});

    auto& button = window.emplace<Button>("Run");
    button.arrange({10, 10, 80, 30});

    CHECK(pointer.route(
        window,
        PointerEvent{{20, 20}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(button.isPressed());

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 4);

    /*
     * The same two-unit border that affected intrinsic size also defines the rendered content clip.
     * The pressed cue then applies its independent two-unit visual offset inside that content.
     */
    CHECK(std::get<StrokeRectCommand>(commands[2]) ==
          StrokeRectCommand{Rect{10, 10, 80, 30}, Color::default_color, 2});

    const auto& text = std::get<DrawTextCommand>(commands[3]);
    CHECK(text.origin == Point{14, 14});
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{12, 12, 76, 26}});
}

TEST_CASE("Rendered theme metrics keep TextField border viewport and caret coherent") {
    WideChromeRenderedMeasurementContext metrics;
    DisplayList display;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 120, 60});

    auto& field = window.emplace<TextField>("a");
    field.arrange({10, 10, 40, 16});
    CHECK(focus.requestFocus(field));

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 5);

    CHECK(std::get<StrokeRectCommand>(commands[2]) ==
          StrokeRectCommand{Rect{10, 10, 40, 16}, Color::default_color, 2});

    const auto& text = std::get<DrawTextCommand>(commands[3]);
    CHECK(text.origin == Point{12, 12});
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{12, 12, 36, 12}});

    // "a" advances by 8; the themed insertion caret starts at x=12+8 and is two units wide.
    CHECK(std::get<FillRectCommand>(commands[4]) ==
          FillRectCommand{
              Rect{20, 12, 2, 12},
              Color::default_color,
              FillRole::foreground});
}

TEST_CASE("RenderedPresentationSink renders focused TextField with clipped text and caret") {
    DisplayList display;
    TestRenderedMeasurementContext metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 200, 80});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 30, 14});
    CHECK(focus.requestFocus(field));

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.complete());
    CHECK(display.size() == 5);

    const auto commands = display.commands();
    CHECK(std::get<FillRectCommand>(commands[1]).bounds == Rect{10, 10, 30, 14});
    CHECK(std::get<StrokeRectCommand>(commands[2]).bounds == Rect{10, 10, 30, 14});

    const auto& text = std::get<DrawTextCommand>(commands[3]);
    CHECK(text.origin == Point{11, 11});
    CHECK(text.text == "abc");
    CHECK(text.style.inverse);
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{11, 11, 28, 12}});

    /*
     * Three 8-unit advances put the end caret at x=11+24. Caret height comes from the rendered font
     * context instead of filling an arbitrarily stretched TextField.
     */
    CHECK(std::get<FillRectCommand>(commands[4]) ==
          FillRectCommand{
              Rect{35, 11, 1, 12},
              Color::default_color,
              FillRole::foreground});
    CHECK(!field.isVisualUpdatePending());
}

TEST_CASE("RenderedPresentationSink scrolls TextField at scalar boundaries using full-run metrics") {
    DisplayList display;
    TestRenderedMeasurementContext metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 120, 50});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"});
    field.arrange({10, 10, 20, 14}); // 18-unit interior, 17 units reserved for text.
    CHECK(focus.requestFocus(field));
    field.setCursorPosition(2); // immediately after the 16-unit CJK scalar

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.complete());

    const auto commands = display.commands();
    const auto& text = std::get<DrawTextCommand>(commands[3]);

    /*
     * Full-run advances are A=8, 界=16. The 24-unit prefix cannot fit, so the viewport begins at the
     * scalar boundary after A. We still emit the complete text run and shift its origin left by 8;
     * clipping exposes the viewport without reshaping a suffix.
     */
    CHECK(text.origin == Point{3, 11});
    CHECK(text.text == std::string{"A\xE7\x95\x8C" "B"});
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{11, 11, 18, 12}});

    CHECK(std::get<FillRectCommand>(commands[4]).bounds == Rect{27, 11, 1, 12});
}

TEST_CASE("RenderedPresentationSink defers unsupported caret metrics transactionally") {
    DisplayList display;
    UnsupportedCaretMeasurementContext metrics;
    RenderedPresentationSink sink{display, metrics, Color::black};
    FocusManager focus;

    /*
     * Seed a prior successful frame command. The failing TextField preflight must not erase or append
     * anything before it discovers that the provider cannot express one caret boundary.
     */
    display.drawText({1, 2}, "previous frame");

    Window window;
    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 30, 14});
    CHECK(focus.requestFocus(field));
    field.setCursorPosition(2);

    const auto result = sink.synchronize(field);

    CHECK(result == PresentationUpdateResult::deferred);
    CHECK(display.size() == 1);
    CHECK(std::get<DrawTextCommand>(display.commands()[0]).text == "previous frame");
    CHECK(field.isVisualUpdatePending());
}

TEST_CASE("RenderedPresentationSink conservatively defers visible TextField") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 240, 100});
    auto& field = window.emplace<TextField>("Robin");
    field.arrange({10, 10, 160, 30});

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(!pass.complete());
    CHECK(pass.deferred == 1);

    /*
     * The root surface can still be cleared safely, but no partial TextField commands are emitted.
     * The metrics-free sink deliberately leaves the field pending; callers that need editable-text
     * presentation provide a RenderedMeasurementContext.
     */
    CHECK(display.size() == 1);
    CHECK(std::get<FillRectCommand>(display.commands()[0]).bounds ==
          Rect{0, 0, 240, 100});
    CHECK(field.isVisualUpdatePending());
}

TEST_CASE("RenderedPresentationSink defers unrepresentable accumulated coordinates") {
    DisplayList display;
    RenderedPresentationSink sink{display};

    Window window;
    window.arrange({std::numeric_limits<Coordinate>::max(), 0, 10, 10});
    auto& label = window.emplace<Label>("overflow");
    label.arrange({1, 0, 5, 5});

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(!pass.complete());
    CHECK(label.isVisualUpdatePending());
}
