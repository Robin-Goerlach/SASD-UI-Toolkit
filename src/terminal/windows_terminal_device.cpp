#include <sasd/ui/terminal/native_terminal_device.hpp>

#ifdef _WIN32

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sasd::ui::terminal {
namespace {

[[nodiscard]] std::runtime_error win32Error(std::string_view operation) {
    std::string message{operation};
    message.append(" failed with Win32 error ");
    message += std::to_string(static_cast<unsigned long>(::GetLastError()));
    return std::runtime_error{message};
}

class WindowsTerminalDevice final : public TerminalDevice {
public:
    WindowsTerminalDevice()
        : input_{::GetStdHandle(STD_INPUT_HANDLE)},
          output_{::GetStdHandle(STD_OUTPUT_HANDLE)} {}

    [[nodiscard]] std::string_view name() const noexcept override {
        return "windows-console-vt";
    }

    [[nodiscard]] bool isInteractive() const noexcept override {
        if (!validHandle(input_) || !validHandle(output_)) {
            return false;
        }

        DWORD input_mode = 0;
        DWORD output_mode = 0;
        return ::GetConsoleMode(input_, &input_mode) != 0 &&
               ::GetConsoleMode(output_, &output_mode) != 0;
    }

    [[nodiscard]] Size size() const override {
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (::GetConsoleScreenBufferInfo(output_, &info) == 0) {
            throw win32Error("GetConsoleScreenBufferInfo");
        }

        const LONG width =
            static_cast<LONG>(info.srWindow.Right) - static_cast<LONG>(info.srWindow.Left) + 1L;
        const LONG height =
            static_cast<LONG>(info.srWindow.Bottom) - static_cast<LONG>(info.srWindow.Top) + 1L;

        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Windows console reported invalid visible dimensions");
        }

        return {static_cast<Coordinate>(width), static_cast<Coordinate>(height)};
    }

    void beginSession(const TerminalSessionOptions& options) override {
        if (active_) {
            throw std::logic_error("Windows terminal session is already active");
        }
        if (!isInteractive()) {
            throw std::runtime_error("stdin/stdout are not interactive Windows console handles");
        }

        State pending;
        pending.options = options;

        if (::GetConsoleMode(input_, &pending.original_input_mode) == 0 ||
            ::GetConsoleMode(output_, &pending.original_output_mode) == 0) {
            throw win32Error("GetConsoleMode");
        }
        pending.modes_saved = true;

        pending.original_input_code_page = ::GetConsoleCP();
        pending.original_output_code_page = ::GetConsoleOutputCP();
        pending.code_pages_saved = true;

        DWORD output_mode =
            pending.original_output_mode |
            ENABLE_VIRTUAL_TERMINAL_PROCESSING |
            DISABLE_NEWLINE_AUTO_RETURN;

        if (::SetConsoleMode(output_, output_mode) == 0) {
            throw win32Error("SetConsoleMode(output VT)");
        }
        pending.output_mode_applied = true;

        if (::SetConsoleOutputCP(CP_UTF8) == 0) {
            rollback(pending);
            throw win32Error("SetConsoleOutputCP(CP_UTF8)");
        }
        pending.output_code_page_applied = true;

        if (options.raw_input) {
            DWORD input_mode = pending.original_input_mode;
            input_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
            /*
             * This byte-oriented adapter intentionally disables record-oriented mouse/window input.
             * Resize is queried through size(), and pointer support will get a dedicated semantic path
             * later. Keeping only byte-producing input avoids a signaled console handle with no bytes
             * for ReadFile().
             */
            input_mode &= ~(ENABLE_ECHO_INPUT |
                            ENABLE_LINE_INPUT |
                            ENABLE_MOUSE_INPUT |
                            ENABLE_WINDOW_INPUT);

            if (::SetConsoleMode(input_, input_mode) == 0) {
                rollback(pending);
                throw win32Error("SetConsoleMode(input VT)");
            }
            pending.input_mode_applied = true;

            if (::SetConsoleCP(CP_UTF8) == 0) {
                rollback(pending);
                throw win32Error("SetConsoleCP(CP_UTF8)");
            }
            pending.input_code_page_applied = true;
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

        State ending = state_;
        active_ = false;
        state_ = {};

        if (ending.alternate_screen_entered) {
            writeAllNoThrow("\x1B[?25h\x1B[?1049l");
        } else {
            writeAllNoThrow("\x1B[?25h");
        }

        restore(ending);
    }

    void write(std::string_view bytes) override {
        if (!active_) {
            throw std::logic_error("Windows terminal write requires an active session");
        }
        writeAll(bytes);
    }

    [[nodiscard]] std::string readAvailable() override {
        if (!active_) {
            throw std::logic_error("Windows terminal read requires an active session");
        }

        std::string result;
        char buffer[256];

        for (;;) {
            const DWORD wait_result = ::WaitForSingleObject(input_, 0);
            if (wait_result == WAIT_TIMEOUT) {
                break;
            }
            if (wait_result != WAIT_OBJECT_0) {
                throw win32Error("WaitForSingleObject(console input)");
            }

            DWORD read = 0;
            if (::ReadFile(input_,
                           buffer,
                           static_cast<DWORD>(sizeof(buffer)),
                           &read,
                           nullptr) == 0) {
                throw win32Error("ReadFile(console input)");
            }
            if (read == 0) {
                break;
            }

            result.append(buffer, static_cast<std::size_t>(read));
        }

        return result;
    }

private:
    struct State {
        TerminalSessionOptions options{};
        DWORD original_input_mode{0};
        DWORD original_output_mode{0};
        UINT original_input_code_page{0};
        UINT original_output_code_page{0};
        bool modes_saved{false};
        bool code_pages_saved{false};
        bool output_mode_applied{false};
        bool input_mode_applied{false};
        bool output_code_page_applied{false};
        bool input_code_page_applied{false};
        bool alternate_screen_entered{false};
    };

    [[nodiscard]] static bool validHandle(HANDLE handle) noexcept {
        return handle != nullptr && handle != INVALID_HANDLE_VALUE;
    }

    void writeAll(std::string_view bytes) {
        std::size_t offset = 0;

        while (offset < bytes.size()) {
            const std::size_t remaining = bytes.size() - offset;
            const DWORD chunk = static_cast<DWORD>(
                std::min<std::size_t>(remaining,
                                      static_cast<std::size_t>(
                                          std::numeric_limits<DWORD>::max())));

            DWORD written = 0;
            if (::WriteFile(output_, bytes.data() + offset, chunk, &written, nullptr) == 0) {
                throw win32Error("WriteFile(console)");
            }
            if (written == 0) {
                throw std::runtime_error("WriteFile(console) made no progress");
            }

            offset += static_cast<std::size_t>(written);
        }
    }

    void writeAllNoThrow(std::string_view bytes) noexcept {
        std::size_t offset = 0;

        while (offset < bytes.size()) {
            const std::size_t remaining = bytes.size() - offset;
            const DWORD chunk = static_cast<DWORD>(
                std::min<std::size_t>(remaining,
                                      static_cast<std::size_t>(
                                          std::numeric_limits<DWORD>::max())));

            DWORD written = 0;
            if (::WriteFile(output_, bytes.data() + offset, chunk, &written, nullptr) == 0 ||
                written == 0) {
                return;
            }

            offset += static_cast<std::size_t>(written);
        }
    }

    void restore(const State& state) noexcept {
        if (state.input_code_page_applied && state.code_pages_saved) {
            (void)::SetConsoleCP(state.original_input_code_page);
        }
        if (state.output_code_page_applied && state.code_pages_saved) {
            (void)::SetConsoleOutputCP(state.original_output_code_page);
        }
        if (state.input_mode_applied && state.modes_saved) {
            (void)::SetConsoleMode(input_, state.original_input_mode);
        }
        if (state.output_mode_applied && state.modes_saved) {
            (void)::SetConsoleMode(output_, state.original_output_mode);
        }
    }

    void rollback(State& pending) noexcept {
        if (pending.alternate_screen_entered) {
            writeAllNoThrow("\x1B[?25h\x1B[?1049l");
        }
        restore(pending);
    }

    HANDLE input_{INVALID_HANDLE_VALUE};
    HANDLE output_{INVALID_HANDLE_VALUE};
    bool active_{false};
    State state_{};
};

} // namespace

std::unique_ptr<TerminalDevice> createNativeTerminalDevice() {
    return std::make_unique<WindowsTerminalDevice>();
}

} // namespace sasd::ui::terminal

#endif
