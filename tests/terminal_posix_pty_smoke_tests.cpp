#include <sasd/ui/terminal/native_terminal_device.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/terminal_session.hpp>

#ifndef _WIN32

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace {

using namespace std::chrono_literals;
using namespace sasd::ui;
using namespace sasd::ui::terminal;

[[noreturn]] void fail(std::string_view message) {
    throw std::runtime_error(std::string{message});
}

void check(bool condition, std::string_view message) {
    if (!condition) {
        fail(message);
    }
}

std::string systemMessage(std::string_view operation) {
    std::string result{operation};
    result.append(": ");
    result.append(std::strerror(errno));
    return result;
}

void writeAll(int descriptor, std::string_view bytes) {
    std::size_t offset = 0;

    while (offset < bytes.size()) {
        const ssize_t written =
            ::write(descriptor, bytes.data() + offset, bytes.size() - offset);

        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(systemMessage("write"));
        }
        if (written == 0) {
            fail("write made no progress");
        }

        offset += static_cast<std::size_t>(written);
    }
}

std::string drainMaster(int master) {
    std::string result;
    char buffer[512];

    for (;;) {
        const ssize_t count = ::read(master, buffer, sizeof(buffer));

        if (count > 0) {
            result.append(buffer, static_cast<std::size_t>(count));
            continue;
        }

        if (count == 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        /*
         * Linux commonly reports EIO on a PTY master after the slave disappears; macOS typically
         * returns EOF. Both mean the child closed the terminal and there is no more output to drain.
         */
        if (errno == EIO || errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }

        throw std::runtime_error(systemMessage("read PTY master"));
    }

    return result;
}

bool equivalentTerminalState(const termios& left, const termios& right) {
    if (left.c_iflag != right.c_iflag ||
        left.c_oflag != right.c_oflag ||
        left.c_cflag != right.c_cflag ||
        left.c_lflag != right.c_lflag) {
        return false;
    }

    for (std::size_t index = 0; index < NCCS; ++index) {
        if (left.c_cc[index] != right.c_cc[index]) {
            return false;
        }
    }

    return true;
}

int childMain(int slave, int ready_pipe_write) {
    try {
        /*
         * Become a session leader and attach the PTY slave as our controlling terminal. Linux is
         * permissive enough for many termios operations without this step; macOS more closely follows
         * traditional terminal job-control rules. This also models a real interactive shell process
         * more faithfully than merely dup2()-ing an arbitrary tty descriptor.
         */
        if (::setsid() < 0) {
            return 18;
        }
        if (::ioctl(slave, TIOCSCTTY, 0) != 0) {
            return 19;
        }

        if (::dup2(slave, STDIN_FILENO) < 0 ||
            ::dup2(slave, STDOUT_FILENO) < 0) {
            return 20;
        }

        if (slave != STDIN_FILENO && slave != STDOUT_FILENO) {
            ::close(slave);
        }

        auto device = createNativeTerminalDevice();
        check(device != nullptr, "native terminal factory returned null");
        check(device->isInteractive(), "PTY-backed stdin/stdout must be interactive");

        {
            TerminalSession session{*device};

            check(session.size() == Size{42, 11},
                  "native terminal size must come from PTY window dimensions");

            /*
             * Signal readiness only after TerminalSession has switched the slave into raw mode and
             * entered the alternate screen. The parent can now inject bytes without canonical line
             * buffering hiding them from readAvailable().
             */
            const char ready = 'R';
            if (::write(ready_pipe_write, &ready, 1) != 1) {
                return 21;
            }

            std::string input;
            for (int attempt = 0; attempt < 100 && input.empty(); ++attempt) {
                input = session.pollInputBytes();
                if (input.empty()) {
                    std::this_thread::sleep_for(1ms);
                }
            }

            check(input == std::string{"\x1B[Aq"},
                  "native PTY input bytes must survive raw non-blocking transport");

            ScreenBuffer buffer{{2, 1}};
            buffer.set({0, 0}, Cell{U'O'});
            buffer.set({1, 0}, Cell{U'K'});

            session.present(buffer, Point{1, 0});
        }

        /*
         * TerminalSession has already restored termios and emitted alternate-screen/cursor teardown
         * before the child exits. The parent validates those effects through the PTY master/slave.
         */
        ::close(ready_pipe_write);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PTY child failure: " << error.what() << '\n';
        ::close(ready_pipe_write);
        return 22;
    } catch (...) {
        std::cerr << "PTY child failure: unknown exception\n";
        ::close(ready_pipe_write);
        return 23;
    }
}

bool waitReadable(int descriptor, int timeout_ms) {
    struct pollfd entry {};
    entry.fd = descriptor;
    entry.events = POLLIN | POLLHUP;

    int result = 0;
    do {
        result = ::poll(&entry, 1, timeout_ms);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        throw std::runtime_error(systemMessage("poll readiness pipe"));
    }

    return result > 0;
}

int waitForChild(pid_t child, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;

    while (std::chrono::steady_clock::now() < deadline) {
        const pid_t result = ::waitpid(child, &status, WNOHANG);
        if (result == child) {
            return status;
        }
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(systemMessage("waitpid"));
        }

        std::this_thread::sleep_for(1ms);
    }

    /*
     * Do not leave a wedged PTY child behind on a CI worker. SIGKILL is test-process cleanup only; it
     * is not part of toolkit terminal-session behavior.
     */
    (void)::kill(child, SIGKILL);

    pid_t result = 0;
    do {
        result = ::waitpid(child, &status, 0);
    } while (result < 0 && errno == EINTR);

    fail("native terminal child exceeded internal 2 second timeout");
}

} // namespace

int main() {
    try {
        /*
         * posix_openpt()/grantpt()/unlockpt() avoids depending on platform-specific openpty() linkage.
         * The test still exercises a genuine kernel pseudo-terminal, not a mock TerminalDevice.
         */
        const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (master < 0) {
            throw std::runtime_error(systemMessage("posix_openpt"));
        }

        if (::grantpt(master) != 0 || ::unlockpt(master) != 0) {
            const std::string message = systemMessage("grantpt/unlockpt");
            ::close(master);
            throw std::runtime_error(message);
        }

        const char* slave_name = ::ptsname(master);
        if (slave_name == nullptr) {
            const std::string message = systemMessage("ptsname");
            ::close(master);
            throw std::runtime_error(message);
        }

        const int slave = ::open(slave_name, O_RDWR | O_NOCTTY);
        if (slave < 0) {
            const std::string message = systemMessage("open PTY slave");
            ::close(master);
            throw std::runtime_error(message);
        }

        termios original{};
        if (::tcgetattr(slave, &original) != 0) {
            throw std::runtime_error(systemMessage("tcgetattr original PTY state"));
        }

        struct winsize dimensions {};
        dimensions.ws_col = 42;
        dimensions.ws_row = 11;
        if (::ioctl(slave, TIOCSWINSZ, &dimensions) != 0) {
            throw std::runtime_error(systemMessage("ioctl(TIOCSWINSZ)"));
        }

        int ready_pipe[2]{};
        if (::pipe(ready_pipe) != 0) {
            throw std::runtime_error(systemMessage("pipe"));
        }

        const pid_t child = ::fork();
        if (child < 0) {
            throw std::runtime_error(systemMessage("fork"));
        }

        if (child == 0) {
            ::close(master);
            ::close(ready_pipe[0]);
            const int code = childMain(slave, ready_pipe[1]);
            ::_exit(code);
        }

        ::close(ready_pipe[1]);

        check(waitReadable(ready_pipe[0], 2000),
              "child did not reach native terminal session within 2 seconds");

        char ready = 0;
        ssize_t ready_count = 0;
        do {
            ready_count = ::read(ready_pipe[0], &ready, 1);
        } while (ready_count < 0 && errno == EINTR);

        if (ready_count != 1 || ready != 'R') {
            const int status = waitForChild(child, 2000ms);
            if (WIFEXITED(status)) {
                throw std::runtime_error(
                    "native terminal child failed before readiness with exit code " +
                    std::to_string(WEXITSTATUS(status)));
            }
            fail("native terminal child failed before readiness");
        }

        writeAll(master, "\x1B[Aq");

        const int status = waitForChild(child, 2000ms);
        check(WIFEXITED(status), "native terminal child did not exit normally");
        check(WEXITSTATUS(status) == 0, "native terminal child reported failure");

        /*
         * Make master reads non-blocking only after the child is gone; all output is now queued and
         * drainMaster() cannot hang while looking for an EOF/EIO termination condition.
         */
        const int current_flags = ::fcntl(master, F_GETFL, 0);
        if (current_flags >= 0) {
            (void)::fcntl(master, F_SETFL, current_flags | O_NONBLOCK);
        }

        const std::string output = drainMaster(master);

        check(output.find("\x1B[?1049h") != std::string::npos,
              "native session must enter alternate screen");
        check(output.find("\x1B[?25l") != std::string::npos,
              "native session must hide cursor while active");
        check(output.find("OK") != std::string::npos,
              "TerminalSession::present must transport frame glyphs to real PTY");
        check(output.find("\x1B[1;2H\x1B[?25h") != std::string::npos,
              "presented caret must become ANSI cursor positioning/show sequence");
        check(output.find("\x1B[?25h\x1B[?1049l") != std::string::npos,
              "native session must restore cursor and leave alternate screen");

        termios restored{};
        if (::tcgetattr(slave, &restored) != 0) {
            throw std::runtime_error(systemMessage("tcgetattr restored PTY state"));
        }

        check(equivalentTerminalState(original, restored),
              "TerminalSession must restore original PTY termios state");

        ::close(ready_pipe[0]);
        ::close(slave);
        ::close(master);

        std::cout << "[PASS] native POSIX PTY terminal smoke test\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] native POSIX PTY terminal smoke test\n"
                  << "       " << error.what() << '\n';
        return 1;
    }
}

#else

int main() {
    return 0;
}

#endif
