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

/**
 * Deterministic metrics for captured scalar-span hit-testing and word-drag policy tests.
 *
 * ASCII-like scalars advance by eight logical units and U+754C advances by sixteen. The provider is
 * deliberately simple: these tests validate viewport/clamping/selection semantics rather than a
 * particular font engine, so every CI platform must see identical geometry.
 */
class ScalarDragMetrics : public RenderedMeasurementContext {
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
        std::size_t byte_offset = 0;
        std::size_t scalar = 0;

        while (byte_offset < text.size() && scalar < wanted) {
            const auto decoded = utf8::decodeOne(text, byte_offset);
            if (decoded.consumed == 0) {
                break;
            }

            byte_offset += decoded.consumed;
            ++scalar;
            advance = static_cast<Coordinate>(
                advance + (decoded.value == U'\u754C' ? 16 : 8));
        }

        return advance;
    }

    std::uint64_t revision() const noexcept override { return 1; }
};

class UnsupportedScalarDragMetrics final : public ScalarDragMetrics {
public:
    std::optional<Coordinate> textAdvanceToScalar(
        std::string_view text,
        std::size_t scalar_index) const override {
        if (scalar_index == 2) {
            return std::nullopt;
        }
        return ScalarDragMetrics::textAdvanceToScalar(text, scalar_index);
    }
};

} // namespace

TEST_CASE("RenderedTextFieldHitTest strict scalar mapping differs intentionally from caret boundaries") {
    ScalarDragMetrics metrics;

    Window window;
    window.arrange({0, 0, 160, 60});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 80, 14});

    /*
     * The default one-unit border places the text viewport at x=11. Each scalar occupies an
     * eight-unit half-open span: scalar zero is x=[11,19), scalar one is x=[19,27), and so on.
     *
     * The right half of scalar zero is the critical distinction. caretIndexAt() answers the nearest
     * insertion boundary, so x=18 is closer to boundary one. scalarIndexAt() instead answers which
     * painted scalar is physically under the pointer and therefore must still report scalar zero.
     * Multi-click selection depends on preserving this difference instead of reverse-engineering a
     * text scalar from caret placement.
     */
    CHECK(RenderedTextFieldHitTest::scalarIndexAt(field, {11, 15}, metrics) ==
          std::optional<std::size_t>{0});
    CHECK(RenderedTextFieldHitTest::scalarIndexAt(field, {18, 15}, metrics) ==
          std::optional<std::size_t>{0});
    CHECK(RenderedTextFieldHitTest::caretIndexAt(field, {18, 15}, metrics) ==
          std::optional<std::size_t>{1});
    CHECK(RenderedTextFieldHitTest::scalarIndexAt(field, {19, 15}, metrics) ==
          std::optional<std::size_t>{1});
    CHECK(RenderedTextFieldHitTest::scalarIndexAt(field, {34, 15}, metrics) ==
          std::optional<std::size_t>{2});

    /*
     * x=35 is exactly after the final shaped scalar. The editable viewport continues to the right,
     * but strict scalar hit-testing must not coerce either this boundary or later blank space onto the
     * final character. Captured scalarIndexForDrag() has a different contract and is tested below.
     */
    CHECK(!RenderedTextFieldHitTest::scalarIndexAt(field, {35, 15}, metrics).has_value());
    CHECK(!RenderedTextFieldHitTest::scalarIndexAt(field, {60, 15}, metrics).has_value());
}

TEST_CASE("RenderedTextFieldHitTest strict scalar mapping follows the shared horizontal viewport") {
    ScalarDragMetrics metrics;

    Window window;
    window.arrange({0, 0, 120, 60});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 20, 14});
    field.setCursorPosition(5);

    /*
     * This field is deliberately too narrow for the complete text, forcing the shared rendered
     * viewport to scroll toward the active cursor. Neither caret nor scalar hit testing may assume
     * scalar zero starts at the left content edge once that happens. At the leftmost visible shaped
     * position, both APIs must therefore resolve to the viewport's actual first visible scalar.
     *
     * We intentionally compare the APIs rather than hard-code the viewport-start index here. The
     * viewport policy owns that exact start decision; this regression protects the more important
     * architectural invariant that presentation and both hit-test modes consume the same viewport.
     */
    const auto left_caret =
        RenderedTextFieldHitTest::caretIndexAt(field, {11, 15}, metrics);
    const auto left_scalar =
        RenderedTextFieldHitTest::scalarIndexAt(field, {11, 15}, metrics);

    CHECK(left_caret.has_value());
    CHECK(left_scalar.has_value());
    CHECK(*left_scalar == *left_caret);
    CHECK(*left_scalar > 0);
}

TEST_CASE("RenderedTextFieldHitTest captured scalar drag clamps outside points to visible text") {
    ScalarDragMetrics metrics;

    Window window;
    window.arrange({0, 0, 140, 60});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 80, 14});

    /*
     * Strict scalarIndexAt() answers only whether a painted scalar is physically under the pointer.
     * Captured dragging has already established gesture ownership, so the drag variant deliberately
     * clamps positions well outside the Widget to the first/last visible positive-width scalar.
     */
    CHECK(!RenderedTextFieldHitTest::scalarIndexAt(field, {-100, -100}, metrics).has_value());
    CHECK(!RenderedTextFieldHitTest::scalarIndexAt(field, {1000, 1000}, metrics).has_value());

    CHECK(RenderedTextFieldHitTest::scalarIndexForDrag(field, {-100, -100}, metrics) ==
          std::optional<std::size_t>{0});
    CHECK(RenderedTextFieldHitTest::scalarIndexForDrag(field, {1000, 1000}, metrics) ==
          std::optional<std::size_t>{2});
}

TEST_CASE("RenderedTextFieldHitTest captured scalar drag maps trailing blank viewport space to final scalar") {
    ScalarDragMetrics metrics;

    Window window;
    window.arrange({0, 0, 140, 60});

    auto& field = window.emplace<TextField>("ab");
    field.arrange({10, 10, 80, 14});

    /*
     * Text occupies x=11..27 while the editable viewport continues much farther right. A normal
     * multi-click hit in that blank area must remain "no scalar under pointer". During an already
     * captured word drag, however, treating the same point as the final visible scalar gives stable
     * edge-extension behavior without fabricating an invisible glyph span.
     */
    CHECK(!RenderedTextFieldHitTest::scalarIndexAt(field, {60, 15}, metrics).has_value());
    CHECK(RenderedTextFieldHitTest::scalarIndexForDrag(field, {60, 15}, metrics) ==
          std::optional<std::size_t>{1});
}

TEST_CASE("RenderedTextFieldHitTest captured scalar drag respects current horizontal viewport") {
    ScalarDragMetrics metrics;

    Window window;
    window.arrange({0, 0, 100, 50});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"});
    field.arrange({10, 10, 20, 14});
    field.setCursorPosition(2); // Shared viewport starts at scalar 1 so U+754C is visible.

    /*
     * The drag mapper clamps only inside the *current* rendered viewport. The leading 'A' is off-screen,
     * so leftward capture clamps to U+754C (scalar 1). The text capacity is 17 units while U+754C uses
     * 16, leaving one visible unit of trailing 'B'; that partially painted positive-width span is an
     * honest visible target, so far-right capture correctly resolves to scalar 2.
     */
    CHECK(RenderedTextFieldHitTest::scalarIndexForDrag(field, {-100, 15}, metrics) ==
          std::optional<std::size_t>{1});
    CHECK(RenderedTextFieldHitTest::scalarIndexForDrag(field, {1000, 15}, metrics) ==
          std::optional<std::size_t>{2});
}

TEST_CASE("RenderedTextFieldHitTest captured scalar drag stays conservative for invalid geometry") {
    ScalarDragMetrics metrics;

    Window window;
    window.arrange({0, 0, 120, 60});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 50, 14});

    field.setEnabled(false);
    CHECK(!RenderedTextFieldHitTest::scalarIndexForDrag(field, {1000, 15}, metrics).has_value());

    field.setEnabled(true);
    UnsupportedScalarDragMetrics unsupported;
    CHECK(!RenderedTextFieldHitTest::scalarIndexForDrag(field, {1000, 15}, unsupported).has_value());

    auto& empty = window.emplace<TextField>();
    empty.arrange({10, 30, 50, 14});
    CHECK(!RenderedTextFieldHitTest::scalarIndexForDrag(empty, {-100, 35}, metrics).has_value());
}

TEST_CASE("Rendered TextField stateful double-click drag extends selection by complete word runs") {
    ScalarDragMetrics metrics;
    PointerRouter pointer_router;
    RenderedTextFieldPointerSelection::GestureState gesture_state;

    Window window;
    window.arrange({0, 0, 240, 60});

    auto& field = window.emplace<TextField>("alpha beta gamma");
    field.arrange({10, 10, 180, 14});

    /*
     * Text begins at x=11 with eight units per scalar. Scalar 7 lies in "beta", so the unmodified
     * double-click selects [6,10). Unlike the legacy stateless overload, the stateful overload keeps
     * PointerRouter capture alive because later motion is part of the same word-granular gesture.
     */
    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {69, 15},
            PointerAction::press,
            PointerButton::primary,
            2,
            KeyModifier::none},
        metrics,
        gesture_state);

    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(gesture_state.hasWordGesture());
    CHECK(field.selectionStart() == 6);
    CHECK(field.selectionEnd() == 10);
    CHECK(field.selectedText() == "beta");

    /*
     * Moving into "gamma" expands to that run's complete right boundary. The original word's left
     * edge remains the anchor, so the separator between beta/gamma is included without introducing a
     * character-granular endpoint inside either word.
     */
    const auto move_right = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{109, 15}, PointerAction::move, PointerButton::none, 0},
        metrics,
        gesture_state);

    CHECK(move_right.handled);
    CHECK(move_right.capture_active);
    CHECK(field.selectionAnchor() == 6);
    CHECK(field.cursorPosition() == 16);
    CHECK(field.selectedText() == "beta gamma");

    /*
     * Crossing back through the origin to "alpha" flips direction. Anchor moves to the original
     * word's right edge while the active cursor snaps to alpha's left boundary, preserving beta in
     * full even though the directed selection is now reversed.
     */
    const auto move_left = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{27, 15}, PointerAction::move, PointerButton::none, 0},
        metrics,
        gesture_state);

    CHECK(move_left.handled);
    CHECK(field.selectionAnchor() == 10);
    CHECK(field.cursorPosition() == 0);
    CHECK(field.selectedText() == "alpha beta");

    const auto release = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{27, 15}, PointerAction::release, PointerButton::primary, 2},
        metrics,
        gesture_state);

    CHECK(release.handled);
    CHECK(!release.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(!gesture_state.hasWordGesture());
    CHECK(field.selectedText() == "alpha beta");
}

TEST_CASE("Rendered TextField word drag keeps whole-word selection while pointer crosses whitespace") {
    ScalarDragMetrics metrics;
    PointerRouter pointer_router;
    RenderedTextFieldPointerSelection::GestureState gesture_state;

    Window window;
    window.arrange({0, 0, 220, 60});

    auto& field = window.emplace<TextField>("alpha beta gamma");
    field.arrange({10, 10, 180, 14});

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{69, 15}, PointerAction::press, PointerButton::primary, 2},
        metrics,
        gesture_state);

    CHECK(field.selectedText() == "beta");

    /*
     * Scalar 10 is the separator immediately after beta. basicWordRangeAt() intentionally does not
     * classify whitespace as a selectable word, so a word-granular drag does not degrade into a
     * character endpoint while crossing the gap. Selection advances only when another semantic run
     * is reached.
     */
    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{92, 15}, PointerAction::move, PointerButton::none, 0},
        metrics,
        gesture_state);

    CHECK(field.selectedText() == "beta");

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{109, 15}, PointerAction::move, PointerButton::none, 0},
        metrics,
        gesture_state);

    CHECK(field.selectedText() == "beta gamma");

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{109, 15}, PointerAction::release, PointerButton::primary, 2},
        metrics,
        gesture_state);
}

TEST_CASE("Rendered TextField word gesture state does not survive external capture loss") {
    ScalarDragMetrics metrics;
    PointerRouter pointer_router;
    RenderedTextFieldPointerSelection::GestureState gesture_state;

    Window window;
    window.arrange({0, 0, 220, 60});

    auto& field = window.emplace<TextField>("alpha beta gamma");
    field.arrange({10, 10, 180, 14});

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{69, 15}, PointerAction::press, PointerButton::primary, 2},
        metrics,
        gesture_state);

    CHECK(pointer_router.hasCapture());
    CHECK(gesture_state.hasWordGesture());
    CHECK(field.selectedText() == "beta");

    /*
     * Native surface leave/capture cancellation can retire PointerRouter ownership without passing a
     * matching release through this helper. The next event observes the authoritative no-capture state
     * and discards the stale semantic origin before it can affect future gestures.
     */
    pointer_router.leaveRoot();
    CHECK(!pointer_router.hasCapture());

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{109, 15}, PointerAction::move, PointerButton::none, 0},
        metrics,
        gesture_state);

    CHECK(!gesture_state.hasWordGesture());
    CHECK(field.selectedText() == "beta");

    /*
     * A fresh ordinary press/drag must now be character-granular, proving the cancelled word mode does
     * not leak into a later independent pointer gesture.
     */
    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{19, 15}, PointerAction::press, PointerButton::primary, 1},
        metrics,
        gesture_state);
    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{35, 15}, PointerAction::move, PointerButton::none, 0},
        metrics,
        gesture_state);

    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 3);
    CHECK(field.selectedText() == "lp");

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{35, 15}, PointerAction::release, PointerButton::primary, 1},
        metrics,
        gesture_state);
}
