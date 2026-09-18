#include <sasd/ui/terminal/terminal_presentation_sink.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/window.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <typeinfo>

namespace sasd::ui::terminal {
namespace {

constexpr char32_t replacement_character = U'\uFFFD';

struct AbsoluteRect {
    std::int64_t x{0};
    std::int64_t y{0};
    std::int64_t width{0};
    std::int64_t height{0};
};

/**
 * Resolves parent-relative Widget bounds to the terminal root coordinate system.
 *
 * The calculation is widened because several perfectly valid int32 parent offsets can overflow an
 * int32 accumulator. We only narrow after clipping against ScreenBuffer dimensions.
 */
AbsoluteRect absoluteRectOf(const Widget& widget) noexcept {
    AbsoluteRect result{
        static_cast<std::int64_t>(widget.bounds().x),
        static_cast<std::int64_t>(widget.bounds().y),
        static_cast<std::int64_t>(widget.bounds().width),
        static_cast<std::int64_t>(widget.bounds().height),
    };

    for (const Container* parent = widget.parent(); parent != nullptr; parent = parent->parent()) {
        result.x += static_cast<std::int64_t>(parent->bounds().x);
        result.y += static_cast<std::int64_t>(parent->bounds().y);
    }

    return result;
}

void clearRect(ScreenBuffer& buffer, const AbsoluteRect& rect) noexcept {
    if (rect.width <= 0 || rect.height <= 0 || buffer.empty()) {
        return;
    }

    const std::int64_t left = rect.x < 0 ? 0 : rect.x;
    const std::int64_t top = rect.y < 0 ? 0 : rect.y;
    const std::int64_t right_limit = static_cast<std::int64_t>(buffer.size().width);
    const std::int64_t bottom_limit = static_cast<std::int64_t>(buffer.size().height);

    const std::int64_t raw_right = rect.x + rect.width;
    const std::int64_t raw_bottom = rect.y + rect.height;
    const std::int64_t right = raw_right < right_limit ? raw_right : right_limit;
    const std::int64_t bottom = raw_bottom < bottom_limit ? raw_bottom : bottom_limit;

    if (left >= right || top >= bottom) {
        return;
    }

    for (std::int64_t y = top; y < bottom; ++y) {
        auto row = buffer.row(static_cast<Coordinate>(y));
        for (std::int64_t x = left; x < right; ++x) {
            row[static_cast<std::size_t>(x)] = Cell{};
        }
    }
}

struct DecodedCodePoint {
    char32_t value{replacement_character};
    std::size_t consumed{1};
};

bool isContinuation(unsigned char value) noexcept {
    return (value & 0xC0U) == 0x80U;
}

/**
 * Decodes exactly one UTF-8 scalar value.
 *
 * Malformed, overlong, surrogate and out-of-range sequences consume one byte and yield U+FFFD. The
 * one-byte recovery rule is intentionally deterministic and prevents malformed input from making the
 * renderer read past the supplied string.
 */
DecodedCodePoint decodeOne(std::string_view text, std::size_t offset) noexcept {
    const auto first = static_cast<unsigned char>(text[offset]);

    if (first <= 0x7FU) {
        return {static_cast<char32_t>(first), 1};
    }

    if (first >= 0xC2U && first <= 0xDFU && offset + 1 < text.size()) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        if (isContinuation(second)) {
            const char32_t value =
                static_cast<char32_t>(((first & 0x1FU) << 6U) | (second & 0x3FU));
            return {value, 2};
        }
    }

    if (first >= 0xE0U && first <= 0xEFU && offset + 2 < text.size()) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);

        const bool valid_second =
            isContinuation(second) &&
            !(first == 0xE0U && second < 0xA0U) && // overlong
            !(first == 0xEDU && second >= 0xA0U);  // UTF-16 surrogate range

        if (valid_second && isContinuation(third)) {
            const char32_t value = static_cast<char32_t>(
                ((first & 0x0FU) << 12U) |
                ((second & 0x3FU) << 6U) |
                (third & 0x3FU));
            return {value, 3};
        }
    }

    if (first >= 0xF0U && first <= 0xF4U && offset + 3 < text.size()) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);
        const auto fourth = static_cast<unsigned char>(text[offset + 3]);

        const bool valid_second =
            isContinuation(second) &&
            !(first == 0xF0U && second < 0x90U) && // overlong
            !(first == 0xF4U && second > 0x8FU);   // above U+10FFFF

        if (valid_second && isContinuation(third) && isContinuation(fourth)) {
            const char32_t value = static_cast<char32_t>(
                ((first & 0x07U) << 18U) |
                ((second & 0x3FU) << 12U) |
                ((third & 0x3FU) << 6U) |
                (fourth & 0x3FU));
            return {value, 4};
        }
    }

    return {};
}

void renderLabel(ScreenBuffer& buffer, const Label& label) noexcept {
    const AbsoluteRect rect = absoluteRectOf(label);

    /*
     * Clear the current label rectangle before writing. This removes stale trailing characters when
     * text becomes shorter and also handles the simple visible -> hidden case without requiring the
     * ScreenBuffer to remember previous text.
     */
    clearRect(buffer, rect);

    if (!label.isVisible() || rect.width <= 0 || rect.height <= 0) {
        return;
    }

    std::int64_t x = 0;
    std::int64_t y = 0;
    const std::string_view text = label.text();

    for (std::size_t offset = 0; offset < text.size();) {
        const DecodedCodePoint decoded = decodeOne(text, offset);
        offset += decoded.consumed;

        if (decoded.value == U'\r') {
            continue;
        }
        if (decoded.value == U'\n') {
            x = 0;
            ++y;
            if (y >= rect.height) {
                break;
            }
            continue;
        }

        if (x < rect.width && y < rect.height) {
            const std::int64_t screen_x = rect.x + x;
            const std::int64_t screen_y = rect.y + y;

            if (screen_x >= 0 && screen_y >= 0 &&
                screen_x < static_cast<std::int64_t>(buffer.size().width) &&
                screen_y < static_cast<std::int64_t>(buffer.size().height)) {
                buffer.set({static_cast<Coordinate>(screen_x), static_cast<Coordinate>(screen_y)},
                           Cell{decoded.value});
            }
        }

        ++x;

        /*
         * Continue decoding after the right edge until a newline or end-of-string. That preserves
         * logical line semantics without wrapping: text outside the arranged width is clipped.
         */
    }
}

} // namespace

PresentationUpdateResult TerminalPresentationSink::synchronize(const Widget& widget) {
    if (const auto* window = dynamic_cast<const Window*>(&widget)) {
        /*
         * Window establishes a clean background for its current client rectangle. Descendants are
         * visited after the Window by PresentationCoordinator's preorder traversal and paint on top.
         * Hidden Window still clears its rectangle so previously presented cells disappear.
         */
        clearRect(buffer_, absoluteRectOf(*window));
        return PresentationUpdateResult::synchronized;
    }

    if (const auto* label = dynamic_cast<const Label*>(&widget)) {
        renderLabel(buffer_, *label);
        return PresentationUpdateResult::synchronized;
    }

    /*
     * Exact base objects are structural primitives with no terminal pixels/cells of their own. Do
     * not generalize this to arbitrary subclasses: silently acknowledging a future Button before it
     * has terminal rendering would lose a valid pending update.
     */
    if (typeid(widget) == typeid(Widget) || typeid(widget) == typeid(Container)) {
        return PresentationUpdateResult::synchronized;
    }

    return PresentationUpdateResult::deferred;
}

} // namespace sasd::ui::terminal
