#include <sasd/ui/terminal/native_terminal_device.hpp>

#ifndef _WIN32

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace sasd::ui::terminal {
namespace {

[[nodiscard]] std::runtime_error systemError(std::string_view operation) {
    std::string message{operation};
    message.append(": ");
    message.append(std::strerror(errno));
    return std::runtime_error{message};
}

class PosixTerminalDevice final : public TerminalDevice {
public:
    [[nodiscard]] std::string_view name() const noexcept override {
        return "posix-tty";
    }

    [[nodiscard]] bool isInteractive() const noexcept override {
        return ::isatty(STDIN_FILENO) == 1 && ::isatty(STDOUT_FILENO) == 1;
    }

    [[nodiscard]] Size size() const override {
        struct winsize dimensions {};
        if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &dimensions) != 0) {
            throw systemError("ioctl(TIOCGWINSZ)");
        }

        if (dimensions.ws_col == 0 || dimensions.ws_row == 0) {
            throw std::runtime_error("terminal reported zero visible dimensions");
        }

        return {static_cast<Coordinate>(dimensions.ws_col),
                static_cast<Coordinate>(dimensions.ws_row)};
    }

    void beginSession(const TerminalSessionOptions& options) override {
        if (active_) {
            throw std::logic_error("POSIX terminal session is already active");
        }
        if (!isInteractive()) {
            throw std::runtime_error("stdin/stdout are not interactive TTYs");
        }

        TerminalState pending;
        pending.options = options;

        if (options.raw_input) {
            if (::tcgetattr(STDIN_FILENO, &pending.original_termios) != 0) {
                throw systemError("tcgetattr");
            }
            pending.termios_saved = true;

            struct termios raw = pending.original_termios;

            /*
             * Equivalent to the traditional cfmakeraw() policy, written explicitly because cfmakeraw
             * is not part of base POSIX and feature-test availability differs across libc/platforms.
             */
            raw.c_iflag &= static_cast<tcflag_t>(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
            raw.c_oflag &= static_cast<tcflag_t>(~OPOST);
            raw.c_cflag |= CS8;
            raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | IEXTEN | ISIG));
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;

            if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
                throw systemError("tcsetattr(raw)");
            }
            pending.raw_applied = true;
        }

        try {
            if (options.alternate_screen) {
                writeAll("\x1B[?1049h\x1B[?25l");
                pending.alternate_screen_entered = true;
            }

            state_ = pending;
            active_ = true;
        } catch (...) {
            rollback(pending);
            throw;
        }
    }

    void endSession() noexcept override {
        if (!active_) {
            return;
        }

        TerminalState ending = state_;
        active_ = false;
        state_ = {};

        /*
         * Restoration is best-effort because this method is called during RAII destruction. Restore
         * presentation before termios so a shell never inherits a hidden cursor/alternate screen.
         */
        if (ending.alternate_screen_entered) {
            writeAllNoThrow("\x1B[?25h\x1B[?1049l");
        } else {
            writeAllNoThrow("\x1B[?25h");
        }

        if (ending.raw_applied && ending.termios_saved) {
            (void)::tcsetattr(STDIN_FILENO, TCSAFLUSH, &ending.original_termios);
        }
    }

    void write(std::string_view bytes) override {
        if (!active_) {
            throw std::logic_error("POSIX terminal write requires an active session");
        }
        writeAll(bytes);
    }

    [[nodiscard]] std::string readAvailable() override {
        if (!active_) {
            throw std::logic_error("POSIX terminal read requires an active session");
        }

        std::string result;
        char buffer[256];

        for (;;) {
            struct pollfd descriptor {};
            descriptor.fd = STDIN_FILENO;
            descriptor.events = POLLIN;

            int poll_result = 0;
            do {
                poll_result = ::poll(&descriptor, 1, 0);
            } while (poll_result < 0 && errno == EINTR);

            if (poll_result < 0) {
                throw systemError("poll(stdin)");
            }
            if (poll_result == 0 || (descriptor.revents & POLLIN) == 0) {
                break;
            }

            ssize_t count = 0;
            do {
                count = ::read(STDIN_FILENO, buffer, sizeof(buffer));
            } while (count < 0 && errno == EINTR);

            if (count < 0) {
                throw systemError("read(stdin)");
            }
            if (count == 0) {
                break;
            }

            result.append(buffer, static_cast<std::size_t>(count));

            /*
             * poll() is checked again rather than assuming a short read means "drained". TTY line
             * disciplines and PTYs are allowed to return short chunks even when more bytes follow.
             */
        }

        return result;
    }

private:
    struct TerminalState {
        TerminalSessionOptions options{};
        struct termios original_termios {};
        bool termios_saved{false};
        bool raw_applied{false};
        bool alternate_screen_entered{false};
    };

    static void writeAll(std::string_view bytes) {
        std::size_t offset = 0;

        while (offset < bytes.size()) {
            const ssize_t written =
                ::write(STDOUT_FILENO,
                        bytes.data() + offset,
                        bytes.size() - offset);

            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                throw systemError("write(stdout)");
            }

            if (written == 0) {
                throw std::runtime_error("write(stdout) made no progress");
            }

            offset += static_cast<std::size_t>(written);
        }
    }

    static void writeAllNoThrow(std::string_view bytes) noexcept {
        std::size_t offset = 0;

        while (offset < bytes.size()) {
            const ssize_t written =
                ::write(STDOUT_FILENO,
                        bytes.data() + offset,
                        bytes.size() - offset);

            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return;
            }
            if (written == 0) {
                return;
            }

            offset += static_cast<std::size_t>(written);
        }
    }

    static void rollback(TerminalState& pending) noexcept {
        if (pending.alternate_screen_entered) {
            writeAllNoThrow("\x1B[?25h\x1B[?1049l");
        }
        if (pending.raw_applied && pending.termios_saved) {
            (void)::tcsetattr(STDIN_FILENO, TCSAFLUSH, &pending.original_termios);
        }
    }

    bool active_{false};
    TerminalState state_{};
};

} // namespace

std::unique_ptr<TerminalDevice> createNativeTerminalDevice() {
    return std::make_unique<PosixTerminalDevice>();
}

} // namespace sasd::ui::terminal

#endif
