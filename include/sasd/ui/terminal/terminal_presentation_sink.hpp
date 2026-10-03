#pragma once

#include <sasd/ui/presentation/presentation_sink.hpp>
#include <sasd/ui/terminal/presentation_frame.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/text_metrics.hpp>

#include <optional>

namespace sasd::ui::terminal {

/**
 * First headless terminal presentation implementation.
 *
 * TerminalPresentationSink translates supported semantic widgets into an off-screen ScreenBuffer.
 * It intentionally performs no ANSI/VT I/O. This lets the complete Widget -> PresentationCoordinator
 * -> terminal-cell path run deterministically in unit tests on every supported operating system.
 *
 * Current M2 support is deliberately narrow:
 * - Window is a structural top-level root and has no terminal cells of its own;
 * - Label paints UTF-8 text into its arranged rectangle;
 * - Button and TextField provide the first terminal control chrome;
 * - TextField exposes a separate hardware-caret request rather than storing a fake caret glyph;
 * - exact base Widget/Container instances are structural and require no cells;
 * - unknown concrete widget types are deferred rather than silently acknowledged.
 *
 * Width is delegated to TextMetrics. Narrow and two-column glyphs are represented explicitly in
 * ScreenBuffer. Text containing zero-width/combining/control semantics that the current Cell model
 * cannot preserve is deferred before the existing buffer is modified.
 */
class TerminalPresentationSink final : public PresentationSink {
public:
    explicit TerminalPresentationSink(
        ScreenBuffer& buffer,
        AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) noexcept
        : buffer_{buffer}, ambiguous_width_{ambiguous_width} {}

    [[nodiscard]] ScreenBuffer& buffer() noexcept { return buffer_; }
    [[nodiscard]] const ScreenBuffer& buffer() const noexcept { return buffer_; }

    [[nodiscard]] AmbiguousWidthMode ambiguousWidthMode() const noexcept {
        return ambiguous_width_;
    }

    /**
     * Returns the currently requested terminal hardware-caret position, if any.
     *
     * Caret state is separate from ScreenBuffer glyph cells. A future ANSI/console writer can move
     * the real terminal cursor here without overwriting text with a fake caret character.
     */
    [[nodiscard]] const std::optional<Point>& caretPosition() const noexcept {
        return caret_position_;
    }

    /**
     * Captures the sink's current cells and hardware-caret request as one owned terminal frame.
     *
     * TerminalPresentationSink itself deliberately renders into caller-owned mutable storage. That is the
     * right contract while PresentationCoordinator is incrementally synchronizing widgets, but later
     * composition stages should not have to retain a reference to that mutable working surface. This method
     * therefore creates an explicit value snapshot at the presentation boundary.
     *
     * The returned TerminalPresentationFrame owns an independent copy of ScreenBuffer and copies the current
     * optional caret. Mutating either the sink's working buffer or the returned frame afterwards cannot affect
     * the other. This is particularly important for transient overlays: menu composition can safely treat the
     * captured frame as immutable base content while the next widget synchronization pass continues to reuse
     * the sink and its original buffer.
     *
     * The ambiguous-width policy is intentionally not copied into TerminalPresentationFrame. It is a policy
     * used while producing cell geometry; once cells and caret coordinates have been captured, the generic
     * frame represents the resulting presentation value rather than the process that created it.
     *
     * Copying the full buffer is intentionally accepted at this stage. A later optimization can introduce
     * move/reuse or damage-aware capture without weakening the owned-frame contract.
     */
    [[nodiscard]] TerminalPresentationFrame captureFrame() const {
        return TerminalPresentationFrame{buffer_, caret_position_};
    }

    [[nodiscard]] PresentationUpdateResult synchronize(const Widget& widget) override;

private:
    ScreenBuffer& buffer_;
    AmbiguousWidthMode ambiguous_width_{AmbiguousWidthMode::narrow};

    // Non-owning identity used only to clear stale caret state when that TextField later loses focus.
    const Widget* caret_owner_{nullptr};
    std::optional<Point> caret_position_;
};

} // namespace sasd::ui::terminal
