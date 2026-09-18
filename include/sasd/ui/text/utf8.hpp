#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace sasd::ui::utf8 {

/**
 * Result of decoding one UTF-8 scalar.
 *
 * Invalid input is represented as U+FFFD and consumes exactly one source byte. This recovery rule
 * guarantees forward progress and is shared by semantic text editing and terminal presentation.
 */
struct DecodedScalar {
    char32_t value{U'\uFFFD'};
    std::size_t consumed{0};
    bool valid{false};
};

/** Decodes one scalar beginning at byte offset, or returns consumed == 0 at/past end. */
[[nodiscard]] DecodedScalar decodeOne(std::string_view text, std::size_t offset) noexcept;

/**
 * Counts Unicode scalar positions using decodeOne() recovery semantics.
 *
 * Every malformed source byte counts as one replacement scalar. The function is therefore total for
 * arbitrary byte strings and never gets stuck on invalid UTF-8.
 */
[[nodiscard]] std::size_t scalarCount(std::string_view text) noexcept;

/**
 * Returns the byte offset corresponding to a scalar index.
 *
 * Indices beyond the end clamp to text.size(). This lets controls clamp cursor positions without
 * exposing UTF-8 continuation-byte details to their public API.
 */
[[nodiscard]] std::size_t byteOffsetForScalarIndex(std::string_view text,
                                                   std::size_t scalar_index) noexcept;

/**
 * Produces valid single-line UTF-8 suitable for editable one-line controls.
 *
 * Malformed input bytes become UTF-8 U+FFFD. C0/C1 controls and Unicode line/paragraph separators are
 * removed. Printable Unicode, combining marks and format characters are retained; whether a backend
 * can render those faithfully remains a presentation capability decision.
 */
[[nodiscard]] std::string sanitizeSingleLine(std::string_view text);

} // namespace sasd::ui::utf8
