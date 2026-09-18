#include <sasd/ui/text/utf8.hpp>

#include <cstdint>

namespace sasd::ui::utf8 {
namespace {

[[nodiscard]] constexpr bool isContinuation(unsigned char value) noexcept {
    return (value & 0xC0U) == 0x80U;
}

[[nodiscard]] constexpr bool isSingleLineControl(char32_t value) noexcept {
    const auto scalar = static_cast<std::uint32_t>(value);

    // C0/C1 controls include CR/LF/TAB/NUL. U+2028/U+2029 are Unicode line/paragraph separators.
    return scalar < 0x20U ||
           (scalar >= 0x7FU && scalar < 0xA0U) ||
           scalar == 0x2028U ||
           scalar == 0x2029U;
}

void appendReplacement(std::string& result) {
    // UTF-8 encoding of U+FFFD REPLACEMENT CHARACTER.
    result.append("\xEF\xBF\xBD", 3);
}

} // namespace

DecodedScalar decodeOne(std::string_view text, std::size_t offset) noexcept {
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

    return {U'\uFFFD', 1, false};
}

std::size_t scalarCount(std::string_view text) noexcept {
    std::size_t count = 0;

    for (std::size_t offset = 0; offset < text.size();) {
        const DecodedScalar decoded = decodeOne(text, offset);
        if (decoded.consumed == 0) {
            break;
        }
        offset += decoded.consumed;
        ++count;
    }

    return count;
}

std::size_t byteOffsetForScalarIndex(std::string_view text,
                                     std::size_t scalar_index) noexcept {
    std::size_t offset = 0;
    std::size_t index = 0;

    while (offset < text.size() && index < scalar_index) {
        const DecodedScalar decoded = decodeOne(text, offset);
        if (decoded.consumed == 0) {
            break;
        }
        offset += decoded.consumed;
        ++index;
    }

    return offset;
}

void appendScalar(std::string& output, char32_t value) {
    auto scalar = static_cast<std::uint32_t>(value);

    if (scalar > 0x10FFFFU || (scalar >= 0xD800U && scalar <= 0xDFFFU)) {
        appendReplacement(output);
        return;
    }

    if (scalar <= 0x7FU) {
        output.push_back(static_cast<char>(scalar));
        return;
    }

    if (scalar <= 0x7FFU) {
        output.push_back(static_cast<char>(0xC0U | (scalar >> 6U)));
        output.push_back(static_cast<char>(0x80U | (scalar & 0x3FU)));
        return;
    }

    if (scalar <= 0xFFFFU) {
        output.push_back(static_cast<char>(0xE0U | (scalar >> 12U)));
        output.push_back(static_cast<char>(0x80U | ((scalar >> 6U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | (scalar & 0x3FU)));
        return;
    }

    output.push_back(static_cast<char>(0xF0U | (scalar >> 18U)));
    output.push_back(static_cast<char>(0x80U | ((scalar >> 12U) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | ((scalar >> 6U) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | (scalar & 0x3FU)));
}

std::string sanitizeSingleLine(std::string_view text) {
    std::string result;
    result.reserve(text.size());

    for (std::size_t offset = 0; offset < text.size();) {
        const std::size_t source_offset = offset;
        const DecodedScalar decoded = decodeOne(text, offset);
        if (decoded.consumed == 0) {
            break;
        }
        offset += decoded.consumed;

        if (!decoded.valid) {
            appendReplacement(result);
            continue;
        }

        if (isSingleLineControl(decoded.value)) {
            continue;
        }

        result.append(text.substr(source_offset, decoded.consumed));
    }

    return result;
}

} // namespace sasd::ui::utf8
