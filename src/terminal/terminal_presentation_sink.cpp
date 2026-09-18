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

[[nodiscard]] bool isInsideBuffer(const ScreenBuffer& buffer,
                                  std::int64_t x,
                                  std::int64_t y) noexcept {
    return x >= 0 && y >= 0 &&
           x < static_cast<std::int64_t>(buffer.size().width) &&
           y < static_cast<std::int64_t>(buffer.size().height);
}

/**
 * Renders a Label using the current simple-cell Unicode policy.
 *
 * Unsupported zero-width/control semantics are detected before clearRect(), so returning deferred
 * preserves the previously synchronized representation instead of partially destroying it.
 */
PresentationUpdateResult renderLabel(ScreenBuffer& buffer,
                                     const Label& label,
                                     AmbiguousWidthMode ambiguous_width) noexcept {
    const AbsoluteRect rect = absoluteRectOf(label);

    if (!label.isVisible() || rect.width <= 0 || rect.height <= 0) {
        clearRect(buffer, rect);
        return PresentationUpdateResult::synchronized;
    }

    const std::string_view text = label.text();
    const TextMeasurement measurement = TextMetrics::measureUtf8(text, ambiguous_width);

    if (!measurement.simpleCellRenderable()) {
        return PresentationUpdateResult::deferred;
    }

    /*
     * Clear before writing so a shorter replacement string removes stale trailing cells. Window no
     * longer clears the entire surface on every descendant update; doing so would erase clean sibling
     * widgets that PresentationCoordinator intentionally does not replay.
     */
    clearRect(buffer, rect);

    std::int64_t x = 0;
    std::int64_t y = 0;

    for (std::size_t offset = 0; offset < text.size();) {
        const DecodedCodePoint decoded = TextMetrics::decodeOne(text, offset);
        if (decoded.consumed == 0) {
            break;
        }
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

        const int width = TextMetrics::codePointWidth(decoded.value, ambiguous_width);
        if (width <= 0) {
            // Preflight above guarantees this branch is unreachable for supported visible text.
            continue;
        }

        if (x < rect.width && y < rect.height) {
            const std::int64_t screen_x = rect.x + x;
            const std::int64_t screen_y = rect.y + y;

            if (width == 1) {
                if (isInsideBuffer(buffer, screen_x, screen_y)) {
                    buffer.set({static_cast<Coordinate>(screen_x),
                                static_cast<Coordinate>(screen_y)},
                               Cell{decoded.value, CellRole::normal});
                }
            } else {
                /*
                 * Never emit half of a wide glyph. Both logical widget columns and both physical
                 * ScreenBuffer cells must be available; otherwise leave the clipped region blank and
                 * still advance by the glyph's logical width.
                 */
                const bool fits_widget = x + 1 < rect.width;
                const bool fits_buffer =
                    isInsideBuffer(buffer, screen_x, screen_y) &&
                    isInsideBuffer(buffer, screen_x + 1, screen_y);

                if (fits_widget && fits_buffer) {
                    buffer.set({static_cast<Coordinate>(screen_x),
                                static_cast<Coordinate>(screen_y)},
                               Cell{decoded.value, CellRole::wide_lead});
                    buffer.set({static_cast<Coordinate>(screen_x + 1),
                                static_cast<Coordinate>(screen_y)},
                               Cell{U' ', CellRole::wide_continuation});
                }
            }
        }

        x += width;

        /*
         * Continue decoding after the right edge until newline/end. Text does not wrap implicitly;
         * arranged width is a clip boundary rather than a line-breaking request.
         */
    }

    return PresentationUpdateResult::synchronized;
}

} // namespace

PresentationUpdateResult TerminalPresentationSink::synchronize(const Widget& widget) {
    if (dynamic_cast<const Window*>(&widget) != nullptr) {
        /*
         * Window is currently a structural presentation root. It deliberately does not clear the
         * complete ScreenBuffer: a descendant invalidation propagates to ancestors, and clearing here
         * would erase clean siblings that are not replayed in a pending-only coordinator pass.
         */
        return PresentationUpdateResult::synchronized;
    }

    if (const auto* label = dynamic_cast<const Label*>(&widget)) {
        return renderLabel(buffer_, *label, ambiguous_width_);
    }

    /*
     * Exact base objects are structural primitives with no terminal cells of their own. Do not
     * generalize this to arbitrary subclasses: silently acknowledging a future Button before it has
     * terminal rendering would lose a valid pending update.
     */
    if (typeid(widget) == typeid(Widget) || typeid(widget) == typeid(Container)) {
        return PresentationUpdateResult::synchronized;
    }

    return PresentationUpdateResult::deferred;
}

} // namespace sasd::ui::terminal
