#pragma once

#include <sasd/ui/terminal/terminal_device.hpp>

#include <memory>

namespace sasd::ui::terminal {

/**
 * Creates the platform-native terminal device for the current process stdin/stdout console.
 *
 * Construction itself does not enter raw/alternate-screen mode; TerminalSession owns that lifetime.
 * The returned device can therefore be created for diagnostics before deciding whether an interactive
 * terminal is available.
 */
[[nodiscard]] std::unique_ptr<TerminalDevice> createNativeTerminalDevice();

} // namespace sasd::ui::terminal
