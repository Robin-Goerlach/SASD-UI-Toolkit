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

        /*
         * tcsetattr() is a job-control operation on a controlling terminal. Make this new session's
         * process group foreground explicitly so macOS cannot stop the child with SIGTTOU while
         * TerminalSession applies raw mode.
         */
        if (::tcsetpgrp(slave, ::getpgrp()) != 0) {
            return 24;
        }

        const char attached = '1';
        if (::write(ready_pipe_write, &attached, 1) != 1) {
            return 25;
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

        const char interactive = '2';
        if (::write(ready_pipe_write, &interactive, 1) != 1) {
            return 26;
        }

        {
            TerminalSession session{*device};

            const char session_started = '3';
            if (::write(ready_pipe_write, &session_started, 1) != 1) {
                return 27;
            }

            check(session.size() == Size{42, 11},
                  "native terminal size must come from PTY window dimensions");

            /*
             * Signal readiness only after TerminalSession has switched the slave into raw mode,
             * entered the alternate screen and proved size discovery works. The parent can now inject
             * bytes without canonical line buffering hiding them from readAvailable().
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

            const char input_read = '4';
            if (::write(ready_pipe_write, &input_read, 1) != 1) {
                return 28;
            }

            ScreenBuffer buffer{{2, 1}};
            buffer.set({0, 0}, Cell{U'O'});
            buffer.set({1, 0}, Cell{U'K'});

            session.present(buffer, Point{1, 0});

            const char frame_presented = '5';
            if (::write(ready_pipe_write, &frame_presented, 1) != 1) {
                return 29;
            }
        }

        const char session_restored = '6';
        if (::write(ready_pipe_write, &session_restored, 1) != 1) {
            return 30;
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

int waitForChild(pid_t child,
                 int stage_descriptor,
                 int master,
                 std::string& master_output,
                 std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    char last_stage = 'R';

    const int flags = ::fcntl(stage_descriptor, F_GETFL, 0);
    if (flags >= 0) {
        (void)::fcntl(stage_descriptor, F_SETFL, flags | O_NONBLOCK);
    }

    while (std::chrono::steady_clock::now() < deadline) {
        /*
         * A real terminal emulator consumes output continuously. Do the same while waiting for the
         * child so macOS tcsetattr(TCSAFLUSH) cannot deadlock behind pending PTY output during
         * TerminalSession restoration.
         */
        if (master >= 0) {
            master_output += drainMaster(master);
        }

        for (;;) {
            char stage = 0;
            const ssize_t count = ::read(stage_descriptor, &stage, 1);
            if (count == 1) {
                last_stage = stage;
                continue;
            }
            if (count < 0 && errno == EINTR) {
                continue;
            }
            break;
        }

        const pid_t result = ::waitpid(child, &status, WNOHANG | WUNTRACED);
        if (result == child) {
            if (WIFSTOPPED(status)) {
                throw std::runtime_error(
                    "native terminal child stopped by signal " +
                    std::to_string(WSTOPSIG(status)) +
                    "; last stage=" + std::string{last_stage});
            }
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
    if (master >= 0) {
        master_output += drainMaster(master);
    }

    pid_t result = 0;
    do {
        result = ::waitpid(child, &status, 0);
    } while (result < 0 && errno == EINTR);

    throw std::runtime_error(
        "native terminal child exceeded internal 2 second timeout; last stage=" +
        std::string{last_stage});
}

char waitForReady(pid_t child, int descriptor, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    char last_stage = '0';

    while (std::chrono::steady_clock::now() < deadline) {
        struct pollfd entry {};
        entry.fd = descriptor;
        entry.events = POLLIN | POLLHUP;

        int result = 0;
        do {
            result = ::poll(&entry, 1, 50);
        } while (result < 0 && errno == EINTR);

        if (result < 0) {
            throw std::runtime_error(systemMessage("poll readiness pipe"));
        }
        if (result == 0) {
            continue;
        }

        char stage = 0;
        const ssize_t count = ::read(descriptor, &stage, 1);

        if (count == 1) {
            last_stage = stage;
            if (stage == 'R') {
                return stage;
            }
            continue;
        }

        if (count == 0) {
            std::string discarded_output;
            const int status =
                waitForChild(child, descriptor, -1, discarded_output, 500ms);
            if (WIFEXITED(status)) {
                throw std::runtime_error(
                    "native terminal child exited before readiness; last stage=" +
                    std::string{last_stage} +
                    ", exit code=" + std::to_string(WEXITSTATUS(status)));
            }
            fail("native terminal child ended before readiness");
        }

        if (errno != EINTR) {
            throw std::runtime_error(systemMessage("read readiness pipe"));
        }
    }

    (void)::kill(child, SIGKILL);
    int status = 0;
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }

    throw std::runtime_error(
        "native terminal child timed out before readiness; last stage=" +
        std::string{last_stage});
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
        const std::string slave_path{slave_name};

        const int slave = ::open(slave_path.c_str(), O_RDWR | O_NOCTTY);
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

        /*
         * The parent must not keep the PTY slave open while testing child lifetime. This more closely
         * matches a real terminal session and lets master EOF/EIO reflect the child closing its tty.
         * Re-open the slave later only to verify that termios restoration persisted.
         */
        ::close(slave);

        (void)waitForReady(child, ready_pipe[0], 2000ms);

        writeAll(master, "\x1B[Aq");

        /*
         * From this point on the parent acts like a terminal emulator: keep the master non-blocking
         * and continuously consume child output while waiting. This matters on macOS because restoring
         * termios with TCSAFLUSH may wait for already-written terminal output to drain.
         */
        const int current_flags = ::fcntl(master, F_GETFL, 0);
        if (current_flags >= 0) {
            (void)::fcntl(master, F_SETFL, current_flags | O_NONBLOCK);
        }

        std::string output;
        const int status =
            waitForChild(child, ready_pipe[0], master, output, 2000ms);
        check(WIFEXITED(status), "native terminal child did not exit normally");
        check(WEXITSTATUS(status) == 0, "native terminal child reported failure");

        output += drainMaster(master);

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

        const int restored_slave = ::open(slave_path.c_str(), O_RDWR | O_NOCTTY);
        if (restored_slave < 0) {
            throw std::runtime_error(systemMessage("re-open PTY slave for restoration check"));
        }

        termios restored{};
        if (::tcgetattr(restored_slave, &restored) != 0) {
            ::close(restored_slave);
            throw std::runtime_error(systemMessage("tcgetattr restored PTY state"));
        }

        check(equivalentTerminalState(original, restored),
              "TerminalSession must restore original PTY termios state");

        ::close(restored_slave);
        ::close(ready_pipe[0]);
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
