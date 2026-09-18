#pragma once

#include <sasd/ui/geometry.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>

#include <optional>
#include <string>

namespace sasd::ui::terminal {

/**
 * Deterministic full-frame ANSI/VT encoder.
 *
 * This class performs no operating-system I/O and owns no terminal session state. It only translates
 * the already-rendered ScreenBuffer plus an optional hardware-caret request into a byte string using
 * standard CSI cursor/visibility sequences and UTF-8 text.
 *
 * Alternate-screen mode, raw input mode, terminal-size discovery and writing bytes to stdout/console
 * belong to a later device/session layer.
 */
class AnsiFrameEncoder final {
public:
    AnsiFrameEncoder() = delete;

    /**
     * Encodes one complete frame.
     *
     * The frame hides the terminal cursor before drawing, clears the display, addresses every row
     * explicitly using 1-based ANSI coordinates, and finally restores/shows the cursor only when caret
     * lies inside buffer.
     *
     * wide_continuation cells are not emitted independently because the preceding wide_lead glyph
     * already advances a real terminal by two columns.
     */
    [[nodiscard]] static std::string encode(
        const ScreenBuffer& buffer,
        std::optional<Point> caret = std::nullopt);
};

} // namespace sasd::ui::terminal
