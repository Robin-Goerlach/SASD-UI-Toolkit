#include "test_framework.hpp"

#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_text_field_pointer_selection.hpp>
#include <sasd/ui/text/selection_boundaries.hpp>
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

TEST_CASE("Basic word boundary policy keeps word-like Unicode together and splits punctuation") {
    const std::string_view text_value = "Gr\xC3\xB6\xC3\x9F" "e,world";

    const auto german = text::basicWordRangeAt(text_value, 2);
    CHECK(german.has_value());
    CHECK(german->start == 0);
    CHECK(german->end == 5);

    const auto comma = text::basicWordRangeAt(text_value, 5);
    CHECK(comma.has_value());
    CHECK(comma->start == 5);
    CHECK(comma->end == 6);

    const auto english = text::basicWordRangeAt(text_value, 8);
    CHECK(english.has_value());
    CHECK(english->start == 6);
    CHECK(english->end == 11);
}

TEST_CASE("Basic word boundary policy rejects whitespace and preserves combining marks in word run") {
    const std::string_view text_value = "a\xCC\x81 b"; // a + combining acute, space, b

    const auto accented = text::basicWordRangeAt(text_value, 1);
    CHECK(accented.has_value());
    CHECK(accented->start == 0);
    CHECK(accented->end == 2);

    CHECK(!text::basicWordRangeAt(text_value, 2).has_value());
    CHECK(!text::basicWordRangeAt(text_value, 4).has_value());
}

TEST_CASE("Rendered TextField unmodified double click selects the word under the painted scalar") {
    ShiftClickMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 200, 60});

    auto& field = window.emplace<TextField>("alpha beta");
    field.arrange({10, 10, 120, 14});

    /*
     * Text starts at x=11 and each scalar is eight units wide. x=47 lies in the right half of scalar
     * four ('a' in "alpha"). caretIndexAt() would choose boundary five, but scalarIndexAt() must still
     * identify scalar four so the double click selects "alpha" rather than treating the separator as
     * the semantic target.
     */
    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {47, 15},
            PointerAction::press,
            PointerButton::primary,
            2,
            KeyModifier::none},
        metrics);

    CHECK(press.handled);
    CHECK(!press.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionStart() == 0);
    CHECK(field.selectionEnd() == 5);
    CHECK(field.selectedText() == "alpha");

    /*
     * Atomic double-click selection retires capture after the press. The matching release therefore
     * cannot run ordinary captured-drag finalization and collapse the selected word back to a caret.
     */
    const auto release = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{47, 15}, PointerAction::release, PointerButton::primary, 2},
        metrics);

    CHECK(!release.capture_active);
    CHECK(field.selectedText() == "alpha");
}

TEST_CASE("Rendered TextField double click selects punctuation separately from adjacent words") {
    ShiftClickMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 200, 60});

    auto& field = window.emplace<TextField>("alpha,beta");
    field.arrange({10, 10, 120, 14});

    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {53, 15}, // scalar five, comma
            PointerAction::press,
            PointerButton::primary,
            2,
            KeyModifier::none},
        metrics);

    CHECK(press.handled);
    CHECK(!press.capture_active);
    CHECK(field.selectionStart() == 5);
    CHECK(field.selectionEnd() == 6);
    CHECK(field.selectedText() == ",");
}

TEST_CASE("Rendered TextField modified double click does not claim unmodified word-selection policy") {
    ShiftClickMetrics metrics;
    PointerRouter pointer_router;

    Window window;
    window.arrange({0, 0, 200, 60});

    auto& field = window.emplace<TextField>("alpha beta");
    field.arrange({10, 10, 120, 14});
    field.setSelection(0, 2);

    const auto press = RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {75, 15},
            PointerAction::press,
            PointerButton::primary,
            2,
            KeyModifier::control},
        metrics);

    /*
     * Ctrl+double-click has no contract yet. It intentionally follows ordinary fresh-anchor click
     * behavior rather than silently inheriting the unmodified word-selection policy.
     */
    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(field.selectionAnchor() == field.cursorPosition());
    CHECK(!field.hasSelection());

    (void)RenderedTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{75, 15}, PointerAction::release, PointerButton::primary, 2},
        metrics);
}
