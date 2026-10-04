#include "test_framework.hpp"

#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_text_field_hit_test.hpp>
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
 * Deterministic metrics for captured scalar-span hit-testing.
 *
 * ASCII-like scalars advance by eight logical units and U+754C advances by sixteen. The provider is
 * deliberately simple: these tests validate viewport/clamping semantics rather than a particular font
 * engine, so every CI platform must see identical geometry.
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
     * The drag mapper may clamp only inside the *current* rendered viewport. It must not jump back to
     * the off-screen leading 'A'. The viewport has seventeen logical text units here: U+754C consumes
     * sixteen of them and the first unit of trailing 'B' is still genuinely painted. Because the
     * scalar mapper intentionally treats partially clipped positive-width spans as visible, a far-right
     * captured drag correctly clamps to scalar 2 rather than pretending that visible fragment does not
     * exist.
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
