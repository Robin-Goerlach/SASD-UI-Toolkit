#pragma once

#include <sasd/ui/terminal/screen_buffer.hpp>

#include <optional>

namespace sasd::ui::terminal {

/**
 * Complete owned presentation state for one terminal frame.
 *
 * ScreenBuffer intentionally models only terminal cells. Some presentation state belongs to the frame
 * without being a glyph, most notably the hardware-caret request used by focused text controls. Keeping
 * both values in one small backend-level object gives composition code and transport code a shared value
 * contract without teaching ScreenBuffer about terminal-session behavior.
 *
 * This type is deliberately generic. Menus are one producer of a composed frame, but future overlays,
 * dialogs, status surfaces, or other terminal presentation stages can reuse the same buffer/caret pair.
 * No semantic Widget, MenuModel, TerminalSession, native handle, or ANSI byte stream is retained here.
 *
 * The object owns its ScreenBuffer. Copying therefore creates an independent frame; moving transfers the
 * complete presentation value. The optional caret uses the same cell coordinate system as the buffer.
 * A missing caret means that the terminal hardware cursor should not be exposed for this frame.
 */
struct TerminalPresentationFrame {
    ScreenBuffer buffer;
    std::optional<Point> caret{};
};

} // namespace sasd::ui::terminal
