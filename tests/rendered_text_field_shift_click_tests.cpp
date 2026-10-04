#include "test_framework.hpp"

#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
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

    /*
     * The one-unit default border puts text x=0 at global x=11. Scalar five therefore lies at x=51.
     * Shift+press must keep anchor=1 and move only the active end to five. Collapsing at the click
     * would lose exactly the semantic state Shift is supposed to extend.
     */
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

    /*
     * A directed selection may have anchor > cursor. Shift+click must not normalize that pair before
     * extension because TextField intentionally stores direction for later keyboard/pointer changes.
     */
    field.setSelection(4, 2);

    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {19, 15}, // scalar 1
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
            {43, 15}, // scalar 4
            PointerAction::press,
            PointerButton::primary,
            1,
            KeyModifier::shift},
        metrics);

    CHECK(pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 4);

    /*
     * Once capture exists, later motion must continue the same Shift-started gesture. The helper must
     * not silently replace anchor=1 with the press boundary (4); only the active end follows motion.
     */
    const auto move = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{27, 15}, PointerAction::move, PointerButton::none, 0}, // scalar 2
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

    /*
     * Only exact Shift currently has defined extension semantics. Keeping Ctrl/Alt/Meta combinations
     * out of that rule leaves room for later word-selection/platform policy without changing today's
     * contract. Until such policy exists, the press follows the ordinary fresh-anchor behavior.
     */
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
