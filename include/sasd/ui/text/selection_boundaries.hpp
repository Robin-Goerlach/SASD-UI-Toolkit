#pragma once

#include <sasd/ui/text/utf8.hpp>

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace sasd::ui::text {

/**
 * Half-open Unicode-scalar range [start, end).
 *
 * The type intentionally carries scalar indices rather than UTF-8 byte offsets. TextField selection,
 * keyboard navigation and rendered pointer mapping all already use the same semantic index domain, so
 * this result can move between Core and presentation code without exposing encoding details.
 */
struct ScalarRange {
    std::size_t start{0};
    std::size_t end{0};

    friend constexpr bool operator==(const ScalarRange&, const ScalarRange&) = default;
};

namespace detail {

enum class BasicWordClass {
    whitespace,
    punctuation,
    word_like,
};

/**
 * Deterministic subset of Unicode White_Space used by the first M4 word-selection policy.
 *
 * TextField already removes line/control characters while sanitizing single-line input, but keeping
 * the complete commonly assigned White_Space set here makes the helper correct for callers that use
 * it on arbitrary UTF-8 and documents the intended semantic separator explicitly.
 */
[[nodiscard]] constexpr bool isBasicWordWhitespace(char32_t value) noexcept {
    return (value >= U'\u0009' && value <= U'\u000D') ||
           value == U'\u0020' ||
           value == U'\u0085' ||
           value == U'\u00A0' ||
           value == U'\u1680' ||
           (value >= U'\u2000' && value <= U'\u200A') ||
           value == U'\u2028' ||
           value == U'\u2029' ||
           value == U'\u202F' ||
           value == U'\u205F' ||
           value == U'\u3000';
}

/**
 * Recognizes punctuation ranges needed for useful deterministic pre-UAX#29 selection behavior.
 *
 * ASCII punctuation and the main Unicode punctuation blocks form their own runs instead of being
 * merged into surrounding letters. This is intentionally a modest, dependency-free M4 policy rather
 * than a claim to implement Unicode Standard Annex #29. Characters outside these punctuation ranges
 * are treated as word-like unless they are whitespace, which keeps accented letters, combining marks
 * and non-Latin scripts together without locale-dependent C library classification.
 */
[[nodiscard]] constexpr bool isBasicWordPunctuation(char32_t value) noexcept {
    const bool ascii_punctuation =
        (value >= U'\u0021' && value <= U'\u002F') ||
        (value >= U'\u003A' && value <= U'\u0040') ||
        (value >= U'\u005B' && value <= U'\u0060') ||
        (value >= U'\u007B' && value <= U'\u007E');

    const bool unicode_punctuation =
        (value >= U'\u2000' && value <= U'\u206F') ||
        (value >= U'\u2E00' && value <= U'\u2E7F') ||
        (value >= U'\u3001' && value <= U'\u303F') ||
        (value >= U'\uFF01' && value <= U'\uFF0F') ||
        (value >= U'\uFF1A' && value <= U'\uFF20') ||
        (value >= U'\uFF3B' && value <= U'\uFF40') ||
        (value >= U'\uFF5B' && value <= U'\uFF65');

    return ascii_punctuation || unicode_punctuation;
}

[[nodiscard]] constexpr BasicWordClass classifyBasicWordScalar(char32_t value) noexcept {
    if (isBasicWordWhitespace(value)) {
        return BasicWordClass::whitespace;
    }
    if (isBasicWordPunctuation(value)) {
        return BasicWordClass::punctuation;
    }
    return BasicWordClass::word_like;
}

} // namespace detail

/**
 * Returns the first M4 semantic word-selection run containing scalar_index.
 *
 * The helper is deliberately backend-neutral and deterministic. It does not use the current C locale,
 * font shaping or platform word-breaking APIs. For this pre-1.0 slice it groups adjacent scalars into
 * three classes:
 *
 * - Unicode whitespace;
 * - ASCII/main-Unicode punctuation;
 * - everything else as word-like text.
 *
 * Whitespace itself is not considered a word and therefore returns std::nullopt. Punctuation returns
 * the maximal adjacent punctuation run, while word-like text returns the maximal adjacent word-like
 * run. This gives useful double-click behavior for ordinary Latin text, accented/combining text and
 * non-Latin scripts without pretending to implement UAX #29. A future Unicode word-break subsystem can
 * replace this basic policy behind the same scalar-domain interaction architecture.
 *
 * Invalid UTF-8 follows utf8::decodeOne() recovery semantics: each malformed byte is represented as one
 * U+FFFD scalar. TextField content is already valid UTF-8, but total behavior keeps this utility safe for
 * other callers. scalar_index at/past the end returns std::nullopt.
 */
[[nodiscard]] inline std::optional<ScalarRange> basicWordRangeAt(
    std::string_view utf8_text,
    std::size_t scalar_index) {
    std::vector<char32_t> scalars;
    scalars.reserve(utf8::scalarCount(utf8_text));

    std::size_t byte_offset = 0;
    while (byte_offset < utf8_text.size()) {
        const auto decoded = utf8::decodeOne(utf8_text, byte_offset);
        if (decoded.consumed == 0) {
            break;
        }

        scalars.push_back(decoded.value);
        byte_offset += decoded.consumed;
    }

    if (scalar_index >= scalars.size()) {
        return std::nullopt;
    }

    const auto selected_class = detail::classifyBasicWordScalar(scalars[scalar_index]);
    if (selected_class == detail::BasicWordClass::whitespace) {
        return std::nullopt;
    }

    std::size_t start = scalar_index;
    while (start > 0 &&
           detail::classifyBasicWordScalar(scalars[start - 1]) == selected_class) {
        --start;
    }

    std::size_t end = scalar_index + 1;
    while (end < scalars.size() &&
           detail::classifyBasicWordScalar(scalars[end]) == selected_class) {
        ++end;
    }

    return ScalarRange{start, end};
}

} // namespace sasd::ui::text
