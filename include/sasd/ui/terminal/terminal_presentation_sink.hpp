#pragma once

#include <sasd/ui/presentation/presentation_sink.hpp>
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

    [[nodiscard]] PresentationUpdateResult synchronize(const Widget& widget) override;

private:
    ScreenBuffer& buffer_;
    AmbiguousWidthMode ambiguous_width_{AmbiguousWidthMode::narrow};

    // Non-owning identity used only to clear stale caret state when that TextField later loses focus.
    const Widget* caret_owner_{nullptr};
    std::optional<Point> caret_position_;
};

} // namespace sasd::ui::terminal
