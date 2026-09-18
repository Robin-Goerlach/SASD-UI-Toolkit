#include <sasd/ui/terminal/terminal_presentation_sink.hpp>

#include <sasd/ui/button.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
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
 * Paints UTF-8 into one already-resolved Widget rectangle.
 *
 * Validation happens before clearRect(). This preserves the last successfully synchronized
 * representation if the current simple Cell model cannot faithfully represent the new text.
 */
PresentationUpdateResult renderUtf8(ScreenBuffer& buffer,
                                    const AbsoluteRect& rect,
                                    std::string_view text,
                                    AmbiguousWidthMode ambiguous_width,
                                    bool allow_multiline) noexcept {
    const TextMeasurement measurement = TextMetrics::measureUtf8(text, ambiguous_width);

    if (!measurement.simpleCellRenderable() ||
        (!allow_multiline && measurement.rows != 1)) {
        return PresentationUpdateResult::deferred;
    }

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
            // Preflight above guarantees this for supported visible text.
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
                 * Never emit half of a wide glyph. Both logical Widget columns and both physical
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

PresentationUpdateResult renderLabel(ScreenBuffer& buffer,
                                     const Label& label,
                                     AmbiguousWidthMode ambiguous_width) noexcept {
    const AbsoluteRect rect = absoluteRectOf(label);

    if (!label.isVisible() || rect.width <= 0 || rect.height <= 0) {
        clearRect(buffer, rect);
        return PresentationUpdateResult::synchronized;
    }

    return renderUtf8(buffer, rect, label.text(), ambiguous_width, true);
}

[[nodiscard]] std::string buttonPresentationText(const Button& button) {
    char left = '[';
    char right = ']';

    if (!button.isEnabled()) {
        left = '(';
        right = ')';
    } else if (button.hasFocus()) {
        left = '>';
        right = '<';
    }

    /*
     * All three states deliberately keep identical width:
     *
     *   [ caption ]  normal
     *   > caption <  focused
     *   ( caption )  disabled
     *
     * Stable chrome width means focus/enable transitions never require re-measurement.
     */
    std::string result;
    result.reserve(button.text().size() + 4);
    result.push_back(left);
    result.push_back(' ');
    result.append(button.text());
    result.push_back(' ');
    result.push_back(right);
    return result;
}

PresentationUpdateResult renderButton(ScreenBuffer& buffer,
                                      const Button& button,
                                      AmbiguousWidthMode ambiguous_width) {
    const AbsoluteRect rect = absoluteRectOf(button);

    if (!button.isVisible() || rect.width <= 0 || rect.height <= 0) {
        clearRect(buffer, rect);
        return PresentationUpdateResult::synchronized;
    }

    const std::string presentation = buttonPresentationText(button);

    /*
     * Initial terminal Buttons are intentionally single-line. Multi-line captions are accepted by the
     * semantic Button but this backend defers them rather than inventing incomplete border/chrome
     * rules. A later richer text/control layout can extend the backend without changing Button input
     * semantics.
     */
    return renderUtf8(buffer, rect, presentation, ambiguous_width, false);
}

} // namespace

PresentationUpdateResult TerminalPresentationSink::synchronize(const Widget& widget) {
    if (dynamic_cast<const Window*>(&widget) != nullptr) {
        /*
         * Window is the current terminal presentation root. Ordinary descendant state changes keep
         * rendering incremental, but geometry/removal requests a conservative subtree refresh. In
         * that case clearing the off-screen surface is safe because PresentationCoordinator will
         * immediately replay every descendant, including otherwise-clean siblings.
         */
        if (widget.isSubtreeRefreshPending()) {
            buffer_.clear();
        }
        return PresentationUpdateResult::synchronized;
    }

    if (const auto* button = dynamic_cast<const Button*>(&widget)) {
        return renderButton(buffer_, *button, ambiguous_width_);
    }

    if (const auto* label = dynamic_cast<const Label*>(&widget)) {
        return renderLabel(buffer_, *label, ambiguous_width_);
    }

    /*
     * Exact base objects and current Box layouts are structural primitives with no terminal cells of
     * their own. Do not generalize this to arbitrary subclasses: silently acknowledging a future
     * control before it has terminal rendering would lose a valid pending update.
     */
    if (typeid(widget) == typeid(Widget) ||
        typeid(widget) == typeid(Container) ||
        dynamic_cast<const VBox*>(&widget) != nullptr ||
        dynamic_cast<const HBox*>(&widget) != nullptr) {
        return PresentationUpdateResult::synchronized;
    }

    return PresentationUpdateResult::deferred;
}

} // namespace sasd::ui::terminal
