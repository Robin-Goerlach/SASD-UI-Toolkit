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
 * position and selection anchor expressed as Unicode-scalar indices. Pixel/cell caret geometry,
 * selection highlighting and horizontal viewport scrolling belong to presentation backends.
 *
 * The cursor is the active selection end. A collapsed selection has selectionAnchor() equal to
 * cursorPosition(). This anchor/cursor model preserves direction for later Shift+navigation without
 * introducing backend or input-policy state into the widget.
 *
 * M2/M4 deliberately edit by Unicode scalar rather than grapheme cluster. Combining/ZWJ sequences are
 * retained in text but cursor/selection boundaries can currently step through their individual
 * scalars. Full grapheme-aware editing is a later Unicode-text milestone and is documented as such
 * rather than being approximated silently.
 */
class TextField final : public Widget {
public:
    TextField();
    explicit TextField(std::string text);

    /** Returns guaranteed-valid, single-line UTF-8 content. */
    [[nodiscard]] std::string_view text() const noexcept { return text_; }

    /** Returns presentation-only text/chrome styling for this field. */
    [[nodiscard]] const TextStyle& textStyle() const noexcept { return text_style_; }

    /** Changes style without affecting text metrics, cursor or selection. */
    void setTextStyle(TextStyle style);

    /**
     * Replaces content after single-line UTF-8 sanitization.
     *
     * Malformed bytes become U+FFFD; C0/C1 controls and Unicode line separators are removed. Existing
     * cursor and selection-anchor scalar positions are preserved when possible and otherwise clamped to
     * the new end.
     */
    void setText(std::string text);

    /** Returns the active insertion/selection end as a Unicode-scalar index. */
    [[nodiscard]] std::size_t cursorPosition() const noexcept { return cursor_position_; }

    /**
     * Moves the insertion cursor and collapses any selection at the resulting position.
     *
     * Positions beyond the current text end are clamped. Cursor-only movement is presentation state and
     * therefore does not invalidate measurement.
     */
    void setCursorPosition(std::size_t scalar_index);

    /** Returns the stable selection anchor as a Unicode-scalar index. */
    [[nodiscard]] std::size_t selectionAnchor() const noexcept { return selection_anchor_; }

    /** Returns the lower scalar boundary of the current selection. */
    [[nodiscard]] std::size_t selectionStart() const noexcept;

    /** Returns the upper scalar boundary of the current selection. */
    [[nodiscard]] std::size_t selectionEnd() const noexcept;

    /** Returns whether anchor and active cursor describe a non-empty range. */
    [[nodiscard]] bool hasSelection() const noexcept {
        return selection_anchor_ != cursor_position_;
    }

    /**
     * Establishes a selection using Unicode-scalar anchor and active-cursor positions.
     *
     * Both positions are clamped independently to the current scalar count. Direction is preserved by
     * retaining the supplied anchor and cursor instead of normalizing them. Presentation backends may
     * use selectionStart()/selectionEnd() for the geometric range while input policy can later use the
     * original anchor to implement Shift+navigation correctly.
     */
    void setSelection(std::size_t anchor_scalar_index, std::size_t cursor_scalar_index);

    /** Collapses the current selection at cursorPosition() without changing text. */
    void clearSelection();

    /** Returns an owned UTF-8 copy of the selected range, or an empty string when collapsed. */
    [[nodiscard]] std::string selectedText() const;

    /**
     * Reads UTF-8 text from a backend-neutral clipboard and replaces the current selection or inserts
     * at the cursor when the selection is collapsed.
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
     * deleting an existing selection or invalidating measurement/presentation. Native clipboard read
     * failures are intentionally allowed to propagate according to the Clipboard contract; TextField is
     * unchanged if readText() throws.
     *
     * Copy/cut are deliberately deferred until the selection contract has also been integrated into the
     * surrounding command/menu surfaces and presentation backends.
     */
    [[nodiscard]] bool pasteFromClipboard(const Clipboard& clipboard);

protected:
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;

    /**
     * Handles TextInputEvent plus unmodified Left/Right/Home/End/Backspace/Delete key events.
     *
     * Editing requires logical focus and local visibility/enabled state. Key release for recognized
     * editing keys is consumed without repeating the operation. Unmodified navigation collapses a
     * programmatically established selection toward the requested edge before ordinary scalar movement.
     */
    [[nodiscard]] EventResult onEvent(const Event& event) override;

private:
    /**
     * Inserts sanitized text, replacing a non-empty selection first, and reports whether content changed.
     *
     * Sanitization deliberately happens before selection deletion. A payload that contains no insertable
     * single-line text is therefore a true no-op and cannot accidentally destroy the user's selection.
     */
    [[nodiscard]] bool insertText(std::string_view text);

    /** Removes the selected scalar range, collapses at its start, but performs no invalidation itself. */
    [[nodiscard]] bool eraseSelection();

    void eraseBeforeCursor();
    void eraseAtCursor();
    void moveCursorLeft();
    void moveCursorRight();

    /** Marks text-dependent size and presentation stale after an actual content mutation. */
    void textChanged() noexcept;

    std::string text_;
    TextStyle text_style_{};
    std::size_t cursor_position_{0};
    std::size_t selection_anchor_{0};
};

} // namespace sasd::ui
