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
