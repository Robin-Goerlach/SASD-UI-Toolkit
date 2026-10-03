#pragma once

#include <sasd/ui/style.hpp>
#include <sasd/ui/widget.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace sasd::ui {

class Clipboard;

/**
 * Single-line editable UTF-8 text control.
 *
 * TextField keeps editing semantics backend-neutral. It stores valid single-line UTF-8 plus a cursor
 * position expressed as a Unicode-scalar index. Pixel/cell caret geometry and horizontal viewport
 * scrolling belong to presentation backends.
 *
 * M2 deliberately edits by Unicode scalar rather than grapheme cluster. Combining/ZWJ sequences are
 * retained in text but cursor movement can currently step through their individual scalars. Full
 * grapheme-aware editing is a later Unicode-text milestone and is documented as such rather than
 * being approximated silently.
 */
class TextField final : public Widget {
public:
    TextField();
    explicit TextField(std::string text);

    /** Returns guaranteed-valid, single-line UTF-8 content. */
    [[nodiscard]] std::string_view text() const noexcept { return text_; }

    /** Returns presentation-only text/chrome styling for this field. */
    [[nodiscard]] const TextStyle& textStyle() const noexcept { return text_style_; }

    /** Changes style without affecting text metrics or cursor position. */
    void setTextStyle(TextStyle style);

    /**
     * Replaces content after single-line UTF-8 sanitization.
     *
     * Malformed bytes become U+FFFD; C0/C1 controls and Unicode line separators are removed. The
     * existing scalar cursor position is preserved when possible and otherwise clamped to the new end.
     */
    void setText(std::string text);

    /** Returns the insertion cursor as a Unicode-scalar index in [0, scalarCount(text)]. */
    [[nodiscard]] std::size_t cursorPosition() const noexcept { return cursor_position_; }

    /** Moves the insertion cursor, clamping positions beyond the current text end. */
    void setCursorPosition(std::size_t scalar_index);

    /**
     * Reads UTF-8 text from a backend-neutral clipboard and inserts it at the current cursor.
     *
     * The clipboard service is supplied explicitly rather than retained by TextField. This keeps the
     * widget independent of Application/Backend lifetime and lets command or menu code decide which
     * clipboard instance is currently appropriate. Clipboard text passes through the same single-line
     * sanitization as TextInputEvent, so pasted line separators/control characters cannot violate the
     * TextField content invariant.
     *
     * The operation does not require logical focus: it is an explicit programmatic editing request,
     * analogous to setText(). It returns true only when sanitized text was actually inserted. Missing
     * clipboard text, an empty payload, or a payload that sanitizes to empty returns false without
     * invalidating measurement/presentation. Native clipboard read failures are intentionally allowed
     * to propagate according to the Clipboard contract; TextField is unchanged if readText() throws.
     *
     * Copy/cut are deliberately not guessed here. Their conventional behavior depends on a selection
     * model, which TextField does not yet expose.
     */
    [[nodiscard]] bool pasteFromClipboard(const Clipboard& clipboard);

protected:
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;

    /**
     * Handles TextInputEvent plus unmodified Left/Right/Home/End/Backspace/Delete key events.
     *
     * Editing requires logical focus and local visibility/enabled state. Key release for recognized
     * editing keys is consumed without repeating the operation.
     */
    [[nodiscard]] EventResult onEvent(const Event& event) override;

private:
    /** Inserts sanitized text and reports whether the content actually changed. */
    [[nodiscard]] bool insertText(std::string_view text);
    void eraseBeforeCursor();
    void eraseAtCursor();
    void moveCursorLeft();
    void moveCursorRight();

    /** Marks text-dependent size and presentation stale after an actual content mutation. */
    void textChanged() noexcept;

    std::string text_;
    TextStyle text_style_{};
    std::size_t cursor_position_{0};
};

} // namespace sasd::ui
