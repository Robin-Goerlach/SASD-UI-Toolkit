#pragma once

#include <sasd/ui/presentation/presentation_sink.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>

namespace sasd::ui::terminal {

/**
 * First headless terminal presentation implementation.
 *
 * TerminalPresentationSink translates supported semantic widgets into an off-screen ScreenBuffer.
 * It intentionally performs no ANSI/VT I/O. This lets the complete Widget -> PresentationCoordinator
 * -> terminal-cell path run deterministically in unit tests on every supported operating system.
 *
 * Current M2 support is deliberately narrow:
 * - Window establishes/clears its rectangular terminal canvas;
 * - Label paints UTF-8 text into its arranged rectangle;
 * - exact base Widget/Container instances are structural and require no cells;
 * - unknown concrete widget types are deferred rather than silently acknowledged.
 *
 * Label rendering currently advances one cell per decoded Unicode code point. That is a temporary
 * M2 subset, not the final terminal-width model; grapheme clusters, combining marks and wide
 * characters require a dedicated display-width layer before v0.1.0 is considered complete.
 */
class TerminalPresentationSink final : public PresentationSink {
public:
    explicit TerminalPresentationSink(ScreenBuffer& buffer) noexcept : buffer_{buffer} {}

    [[nodiscard]] ScreenBuffer& buffer() noexcept { return buffer_; }
    [[nodiscard]] const ScreenBuffer& buffer() const noexcept { return buffer_; }

    [[nodiscard]] PresentationUpdateResult synchronize(const Widget& widget) override;

private:
    ScreenBuffer& buffer_;
};

} // namespace sasd::ui::terminal
