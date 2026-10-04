#pragma once

#include <sasd/ui/geometry.hpp>

#include <string>
#include <string_view>

namespace sasd::ui::terminal {

/**
 * Session options shared by terminal-session and native-device lifetime code.
 *
 * These describe portable intent rather than exposing termios, Win32 or DEC/xterm details to callers.
 * Native TerminalDevice implementations decide which host-console flags are required for raw input and
 * ANSI/VT transport. TerminalSession may additionally own portable ANSI session protocols whose enable
 * and disable bytes must be paired with the same RAII lifetime, such as pointer reporting.
 */
struct TerminalSessionOptions {
    /** Use the terminal's alternate screen so application UI does not overwrite shell history. */
    bool alternate_screen{true};

    /**
     * Configure immediate/raw-style keyboard input suitable for semantic event parsing.
     *
     * Input-byte parsing is implemented separately. Keeping the option here makes the lifetime boundary
     * explicit before reads begin.
     */
    bool raw_input{true};

    /**
     * Request terminal pointer reports suitable for click-and-drag interaction.
     *
     * This is deliberately opt-in. When enabled, TerminalSession requests xterm-compatible button-event
     * tracking plus SGR coordinates for the lifetime of the session and restores the terminal on close.
     * The byte decoder remains a separate responsibility and merely understands reports that arrive.
     */
    bool pointer_input{false};

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
     * Enters one native terminal UI session.
     *
     * Calling this twice without an intervening endSession() is an error. Implementations may enable
     * raw input, Virtual Terminal processing and alternate-screen support according to the portable
     * options. Cross-platform ANSI protocols owned by TerminalSession are layered on top only after
     * this native transition succeeds.
     */
    virtual void beginSession(const TerminalSessionOptions& options) = 0;

    /**
     * Restores all native session-owned terminal state.
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

    /**
     * Returns all input bytes currently available without waiting for future input.
     *
     * An empty string means that no byte is currently ready, not end-of-session. Native
     * implementations must keep this operation non-blocking even when the underlying terminal is not
     * configured in raw mode.
     */
    [[nodiscard]] virtual std::string readAvailable() = 0;
};

} // namespace sasd::ui::terminal
