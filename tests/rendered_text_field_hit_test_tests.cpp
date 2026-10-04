#include "test_framework.hpp"

#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_text_field_hit_test.hpp>
#include <sasd/ui/rendered/rendered_text_field_pointer_selection.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

class HitTestMetrics : public RenderedMeasurementContext {
public:
    Size measureText(std::string_view text) const override {
        return {
            textAdvanceToScalar(text, utf8::scalarCount(text)).value_or(0),
            lineHeight()};
    }

    Coordinate lineHeight() const noexcept override { return 12; }

    std::optional<Coordinate> textAdvanceToScalar(
        std::string_view text,
        std::size_t scalar_index) const override {
        const std::size_t wanted =
            std::min(scalar_index, utf8::scalarCount(text));

        Coordinate advance = 0;
        std::size_t offset = 0;
        std::size_t scalar = 0;

        while (offset < text.size() && scalar < wanted) {
            const auto decoded = utf8::decodeOne(text, offset);
            if (decoded.consumed == 0) {
                break;
            }

            offset += decoded.consumed;
            ++scalar;
            advance = static_cast<Coordinate>(
                advance + (decoded.value == U'\u754C' ? 16 : 8));
        }

        return advance;
    }

    std::uint64_t revision() const noexcept override { return 1; }
};

class ThemedHitTestMetrics final : public HitTestMetrics {
public:
    RenderedThemeMetrics themeMetrics() const noexcept override {
        return {2, 2, 2};
    }

    std::uint64_t revision() const noexcept override { return 2; }
};

class UnsupportedVisibleBoundaryMetrics final : public HitTestMetrics {
public:
    std::optional<Coordinate> textAdvanceToScalar(
        std::string_view text,
        std::size_t scalar_index) const override {
        if (scalar_index == 2) {
            return std::nullopt;
        }
        return HitTestMetrics::textAdvanceToScalar(text, scalar_index);
    }
};

} // namespace

TEST_CASE("RenderedTextFieldHitTest chooses nearest scalar boundary in visible field") {
    HitTestMetrics metrics;

    Window window;
    window.arrange({0, 0, 120, 60});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 40, 14});

    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {11, 15}, metrics) ==
          std::optional<std::size_t>{0});
    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {16, 15}, metrics) ==
          std::optional<std::size_t>{1});
    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {24, 15}, metrics) ==
          std::optional<std::size_t>{2});
    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {48, 15}, metrics) ==
          std::optional<std::size_t>{3});
}

TEST_CASE("RenderedTextFieldHitTest uses theme border geometry from the metric provider") {
    ThemedHitTestMetrics metrics;

    Window window;
    window.arrange({0, 0, 100, 50});

    auto& field = window.emplace<TextField>("ab");
    field.arrange({10, 10, 40, 16});

    /*
     * With a two-unit border the text viewport starts at global x=12. The midpoint to the first
     * eight-unit advance is therefore x=16; x=15 must still select scalar zero. A hard-coded
     * one-unit inset would incorrectly treat x=15 as the midpoint and select scalar one.
     */
    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {15, 15}, metrics) ==
          std::optional<std::size_t>{0});
    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {16, 15}, metrics) ==
          std::optional<std::size_t>{1});
}

TEST_CASE("RenderedTextFieldHitTest uses the same horizontally scrolled viewport as presentation") {
    HitTestMetrics metrics;

    Window window;
    window.arrange({0, 0, 100, 50});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"});
    field.arrange({10, 10, 20, 14}); // content width 18, caret text capacity 17.
    field.setCursorPosition(2);       // viewport starts after A so 界 caret can fit.

    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {10, 15}, metrics) ==
          std::optional<std::size_t>{1});
    /*
     * The visible CJK advance spans relative x=0..16. Relative x=8 is the exact midpoint and the
     * documented tie-break chooses the later scalar boundary, so global x is content.x(11)+8 = 19.
     */
    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {19, 15}, metrics) ==
          std::optional<std::size_t>{2});
    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {29, 15}, metrics) ==
          std::optional<std::size_t>{2});
}

TEST_CASE("RenderedTextFieldHitTest rejects outside disabled and unsupported mappings") {
    HitTestMetrics metrics;

    Window window;
    window.arrange({0, 0, 100, 50});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 40, 14});

    CHECK(!RenderedTextFieldHitTest::caretIndexAt(field, {60, 15}, metrics).has_value());

    field.setEnabled(false);
    CHECK(!RenderedTextFieldHitTest::caretIndexAt(field, {20, 15}, metrics).has_value());

    field.setEnabled(true);
    UnsupportedVisibleBoundaryMetrics unsupported;
    CHECK(!RenderedTextFieldHitTest::caretIndexAt(field, {20, 15}, unsupported).has_value());
}

TEST_CASE("RenderedTextFieldHitTest maps an empty editable field to scalar zero") {
    HitTestMetrics metrics;

    Window window;
    window.arrange({0, 0, 80, 40});

    auto& field = window.emplace<TextField>();
    field.arrange({10, 10, 30, 14});

    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {20, 15}, metrics) ==
          std::optional<std::size_t>{0});
}

TEST_CASE("RenderedTextFieldHitTest clamps captured drag positions outside the field") {
    HitTestMetrics metrics;

    Window window;
    window.arrange({0, 0, 100, 50});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 40, 14});

    /*
     * caretIndexAt() intentionally rejects both points because they are outside the Widget. During a
     * captured drag, however, ownership has already been established by an earlier press. The drag
     * mapper therefore clamps x to the nearest representable boundary and ignores y for this
     * single-line caret decision.
     */
    CHECK(!RenderedTextFieldHitTest::caretIndexAt(field, {-100, -100}, metrics).has_value());
    CHECK(!RenderedTextFieldHitTest::caretIndexAt(field, {1000, 1000}, metrics).has_value());

    CHECK(RenderedTextFieldHitTest::caretIndexForDrag(field, {-100, -100}, metrics) ==
          std::optional<std::size_t>{0});
    CHECK(RenderedTextFieldHitTest::caretIndexForDrag(field, {1000, 1000}, metrics) ==
          std::optional<std::size_t>{3});
}

TEST_CASE("RenderedTextFieldHitTest captured drag respects the current scrolled viewport") {
    HitTestMetrics metrics;

    Window window;
    window.arrange({0, 0, 100, 50});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"});
    field.arrange({10, 10, 20, 14});
    field.setCursorPosition(2); // current viewport begins at scalar 1.

    /*
     * A captured drag does not invent off-screen geometry. It can only return caret boundaries that
     * the current viewport/shaping provider can represent. Updating TextField with the returned
     * active end may scroll the viewport; the next motion is then evaluated against that new state.
     */
    CHECK(RenderedTextFieldHitTest::caretIndexForDrag(field, {-100, 15}, metrics) ==
          std::optional<std::size_t>{1});
    CHECK(RenderedTextFieldHitTest::caretIndexForDrag(field, {1000, 15}, metrics) ==
          std::optional<std::size_t>{2});
}

TEST_CASE("RenderedTextFieldHitTest captured drag remains conservative for disabled or unsupported fields") {
    HitTestMetrics metrics;

    Window window;
    window.arrange({0, 0, 100, 50});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 40, 14});

    field.setEnabled(false);
    CHECK(!RenderedTextFieldHitTest::caretIndexForDrag(field, {1000, 15}, metrics).has_value());

    field.setEnabled(true);
    UnsupportedVisibleBoundaryMetrics unsupported;
    CHECK(!RenderedTextFieldHitTest::caretIndexForDrag(field, {1000, 15}, unsupported).has_value());
}

TEST_CASE("RenderedTextFieldPointerSelection creates and extends a captured drag selection") {
    HitTestMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 160, 60});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 80, 14});

    /*
     * The field content begins at x=11 and every scalar advances by eight units. Pressing at x=19
     * therefore starts the gesture exactly at scalar 1. TextField handles the primary press, so the
     * ordinary PointerRouter capture protocol becomes the lifetime owner of the drag.
     */
    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{19, 15}, PointerAction::press, PointerButton::primary, 1},
        metrics);

    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 1);
    CHECK(!field.hasSelection());

    /*
     * Captured motion can leave ordinary hit-test semantics behind. The helper asks the rendered drag
     * mapper for scalar 4 and preserves the press boundary as the stable semantic anchor.
     */
    const auto move = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{43, 15}, PointerAction::move, PointerButton::none, 0},
        metrics);

    CHECK(move.handled);
    CHECK(move.capture_active);
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 4);
    CHECK(field.selectionStart() == 1);
    CHECK(field.selectionEnd() == 4);

    /*
     * Release performs one final geometry update before normal routing retires capture. The resulting
     * selection therefore reflects the actual release position rather than the previous move event.
     */
    const auto release = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{27, 15}, PointerAction::release, PointerButton::primary, 1},
        metrics);

    CHECK(release.handled);
    CHECK(!release.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 2);
    CHECK(field.selectedText() == "b");
}

TEST_CASE("RenderedTextFieldPointerSelection preserves anchor while dragging across it and outside bounds") {
    HitTestMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 160, 60});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 80, 14});

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{43, 15}, PointerAction::press, PointerButton::primary, 1},
        metrics); // scalar 4

    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(field.selectionAnchor() == 4);

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{19, 15}, PointerAction::move, PointerButton::none, 0},
        metrics); // scalar 1

    CHECK(field.selectionAnchor() == 4);
    CHECK(field.cursorPosition() == 1);
    CHECK(field.selectionStart() == 1);
    CHECK(field.selectionEnd() == 4);

    /*
     * Once capture exists, a point well left and vertically outside the field is still part of the
     * same gesture. The rendered drag mapper clamps to the first visible boundary instead of asking
     * Core TextField to understand pixels or font metrics.
     */
    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{-100, -100}, PointerAction::move, PointerButton::none, 0},
        metrics);

    CHECK(field.selectionAnchor() == 4);
    CHECK(field.cursorPosition() == 0);
    CHECK(field.selectedText() == "abcd");

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{-100, -100}, PointerAction::release, PointerButton::primary, 1},
        metrics);
    CHECK(!pointer_router.hasCapture());
}

TEST_CASE("RenderedTextFieldPointerSelection cancels capture when press geometry is unsupported") {
    UnsupportedVisibleBoundaryMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 120, 60});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 50, 14});
    field.setSelection(0, 1);

    const auto anchor_before = field.selectionAnchor();
    const auto cursor_before = field.cursorPosition();

    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{20, 15}, PointerAction::press, PointerButton::primary, 1},
        metrics);

    /*
     * The TextField still consumes its own primary press, but the rendered helper refuses to guess an
     * unavailable scalar boundary and immediately releases the transient capture. The pre-existing
     * semantic selection is therefore preserved and cannot become an accidental drag anchor.
     */
    CHECK(press.handled);
    CHECK(!press.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == anchor_before);
    CHECK(field.cursorPosition() == cursor_before);

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{45, 15}, PointerAction::move, PointerButton::none, 0},
        metrics);

    CHECK(field.selectionAnchor() == anchor_before);
    CHECK(field.cursorPosition() == cursor_before);
}

TEST_CASE("Rendered TextField pointer selection capture loss keeps the last semantic selection stable") {
    HitTestMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 160, 60});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 80, 14});

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{19, 15}, PointerAction::press, PointerButton::primary, 1},
        metrics);
    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{43, 15}, PointerAction::move, PointerButton::none, 0},
        metrics);

    CHECK(pointer_router.hasCapture());
    CHECK(field.selectedText() == "bcd");

    /*
     * Native surface loss may end capture without a release event. TextField's capture-lost hook
     * retires only transient gesture ownership; the user's last trustworthy semantic selection stays
     * intact. A later uncaptured move must not continue extending it.
     */
    pointer_router.leaveRoot();
    CHECK(!pointer_router.hasCapture());

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{75, 15}, PointerAction::move, PointerButton::none, 0},
        metrics);

    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 4);
    CHECK(field.selectedText() == "bcd");
}
