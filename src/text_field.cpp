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
    selection_anchor_ = cursor_position_;
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

    /*
     * Programmatic text replacement preserves the semantic scalar positions when possible. Clamping
     * anchor and cursor independently is important: a directed selection remains directed after the
     * content shrinks unless one or both endpoints are forced onto the new end.
     */
    const std::size_t scalar_count = utf8::scalarCount(text_);
    cursor_position_ = std::min(cursor_position_, scalar_count);
    selection_anchor_ = std::min(selection_anchor_, scalar_count);
    textChanged();
}

void TextField::setCursorPosition(std::size_t scalar_index) {
    const std::size_t clamped = std::min(scalar_index, utf8::scalarCount(text_));
    if (cursor_position_ == clamped && selection_anchor_ == clamped) {
        return;
    }

    cursor_position_ = clamped;
    selection_anchor_ = clamped;

    /*
     * Cursor/selection motion can change caret position, selection highlighting and a backend-specific
     * horizontal viewport, but it does not change intrinsic content size. Visual invalidation is
     * therefore sufficient.
     */
    invalidateVisual();
}

std::size_t TextField::selectionStart() const noexcept {
    return std::min(selection_anchor_, cursor_position_);
}

std::size_t TextField::selectionEnd() const noexcept {
    return std::max(selection_anchor_, cursor_position_);
}

void TextField::setSelection(std::size_t anchor_scalar_index,
                             std::size_t cursor_scalar_index) {
    const std::size_t scalar_count = utf8::scalarCount(text_);
    const std::size_t clamped_anchor = std::min(anchor_scalar_index, scalar_count);
    const std::size_t clamped_cursor = std::min(cursor_scalar_index, scalar_count);

    if (selection_anchor_ == clamped_anchor && cursor_position_ == clamped_cursor) {
        return;
    }

    selection_anchor_ = clamped_anchor;
    cursor_position_ = clamped_cursor;

    /*
     * Selection is presentation/editing state only. Measurement remains valid because neither the text
     * bytes nor their intrinsic geometry changed.
     */
    invalidateVisual();
}

void TextField::clearSelection() {
    if (!hasSelection()) {
        return;
    }

    selection_anchor_ = cursor_position_;
    invalidateVisual();
}

void TextField::selectAll() {
    /*
     * Keep complete-content selection on the same public scalar API as pointer and keyboard selection.
     * That matters for architecture: Ctrl+A, menu Select All and rendered triple-click can all express
     * intent through one Core operation instead of each caller knowing how TextField counts UTF-8 text.
     *
     * The operation deliberately creates a forward selection. Direction is observable through the
     * stable anchor/active cursor model, so choosing one canonical direction avoids accidental caller-
     * differences while still allowing later Shift navigation to continue from the end of the content.
     */
    setSelection(0, utf8::scalarCount(text_));
}

std::string TextField::selectedText() const {
    if (!hasSelection()) {
        return {};
    }

    const std::size_t begin =
        utf8::byteOffsetForScalarIndex(text_, selectionStart());
    const std::size_t end =
        utf8::byteOffsetForScalarIndex(text_, selectionEnd());
    return text_.substr(begin, end - begin);
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
     * Reuse the exact same replacement/insertion/sanitization path as TextInputEvent. Keeping one
     * mutation path is important: paste must not accidentally accept line separators or malformed
     * UTF-8 that normal keyboard/IME text input would sanitize away.
     */
    return insertText(*clipboard_text);
}

bool TextField::copySelectionToClipboard(Clipboard& clipboard) const {
    if (!hasSelection()) {
        /*
         * A collapsed selection is not equivalent to an empty payload. Writing an empty string here
         * would silently clear/replace a user's existing clipboard despite there being nothing to copy.
         * Returning false lets command/menu policy disable or ignore Copy without side effects.
         */
        return false;
    }

    /*
     * selectedText() returns an owned UTF-8 value, matching Clipboard's owned-value contract. No view
     * into TextField storage escapes to a backend that may retain or synchronously transform the bytes.
     * Clipboard failures intentionally propagate; Copy itself does not mutate this TextField.
     */
    clipboard.writeText(selectedText());
    return true;
}

bool TextField::cutSelectionToClipboard(Clipboard& clipboard) {
    if (!hasSelection()) {
        return false;
    }

    /*
     * Capture the selected text before touching editor state, then write it before deletion. Native
     * clipboard operations can fail (for example because another process temporarily owns/locks the
     * platform facility). If writeText() throws, the user's selected text and selection therefore remain
     * intact rather than being destroyed before we know the clipboard accepted the payload.
     */
    const std::string selection = selectedText();
    clipboard.writeText(selection);

    /*
     * No external callback occurs between the successful write and this local mutation. With a live
     * selection established above, eraseSelection() collapses at selectionStart() and cannot fail under
     * the TextField invariants. Keep the bool check anyway so this helper remains defensive if its
     * internal contract evolves later.
     */
    if (!eraseSelection()) {
        return false;
    }

    textChanged();
    return true;
}

Size TextField::onMeasure(const MeasurementContext& context, const MeasureConstraints&) {
    return context.measureTextField(text_);
}

EventResult TextField::onEvent(const Event& event) {
    /*
     * Pointer capture participation is intentionally checked before logical focus. Desktop hosts often
     * focus a TextField on the same press, but capture mechanics must not depend on focus policy or on
     * event-order details outside this control. Geometry remains outside Core: presentation-specific
     * code maps the pointer position to scalar indices and updates selection before routing the event.
     */
    if (const auto* pointer = std::get_if<PointerEvent>(&event)) {
        if (!isEnabled() || !isVisible()) {
            return EventResult::ignored;
        }

        if (pointer->action == PointerAction::press &&
            pointer->button == PointerButton::primary) {
            pointer_selection_active_ = true;
            return EventResult::handled;
        }

        if (!pointer_selection_active_) {
            return EventResult::ignored;
        }

        if (pointer->action == PointerAction::move) {
            /*
             * PointerRouter delivers captured motion directly to the field even after the geometric
             * pointer leaves its bounds. Consuming it here preserves capture ownership; scalar movement
             * has already been applied (when representable) by the rendered interaction layer.
             */
            return EventResult::handled;
        }

        if (pointer->action == PointerAction::release &&
            pointer->button == PointerButton::primary) {
            pointer_selection_active_ = false;
            return EventResult::handled;
        }

        return EventResult::ignored;
    }

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
    if (key == nullptr) {
        return EventResult::ignored;
    }

    const bool unmodified = key->modifiers == KeyModifier::none;
    const bool shift_only = key->modifiers == KeyModifier::shift;
    if (!unmodified && !shift_only) {
        /*
         * Control/Alt/Meta combinations remain available to application/window shortcut policy. This is
         * especially important while printable-key shortcuts are still being designed separately from
         * the TextField editing contract.
         */
        return EventResult::ignored;
    }

    const bool navigation_key =
        key->key == Key::left ||
        key->key == Key::right ||
        key->key == Key::home ||
        key->key == Key::end;

    const bool editing_key =
        navigation_key ||
        (unmodified &&
         (key->key == Key::backspace || key->key == Key::delete_forward));

    if (!editing_key) {
        /*
         * Shift modifies only navigation in this slice. In particular, Shift+Backspace/Delete is left
         * untouched rather than inventing platform-specific deletion aliases accidentally.
         */
        return EventResult::ignored;
    }

    // Desktop backends may report releases; consume recognized gestures without repeating the action.
    if (!key->pressed) {
        return EventResult::handled;
    }

    if (shift_only) {
        /*
         * Shift navigation moves only the active cursor and deliberately preserves selection_anchor_.
         * The first Shift movement starts from a collapsed anchor/cursor pair; repeated movements extend
         * or shrink the same directed selection. Crossing the anchor is valid and simply reverses the
         * normalized selection direction while the stable anchor stays unchanged.
         */
        const std::size_t scalar_count = utf8::scalarCount(text_);
        std::size_t target = cursor_position_;

        switch (key->key) {
        case Key::left:
            if (target > 0) {
                --target;
            }
            break;
        case Key::right:
            if (target < scalar_count) {
                ++target;
            }
            break;
        case Key::home:
            target = 0;
            break;
        case Key::end:
            target = scalar_count;
            break;
        default:
            break;
        }

        setSelection(selection_anchor_, target);
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

void TextField::onPointerCaptureLost() noexcept {
    /*
     * Capture may disappear without a matching release when a routing surface is left, the field is
     * detached/destroyed, or a host explicitly cancels the gesture. Retiring only transient gesture
     * ownership is important: the semantic selection itself remains exactly where the last trustworthy
     * pointer/key update left it.
     */
    pointer_selection_active_ = false;
}

bool TextField::insertText(std::string_view text) {
    /*
     * Sanitize before touching an existing selection. This gives rejected/empty input strict no-op
     * semantics: a clipboard containing only filtered controls must not erase selected user text.
     */
    const std::string sanitized = utf8::sanitizeSingleLine(text);
    if (sanitized.empty()) {
        return false;
    }

    const std::size_t insertion_scalar = selectionStart();
    const std::size_t begin =
        utf8::byteOffsetForScalarIndex(text_, insertion_scalar);
    const std::size_t end =
        utf8::byteOffsetForScalarIndex(text_, selectionEnd());

    if (end > begin) {
        text_.erase(begin, end - begin);
    }

    const std::size_t inserted_scalars = utf8::scalarCount(sanitized);
    text_.insert(begin, sanitized);
    cursor_position_ = insertion_scalar + inserted_scalars;
    selection_anchor_ = cursor_position_;
    textChanged();
    return true;
}

bool TextField::eraseSelection() {
    if (!hasSelection()) {
        return false;
    }

    const std::size_t start = selectionStart();
    const std::size_t begin = utf8::byteOffsetForScalarIndex(text_, start);
    const std::size_t end = utf8::byteOffsetForScalarIndex(text_, selectionEnd());

    text_.erase(begin, end - begin);
    cursor_position_ = start;
    selection_anchor_ = start;
    return true;
}

void TextField::eraseBeforeCursor() {
    if (eraseSelection()) {
        textChanged();
        return;
    }

    if (cursor_position_ == 0) {
        return;
    }

    const std::size_t begin =
        utf8::byteOffsetForScalarIndex(text_, cursor_position_ - 1);
    const std::size_t end =
        utf8::byteOffsetForScalarIndex(text_, cursor_position_);

    text_.erase(begin, end - begin);
    --cursor_position_;
    selection_anchor_ = cursor_position_;
    textChanged();
}

void TextField::eraseAtCursor() {
    if (eraseSelection()) {
        textChanged();
        return;
    }

    const std::size_t scalar_count = utf8::scalarCount(text_);
    if (cursor_position_ >= scalar_count) {
        return;
    }

    const std::size_t begin =
        utf8::byteOffsetForScalarIndex(text_, cursor_position_);
    const std::size_t end =
        utf8::byteOffsetForScalarIndex(text_, cursor_position_ + 1);

    text_.erase(begin, end - begin);
    selection_anchor_ = cursor_position_;
    textChanged();
}

void TextField::moveCursorLeft() {
    if (hasSelection()) {
        setCursorPosition(selectionStart());
        return;
    }

    if (cursor_position_ != 0) {
        setCursorPosition(cursor_position_ - 1);
    }
}

void TextField::moveCursorRight() {
    if (hasSelection()) {
        setCursorPosition(selectionEnd());
        return;
    }

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
