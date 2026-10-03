#include <sasd/ui/text_field.hpp>

#include <sasd/ui/clipboard.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/text/utf8.hpp>

#include <algorithm>
#include <utility>
#include <variant>

namespace sasd::ui {

TextField::TextField() {
    setFocusable(true);
}

TextField::TextField(std::string text)
    : TextField() {
    text_ = utf8::sanitizeSingleLine(text);
    cursor_position_ = utf8::scalarCount(text_);
}

void TextField::setTextStyle(TextStyle style) {
    if (text_style_ == style) {
        return;
    }

    text_style_ = style;
    invalidateVisual();
}

void TextField::setText(std::string text) {
    std::string sanitized = utf8::sanitizeSingleLine(text);

    if (text_ == sanitized) {
        return;
    }

    text_ = std::move(sanitized);
    cursor_position_ = std::min(cursor_position_, utf8::scalarCount(text_));
    textChanged();
}

void TextField::setCursorPosition(std::size_t scalar_index) {
    const std::size_t clamped = std::min(scalar_index, utf8::scalarCount(text_));
    if (cursor_position_ == clamped) {
        return;
    }

    cursor_position_ = clamped;

    /*
     * Cursor motion can change caret position and a backend-specific horizontal viewport, but it does
     * not change intrinsic content size. Visual invalidation is therefore sufficient.
     */
    invalidateVisual();
}

bool TextField::pasteFromClipboard(const Clipboard& clipboard) {
    /*
     * Clipboard returns an owned value, so no backend/native lifetime crosses into the mutation step.
     * Reading happens before we touch TextField state. If a native clipboard implementation throws,
     * the editor therefore remains exactly as it was before the paste request.
     */
    const auto clipboard_text = clipboard.readText();
    if (!clipboard_text.has_value()) {
        return false;
    }

    /*
     * Reuse the exact same insertion/sanitization path as TextInputEvent. Keeping one mutation path is
     * important: paste must not accidentally accept line separators or malformed UTF-8 that normal
     * keyboard/IME text input would sanitize away.
     */
    return insertText(*clipboard_text);
}

Size TextField::onMeasure(const MeasurementContext& context, const MeasureConstraints&) {
    return context.measureTextField(text_);
}

EventResult TextField::onEvent(const Event& event) {
    if (!hasFocus() || !isEnabled() || !isVisible()) {
        return EventResult::ignored;
    }

    if (const auto* text_input = std::get_if<TextInputEvent>(&event)) {
        /*
         * A focused editor owns textual input even if sanitization removes every supplied code point
         * (for example a pasted newline). Bubbling such text into a parent would be surprising and
         * could cause duplicate handling.
         */
        (void)insertText(text_input->text);
        return EventResult::handled;
    }

    const auto* key = std::get_if<KeyEvent>(&event);
    if (key == nullptr || key->modifiers != KeyModifier::none) {
        return EventResult::ignored;
    }

    const bool editing_key =
        key->key == Key::left ||
        key->key == Key::right ||
        key->key == Key::home ||
        key->key == Key::end ||
        key->key == Key::backspace ||
        key->key == Key::delete_forward;

    if (!editing_key) {
        return EventResult::ignored;
    }

    // Desktop backends may report releases; consume them without repeating the editing operation.
    if (!key->pressed) {
        return EventResult::handled;
    }

    switch (key->key) {
    case Key::left:
        moveCursorLeft();
        break;
    case Key::right:
        moveCursorRight();
        break;
    case Key::home:
        setCursorPosition(0);
        break;
    case Key::end:
        setCursorPosition(utf8::scalarCount(text_));
        break;
    case Key::backspace:
        eraseBeforeCursor();
        break;
    case Key::delete_forward:
        eraseAtCursor();
        break;
    default:
        break;
    }

    return EventResult::handled;
}

bool TextField::insertText(std::string_view text) {
    const std::string sanitized = utf8::sanitizeSingleLine(text);
    if (sanitized.empty()) {
        return false;
    }

    const std::size_t byte_offset =
        utf8::byteOffsetForScalarIndex(text_, cursor_position_);
    const std::size_t inserted_scalars = utf8::scalarCount(sanitized);

    text_.insert(byte_offset, sanitized);
    cursor_position_ += inserted_scalars;
    textChanged();
    return true;
}

void TextField::eraseBeforeCursor() {
    if (cursor_position_ == 0) {
        return;
    }

    const std::size_t begin =
        utf8::byteOffsetForScalarIndex(text_, cursor_position_ - 1);
    const std::size_t end =
        utf8::byteOffsetForScalarIndex(text_, cursor_position_);

    text_.erase(begin, end - begin);
    --cursor_position_;
    textChanged();
}

void TextField::eraseAtCursor() {
    const std::size_t scalar_count = utf8::scalarCount(text_);
    if (cursor_position_ >= scalar_count) {
        return;
    }

    const std::size_t begin =
        utf8::byteOffsetForScalarIndex(text_, cursor_position_);
    const std::size_t end =
        utf8::byteOffsetForScalarIndex(text_, cursor_position_ + 1);

    text_.erase(begin, end - begin);
    textChanged();
}

void TextField::moveCursorLeft() {
    if (cursor_position_ != 0) {
        setCursorPosition(cursor_position_ - 1);
    }
}

void TextField::moveCursorRight() {
    const std::size_t scalar_count = utf8::scalarCount(text_);
    if (cursor_position_ < scalar_count) {
        setCursorPosition(cursor_position_ + 1);
    }
}

void TextField::textChanged() noexcept {
    invalidateMeasure();
    invalidateVisual();
}

} // namespace sasd::ui
