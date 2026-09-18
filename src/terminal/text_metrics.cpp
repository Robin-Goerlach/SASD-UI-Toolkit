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
    /*
     * UTF-8 syntax/navigation is a core text concern now that TextField edits Unicode scalars.
     * Delegate to the shared utility so semantic editing and terminal measurement cannot disagree
     * about malformed-sequence recovery or scalar boundaries.
     */
    return utf8::decodeOne(text, offset);
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
