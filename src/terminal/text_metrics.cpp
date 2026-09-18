#include <sasd/ui/terminal/text_metrics.hpp>

#include "unicode_width_tables.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace sasd::ui::terminal {
namespace {

using detail::CodePointRange;

template <std::size_t N>
[[nodiscard]] bool contains(const std::array<CodePointRange, N>& ranges,
                            char32_t value) noexcept {
    /*
     * The generated tables are sorted, non-overlapping ranges. Binary search keeps width lookup
     * logarithmic without pulling a heavyweight Unicode library into the first terminal backend.
     */
    std::size_t first = 0;
    std::size_t last = N;

    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        const auto& range = ranges[middle];

        if (value < range.first) {
            last = middle;
        } else if (value > range.last) {
            first = middle + 1;
        } else {
            return true;
        }
    }

    return false;
}

[[nodiscard]] constexpr bool isUnicodeScalar(char32_t value) noexcept {
    const auto scalar = static_cast<std::uint32_t>(value);
    return scalar <= 0x10FFFFU && !(scalar >= 0xD800U && scalar <= 0xDFFFU);
}

[[nodiscard]] constexpr bool isContinuation(unsigned char value) noexcept {
    return (value & 0xC0U) == 0x80U;
}

void addWidthSaturating(Coordinate& value, int width, bool& saturated) noexcept {
    const Coordinate maximum = std::numeric_limits<Coordinate>::max();

    if (width <= 0) {
        return;
    }

    if (value > maximum - width) {
        value = maximum;
        saturated = true;
        return;
    }

    value = static_cast<Coordinate>(value + width);
}

void incrementRowsSaturating(Coordinate& rows, bool& saturated) noexcept {
    const Coordinate maximum = std::numeric_limits<Coordinate>::max();
    if (rows == maximum) {
        saturated = true;
        return;
    }
    ++rows;
}

} // namespace

DecodedCodePoint TextMetrics::decodeOne(std::string_view text, std::size_t offset) noexcept {
    if (offset >= text.size()) {
        return {};
    }

    const auto first = static_cast<unsigned char>(text[offset]);

    if (first <= 0x7FU) {
        return {static_cast<char32_t>(first), 1, true};
    }

    if (first >= 0xC2U && first <= 0xDFU && offset + 1 < text.size()) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        if (isContinuation(second)) {
            const char32_t value =
                static_cast<char32_t>(((first & 0x1FU) << 6U) | (second & 0x3FU));
            return {value, 2, true};
        }
    }

    if (first >= 0xE0U && first <= 0xEFU && offset + 2 < text.size()) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);

        const bool valid_second =
            isContinuation(second) &&
            !(first == 0xE0U && second < 0xA0U) && // overlong three-byte sequence
            !(first == 0xEDU && second >= 0xA0U);  // UTF-16 surrogate range

        if (valid_second && isContinuation(third)) {
            const char32_t value = static_cast<char32_t>(
                ((first & 0x0FU) << 12U) |
                ((second & 0x3FU) << 6U) |
                (third & 0x3FU));
            return {value, 3, true};
        }
    }

    if (first >= 0xF0U && first <= 0xF4U && offset + 3 < text.size()) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);
        const auto fourth = static_cast<unsigned char>(text[offset + 3]);

        const bool valid_second =
            isContinuation(second) &&
            !(first == 0xF0U && second < 0x90U) && // overlong four-byte sequence
            !(first == 0xF4U && second > 0x8FU);   // above U+10FFFF

        if (valid_second && isContinuation(third) && isContinuation(fourth)) {
            const char32_t value = static_cast<char32_t>(
                ((first & 0x07U) << 18U) |
                ((second & 0x3FU) << 12U) |
                ((third & 0x3FU) << 6U) |
                (fourth & 0x3FU));
            return {value, 4, true};
        }
    }

    /*
     * Invalid/truncated UTF-8 consumes exactly one byte. This mirrors the previous terminal renderer
     * behavior while centralizing decoding for measurement and presentation.
     */
    return {U'\uFFFD', 1, false};
}

int TextMetrics::codePointWidth(char32_t value,
                                AmbiguousWidthMode ambiguous_width) noexcept {
    if (!isUnicodeScalar(value)) {
        return -1;
    }

    const auto scalar = static_cast<std::uint32_t>(value);

    // Printable ASCII is overwhelmingly common and can bypass all table lookups.
    if (scalar >= 0x20U && scalar < 0x7FU) {
        return 1;
    }

    // C0/C1 controls are not ordinary printable glyphs. NUL is represented by the zero-width table.
    if ((scalar != 0U && scalar < 0x20U) || (scalar >= 0x7FU && scalar < 0xA0U)) {
        return -1;
    }

    if (contains(detail::zero_width_ranges, value)) {
        return 0;
    }

    if (contains(detail::wide_ranges, value)) {
        return 2;
    }

    if (ambiguous_width == AmbiguousWidthMode::wide &&
        contains(detail::ambiguous_ranges, value)) {
        return 2;
    }

    return 1;
}

TextMeasurement TextMetrics::measureUtf8(std::string_view text,
                                         AmbiguousWidthMode ambiguous_width) noexcept {
    TextMeasurement result;
    Coordinate current_columns = 0;

    for (std::size_t offset = 0; offset < text.size();) {
        const DecodedCodePoint decoded = decodeOne(text, offset);
        if (decoded.consumed == 0) {
            break;
        }
        offset += decoded.consumed;

        if (!decoded.valid) {
            result.had_invalid_utf8 = true;
        }

        if (decoded.value == U'\r') {
            continue;
        }

        if (decoded.value == U'\n') {
            if (current_columns > result.columns) {
                result.columns = current_columns;
            }
            current_columns = 0;
            incrementRowsSaturating(result.rows, result.saturated);
            continue;
        }

        const int width = codePointWidth(decoded.value, ambiguous_width);

        if (decoded.value == U'\0' || width < 0) {
            result.contains_nonprinting_control = true;
            continue;
        }

        if (width == 0) {
            /*
             * Width zero is not automatically "bad Unicode"; it includes valid combining/format
             * characters. It does, however, require grapheme-aware cell storage that M2 does not yet
             * have, so preserve that fact for the presentation layer.
             */
            result.contains_zero_width = true;
            continue;
        }

        addWidthSaturating(current_columns, width, result.saturated);
    }

    if (current_columns > result.columns) {
        result.columns = current_columns;
    }

    return result;
}

} // namespace sasd::ui::terminal
