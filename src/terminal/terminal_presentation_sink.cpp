#include <sasd/ui/terminal/terminal_presentation_sink.hpp>

#include <sasd/ui/button.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

namespace sasd::ui::terminal {
namespace {

struct AbsoluteRect {
    std::int64_t x{0};
    std::int64_t y{0};
    std::int64_t width{0};
    std::int64_t height{0};
};

struct TextFieldRenderResult {
    PresentationUpdateResult update{PresentationUpdateResult::synchronized};
    std::optional<Point> caret;
};

struct ScalarLayout {
    char32_t value{U' '};
    int width{1};
    std::int64_t column{0};
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

void writeNarrowCell(ScreenBuffer& buffer,
                     std::int64_t x,
                     std::int64_t y,
                     char32_t value,
                     TextStyle style = {}) {
    if (isInsideBuffer(buffer, x, y)) {
        buffer.set({static_cast<Coordinate>(x), static_cast<Coordinate>(y)},
                   Cell{value, CellRole::normal, style});
    }
}

/**
 * Writes one already-measured scalar without ever emitting half of a wide glyph.
 */
void writeScalar(ScreenBuffer& buffer,
                 std::int64_t x,
                 std::int64_t y,
                 char32_t value,
                 int width,
                 TextStyle style = {}) {
    if (width == 1) {
        writeNarrowCell(buffer, x, y, value, style);
        return;
    }

    if (width == 2 &&
        isInsideBuffer(buffer, x, y) &&
        isInsideBuffer(buffer, x + 1, y)) {
        buffer.set({static_cast<Coordinate>(x), static_cast<Coordinate>(y)},
                   Cell{value, CellRole::wide_lead, style});
        buffer.set({static_cast<Coordinate>(x + 1), static_cast<Coordinate>(y)},
                   Cell{U' ', CellRole::wide_continuation, style});
    }
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
                                    bool allow_multiline,
                                    TextStyle style) noexcept {
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
            // Preflight above guarantees this branch is unreachable for supported visible text.
            continue;
        }

        if (x < rect.width && y < rect.height) {
            const std::int64_t screen_x = rect.x + x;
            const std::int64_t screen_y = rect.y + y;
            const bool fits_widget = width == 1 || x + 1 < rect.width;

            if (fits_widget) {
                writeScalar(buffer, screen_x, screen_y, decoded.value, width, style);
            }
        }

        x += width;
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

    return renderUtf8(buffer, rect, label.text(), ambiguous_width, true, label.textStyle());
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
     * Focus/disabled appearance is a terminal presentation overlay, not semantic widget state.
     * User-provided TextStyle remains the base; backend state can strengthen it without changing
     * measurement or mutating the Button.
     */
    TextStyle style = button.textStyle();
    if (!button.isEnabled()) {
        style.dim = true;
    } else if (button.hasFocus()) {
        style.inverse = true;
    }

    /*
     * Initial terminal Buttons are intentionally single-line. Multi-line captions are accepted by the
     * semantic Button but this backend defers them rather than inventing incomplete border/chrome
     * rules. A later richer text/control layout can extend the backend without changing Button input
     * semantics.
     */
    return renderUtf8(buffer, rect, presentation, ambiguous_width, false, style);
}

[[nodiscard]] std::vector<ScalarLayout> layoutTextScalars(
    std::string_view text,
    AmbiguousWidthMode ambiguous_width,
    std::int64_t& total_columns) {
    std::vector<ScalarLayout> result;
    result.reserve(text.size());

    total_columns = 0;

    for (std::size_t offset = 0; offset < text.size();) {
        const DecodedCodePoint decoded = TextMetrics::decodeOne(text, offset);
        if (decoded.consumed == 0) {
            break;
        }
        offset += decoded.consumed;

        const int width = TextMetrics::codePointWidth(decoded.value, ambiguous_width);

        /*
         * TextField preflight rejects non-renderable controls/zero-width sequences. Keep this helper
         * defensive anyway: invalid widths are omitted rather than corrupting column arithmetic.
         */
        if (width <= 0) {
            continue;
        }

        result.push_back({decoded.value, width, total_columns});
        total_columns += width;
    }

    return result;
}

TextFieldRenderResult renderTextField(ScreenBuffer& buffer,
                                      const TextField& field,
                                      AmbiguousWidthMode ambiguous_width) {
    const AbsoluteRect rect = absoluteRectOf(field);

    if (!field.isVisible() || rect.width <= 0 || rect.height <= 0) {
        clearRect(buffer, rect);
        return {PresentationUpdateResult::synchronized, std::nullopt};
    }

    const TextMeasurement measurement = TextMetrics::measureUtf8(field.text(), ambiguous_width);
    if (!measurement.simpleCellRenderable() || measurement.rows != 1) {
        /*
         * Do not clear old cells before discovering unsupported Unicode. The semantic TextField keeps
         * such Unicode losslessly; this backend stays pending until its Cell model can preserve it.
         */
        return {PresentationUpdateResult::deferred, std::nullopt};
    }

    clearRect(buffer, rect);

    TextStyle style = field.textStyle();
    if (!field.isEnabled()) {
        style.dim = true;
    } else if (field.hasFocus()) {
        style.inverse = true;
    }

    char left = '[';
    char right = ']';

    if (!field.isEnabled()) {
        left = '(';
        right = ')';
    } else if (field.hasFocus()) {
        left = '>';
        right = '<';
    }

    writeNarrowCell(buffer, rect.x, rect.y, static_cast<char32_t>(left), style);
    if (rect.width >= 2) {
        writeNarrowCell(buffer, rect.x + rect.width - 1, rect.y, static_cast<char32_t>(right), style);
    }

    const std::int64_t viewport_width = rect.width > 2 ? rect.width - 2 : 0;
    if (viewport_width <= 0) {
        return {PresentationUpdateResult::synchronized, std::nullopt};
    }

    std::int64_t total_columns = 0;
    const std::vector<ScalarLayout> scalars =
        layoutTextScalars(field.text(), ambiguous_width, total_columns);

    const std::size_t cursor_index = std::min(field.cursorPosition(), scalars.size());
    const std::int64_t cursor_column =
        cursor_index < scalars.size() ? scalars[cursor_index].column : total_columns;

    /*
     * Choose a scalar boundary that keeps the caret inside the visible interior. We never start in
     * the second cell of a wide glyph, so horizontal scrolling cannot manufacture half-glyph state.
     *
     * The caret is a terminal cursor position between Unicode scalars. Requiring relative caret
     * column < viewport_width reserves an actual terminal cell for it, including the end-of-text case.
     */
    std::size_t start_index = 0;
    while (start_index < cursor_index) {
        const std::int64_t candidate_column = scalars[start_index].column;
        if (cursor_column - candidate_column < viewport_width) {
            break;
        }
        ++start_index;
    }

    const std::int64_t start_column =
        start_index < scalars.size() ? scalars[start_index].column : total_columns;

    for (std::size_t index = start_index; index < scalars.size(); ++index) {
        const ScalarLayout& scalar = scalars[index];
        const std::int64_t relative = scalar.column - start_column;

        if (relative >= viewport_width) {
            break;
        }

        if (relative + scalar.width > viewport_width) {
            // Do not skip a clipped wide glyph and collapse following text into its logical space.
            break;
        }

        writeScalar(buffer,
                    rect.x + 1 + relative,
                    rect.y,
                    scalar.value,
                    scalar.width,
                    style);
    }

    std::optional<Point> caret;
    if (field.hasFocus()) {
        const std::int64_t relative_caret = cursor_column - start_column;
        const std::int64_t caret_x = rect.x + 1 + relative_caret;

        if (relative_caret >= 0 &&
            relative_caret < viewport_width &&
            isInsideBuffer(buffer, caret_x, rect.y)) {
            caret = Point{static_cast<Coordinate>(caret_x), static_cast<Coordinate>(rect.y)};
        }
    }

    return {PresentationUpdateResult::synchronized, caret};
}

} // namespace

PresentationUpdateResult TerminalPresentationSink::synchronize(const Widget& widget) {
    if (dynamic_cast<const Window*>(&widget) != nullptr) {
        /*
         * A structural refresh rebuilds the whole terminal presentation. Any remembered caret owner
         * may have moved or been removed, so clear it before forced descendants are replayed.
         *
         * For ordinary incremental updates we deliberately preserve caret state: the focused
         * TextField may be clean while an unrelated sibling changes.
         */
        if (widget.isSubtreeRefreshPending()) {
            buffer_.clear();
            caret_owner_ = nullptr;
            caret_position_.reset();
        }
        return PresentationUpdateResult::synchronized;
    }

    if (const auto* field = dynamic_cast<const TextField*>(&widget)) {
        const TextFieldRenderResult rendered =
            renderTextField(buffer_, *field, ambiguous_width_);

        if (field->hasFocus() &&
            rendered.update == PresentationUpdateResult::synchronized &&
            rendered.caret.has_value()) {
            caret_owner_ = field;
            caret_position_ = rendered.caret;
        } else if (caret_owner_ == field) {
            /*
             * Clear only when this field owns the remembered caret. During focus transfer, traversal
             * order may render the new focused field before the old one; the old field must not then
             * erase the new owner's caret.
             */
            caret_owner_ = nullptr;
            caret_position_.reset();
        }

        return rendered.update;
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
