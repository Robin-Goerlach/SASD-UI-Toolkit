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
 * Deterministic rendered metrics for pointer-selection policy tests.
 *
 * Every Unicode scalar advances by eight logical units. These tests are about selection-anchor policy,
 * not SDL/font behavior, so a simple metric provider makes click positions exact and keeps the tests
 * portable across every compiler/OS in the normal Rendered test matrix.
 */
class ShiftClickMetrics final : public RenderedMeasurementContext {
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
        const std::size_t clamped =
            std::min(scalar_index, utf8::scalarCount(text));
        return static_cast<Coordinate>(clamped * 8U);
    }

    std::uint64_t revision() const noexcept override { return 1; }
};

} // namespace

TEST_CASE("Rendered TextField exact Shift press preserves the existing selection anchor") {
    ShiftClickMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 160, 60});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 80, 14});
    field.setSelection(1, 3);

    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {51, 15},
            PointerAction::press,
            PointerButton::primary,
            1,
            KeyModifier::shift},
        metrics);

    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 5);
    CHECK(field.selectionStart() == 1);
    CHECK(field.selectionEnd() == 5);
    CHECK(field.selectedText() == "bcde");

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{51, 15}, PointerAction::release, PointerButton::primary, 1},
        metrics);
    CHECK(!pointer_router.hasCapture());
}

TEST_CASE("Rendered TextField Shift press preserves directed anchor when extending across it") {
    ShiftClickMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 160, 60});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 80, 14});
    field.setSelection(4, 2);

    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {19, 15},
            PointerAction::press,
            PointerButton::primary,
            1,
            KeyModifier::shift},
        metrics);

    CHECK(press.handled);
    CHECK(field.selectionAnchor() == 4);
    CHECK(field.cursorPosition() == 1);
    CHECK(field.selectionStart() == 1);
    CHECK(field.selectionEnd() == 4);
    CHECK(field.selectedText() == "bcd");

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{19, 15}, PointerAction::release, PointerButton::primary, 1},
        metrics);
}

TEST_CASE("Rendered TextField drag after Shift press keeps the pre-existing anchor") {
    ShiftClickMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 160, 60});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 80, 14});
    field.setSelection(1, 2);

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {43, 15},
            PointerAction::press,
            PointerButton::primary,
            1,
            KeyModifier::shift},
        metrics);

    CHECK(pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 4);

    const auto move = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{27, 15}, PointerAction::move, PointerButton::none, 0},
        metrics);

    CHECK(move.handled);
    CHECK(move.capture_active);
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 2);
    CHECK(field.selectedText() == "b");

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{27, 15}, PointerAction::release, PointerButton::primary, 1},
        metrics);
    CHECK(!pointer_router.hasCapture());
}

TEST_CASE("Rendered TextField reserves combined modifiers instead of treating them as Shift extension") {
    ShiftClickMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 160, 60});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 80, 14});
    field.setSelection(1, 3);

    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {51, 15},
            PointerAction::press,
            PointerButton::primary,
            1,
            KeyModifier::shift | KeyModifier::control},
        metrics);

    CHECK(press.handled);
    CHECK(field.selectionAnchor() == 5);
    CHECK(field.cursorPosition() == 5);
    CHECK(!field.hasSelection());

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{51, 15}, PointerAction::release, PointerButton::primary, 1},
        metrics);
}

TEST_CASE("Rendered TextField scalar hit mapping distinguishes glyph spans from caret boundaries") {
    ShiftClickMetrics metrics;

    Window window;
    window.arrange({0, 0, 160, 60});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({10, 10, 80, 14});

    /*
     * The default one-unit border places the text viewport at x=11. Each scalar occupies an eight-unit
     * shaped span, so x=11..18 belongs to scalar zero and x=19 starts scalar one. This is deliberately
     * different from caretIndexAt(), whose midpoint rule may choose a neighboring insertion boundary.
     */
    CHECK(RenderedTextFieldHitTest::scalarIndexAt(field, {11, 15}, metrics) ==
          std::optional<std::size_t>{0});
    CHECK(RenderedTextFieldHitTest::scalarIndexAt(field, {18, 15}, metrics) ==
          std::optional<std::size_t>{0});
    CHECK(RenderedTextFieldHitTest::scalarIndexAt(field, {19, 15}, metrics) ==
          std::optional<std::size_t>{1});
    CHECK(RenderedTextFieldHitTest::scalarIndexAt(field, {34, 15}, metrics) ==
          std::optional<std::size_t>{2});

    // x=35 is exactly after the final shaped scalar; trailing viewport space is not text.
    CHECK(!RenderedTextFieldHitTest::scalarIndexAt(field, {35, 15}, metrics).has_value());
    CHECK(!RenderedTextFieldHitTest::scalarIndexAt(field, {60, 15}, metrics).has_value());
}

TEST_CASE("Rendered TextField scalar hit mapping follows the current horizontal viewport") {
    ShiftClickMetrics metrics;

    Window window;
    window.arrange({0, 0, 120, 60});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({10, 10, 20, 14});
    field.setCursorPosition(5);

    /*
     * The narrow field scrolls so later scalars are visible. scalarIndexAt() must use the shared
     * viewport builder instead of assuming scalar zero begins at the left edge. Whatever the exact
     * start chosen by the viewport policy, the leftmost visible shaped span must map to that scalar,
     * not to an off-screen predecessor.
     */
    const auto left_caret =
        RenderedTextFieldHitTest::caretIndexAt(field, {11, 15}, metrics);
    const auto left_scalar =
        RenderedTextFieldHitTest::scalarIndexAt(field, {11, 15}, metrics);

    CHECK(left_caret.has_value());
    CHECK(left_scalar.has_value());
    CHECK(*left_scalar == *left_caret);
}
