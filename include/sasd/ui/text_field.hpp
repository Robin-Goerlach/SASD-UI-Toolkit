#pragma once

#include <sasd/ui/widget.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace sasd::ui {

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
    void insertText(std::string_view text);
    void eraseBeforeCursor();
    void eraseAtCursor();
    void moveCursorLeft();
    void moveCursorRight();

    /** Marks text-dependent size and presentation stale after an actual content mutation. */
    void textChanged() noexcept;

    std::string text_;
    std::size_t cursor_position_{0};
};

} // namespace sasd::ui
