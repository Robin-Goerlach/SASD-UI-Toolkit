#pragma once

#include <sasd/ui/geometry.hpp>

#include <string_view>

namespace sasd::ui::terminal {

/**
 * Session options shared by native terminal-device implementations.
 *
 * These describe portable intent. Platform adapters decide which termios/Win32 flags or ANSI session
 * sequences are required to provide the requested behavior.
 */
struct TerminalSessionOptions {
    /** Use the terminal's alternate screen so application UI does not overwrite shell history. */
    bool alternate_screen{true};

    /**
     * Configure immediate/raw-style keyboard input suitable for semantic event parsing.
     *
     * The current M2 step establishes/restores raw mode but input-byte parsing is implemented
     * separately. Keeping the option here makes the lifetime boundary explicit before reads begin.
     */
    bool raw_input{true};

    friend constexpr bool operator==(const TerminalSessionOptions&,
                                     const TerminalSessionOptions&) = default;
};

/**
 * Small byte-oriented operating-system terminal boundary.
 *
 * TerminalDevice owns no widgets, layouts or ScreenBuffer. Its job is only to manage the native TTY
 * session, expose current cell dimensions and transport already-encoded bytes.
 *
 * Implementations must make beginSession() transactional: when it throws, the terminal must be left
 * in/restored to its pre-call state. endSession() is noexcept because it is used from RAII teardown.
 */
class TerminalDevice {
public:
    virtual ~TerminalDevice() = default;

    /** Human-readable implementation name for diagnostics. */
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /** True when the device currently refers to a usable interactive terminal/console. */
    [[nodiscard]] virtual bool isInteractive() const noexcept = 0;

    /**
     * Returns the current visible terminal size in cells.
     *
     * @throws std::runtime_error when dimensions cannot be queried.
     */
    [[nodiscard]] virtual Size size() const = 0;

    /**
     * Enters one terminal UI session.
     *
     * Calling this twice without an intervening endSession() is an error. Implementations may enable
     * raw input, Virtual Terminal processing and alternate-screen mode according to options.
     */
    virtual void beginSession(const TerminalSessionOptions& options) = 0;

    /**
     * Restores all session-owned terminal state.
     *
     * Must be idempotent and noexcept; best-effort output restoration is preferable to throwing from
     * a destructor path.
     */
    virtual void endSession() noexcept = 0;

    /**
     * Writes every byte in bytes or throws.
     *
     * Short writes and interrupt/retry handling are implementation details; callers see an all-or-
     * exception operation so frame transport cannot silently truncate.
     */
    virtual void write(std::string_view bytes) = 0;
};

} // namespace sasd::ui::terminal
