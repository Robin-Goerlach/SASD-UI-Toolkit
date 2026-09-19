#include <sasd/ui/terminal/native_terminal_device.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/terminal/terminal_session.hpp>

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;
using namespace sasd::ui;
using namespace sasd::ui::terminal;

constexpr std::string_view ready_marker{"__SASD_CONPTY_READY__"};
constexpr std::string_view restored_marker{"__SASD_CONPTY_RESTORED__"};

[[noreturn]] void fail(std::string_view message) {
    throw std::runtime_error(std::string{message});
}

void check(bool condition, std::string_view message) {
    if (!condition) {
        fail(message);
    }
}

[[nodiscard]] std::runtime_error win32Error(std::string_view operation) {
    std::string message{operation};
    message.append(" failed with Win32 error ");
    message += std::to_string(static_cast<unsigned long>(::GetLastError()));
    return std::runtime_error{message};
}

class UniqueHandle final {
public:
    UniqueHandle() noexcept = default;
    explicit UniqueHandle(HANDLE handle) noexcept : handle_{handle} {}

    ~UniqueHandle() {
        reset();
    }

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept : handle_{other.release()} {}

    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    [[nodiscard]] HANDLE get() const noexcept { return handle_; }

    [[nodiscard]] explicit operator bool() const noexcept {
        return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
    }

    [[nodiscard]] HANDLE release() noexcept {
        HANDLE result = handle_;
        handle_ = INVALID_HANDLE_VALUE;
        return result;
    }

    void reset(HANDLE replacement = INVALID_HANDLE_VALUE) noexcept {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            (void)::CloseHandle(handle_);
        }
        handle_ = replacement;
    }

private:
    HANDLE handle_{INVALID_HANDLE_VALUE};
};

class PseudoConsole final {
public:
    PseudoConsole() = default;

    ~PseudoConsole() {
        reset();
    }

    PseudoConsole(const PseudoConsole&) = delete;
    PseudoConsole& operator=(const PseudoConsole&) = delete;

    void create(COORD size, HANDLE input_read, HANDLE output_write) {
        if (console_ != nullptr) {
            throw std::logic_error("PseudoConsole already created");
        }

        const HRESULT result =
            ::CreatePseudoConsole(size, input_read, output_write, 0, &console_);
        if (FAILED(result)) {
            throw std::runtime_error(
                "CreatePseudoConsole failed with HRESULT " +
                std::to_string(static_cast<unsigned long>(result)));
        }
    }

    [[nodiscard]] HPCON get() const noexcept { return console_; }

    void reset() noexcept {
        if (console_ != nullptr) {
            ::ClosePseudoConsole(console_);
            console_ = nullptr;
        }
    }

private:
    HPCON console_{nullptr};
};

void writeAll(HANDLE handle, std::string_view bytes) {
    std::size_t offset = 0;

    while (offset < bytes.size()) {
        const std::size_t remaining = bytes.size() - offset;
        const DWORD chunk = static_cast<DWORD>(
            remaining > static_cast<std::size_t>(MAXDWORD)
                ? static_cast<std::size_t>(MAXDWORD)
                : remaining);

        DWORD written = 0;
        if (::WriteFile(handle, bytes.data() + offset, chunk, &written, nullptr) == 0) {
            throw win32Error("WriteFile");
        }
        if (written == 0) {
            fail("WriteFile made no progress");
        }

        offset += static_cast<std::size_t>(written);
    }
}

[[nodiscard]] std::string hexBytes(std::string_view bytes) {
    constexpr char digits[] = "0123456789ABCDEF";
    std::string result;

    for (const unsigned char byte : bytes) {
        if (!result.empty()) {
            result.push_back(' ');
        }
        result.push_back(digits[(byte >> 4U) & 0x0FU]);
        result.push_back(digits[byte & 0x0FU]);
    }

    return result;
}

void writeConsoleMarker(std::string_view marker) {
    const HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
    check(output != nullptr && output != INVALID_HANDLE_VALUE,
          "child stdout console handle is invalid");
    writeAll(output, marker);
}

[[nodiscard]] std::string drainPipe(HANDLE pipe) {
    std::string result;
    char buffer[1024];

    for (;;) {
        DWORD available = 0;
        if (::PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr) == 0) {
            const DWORD error = ::GetLastError();

            /*
             * Once the ConPTY output side closes, anonymous pipes report BROKEN_PIPE/NO_DATA. At that
             * point every byte produced by the child has already been consumed.
             */
            if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA) {
                break;
            }
            throw win32Error("PeekNamedPipe(ConPTY output)");
        }

        if (available == 0) {
            break;
        }

        const DWORD requested =
            available < static_cast<DWORD>(sizeof(buffer))
                ? available
                : static_cast<DWORD>(sizeof(buffer));

        DWORD count = 0;
        if (::ReadFile(pipe, buffer, requested, &count, nullptr) == 0) {
            const DWORD error = ::GetLastError();
            if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA) {
                break;
            }
            throw win32Error("ReadFile(ConPTY output)");
        }

        if (count == 0) {
            break;
        }

        result.append(buffer, static_cast<std::size_t>(count));
    }

    return result;
}

[[nodiscard]] std::wstring currentExecutablePath() {
    std::vector<wchar_t> buffer(512);

    for (;;) {
        const DWORD count =
            ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

        if (count == 0) {
            throw win32Error("GetModuleFileNameW");
        }

        if (count < buffer.size() - 1) {
            return std::wstring{buffer.data(), count};
        }

        buffer.resize(buffer.size() * 2);
    }
}

[[nodiscard]] std::wstring quoteCommandArgument(std::wstring_view value) {
    /*
     * The executable path is the only potentially quoted argument. The test binary path produced by
     * CMake does not contain embedded quotes, so normal Windows command-line quoting is sufficient.
     */
    std::wstring result;
    result.reserve(value.size() + 2);
    result.push_back(L'"');
    result.append(value);
    result.push_back(L'"');
    return result;
}

[[nodiscard]] UniqueHandle openConsoleHandle(const wchar_t* name) {
    HANDLE handle = ::CreateFileW(name,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  nullptr,
                                  OPEN_EXISTING,
                                  0,
                                  nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throw win32Error("CreateFileW(console handle)");
    }
    return UniqueHandle{handle};
}

int childMain() {
    try {
        /*
         * CTest itself redirects standard streams. A ConPTY-attached child can therefore enter with
         * inherited/redirected standard-handle table entries even though it owns a real console
         * session. Open CONIN$/CONOUT$ explicitly and publish them as this test child's standard
         * handles before constructing WindowsTerminalDevice. This models a normal interactive launch
         * while still letting the parent observe the ConPTY transport pipes.
         */
        UniqueHandle console_input = openConsoleHandle(L"CONIN$");
        UniqueHandle console_output = openConsoleHandle(L"CONOUT$");

        check(::SetStdHandle(STD_INPUT_HANDLE, console_input.get()) != 0,
              "SetStdHandle(STDIN -> CONIN$) failed");
        check(::SetStdHandle(STD_OUTPUT_HANDLE, console_output.get()) != 0,
              "SetStdHandle(STDOUT -> CONOUT$) failed");
        check(::SetStdHandle(STD_ERROR_HANDLE, console_output.get()) != 0,
              "SetStdHandle(STDERR -> CONOUT$) failed");

        const HANDLE input = ::GetStdHandle(STD_INPUT_HANDLE);
        const HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);

        DWORD original_input_mode = 0;
        DWORD original_output_mode = 0;
        check(::GetConsoleMode(input, &original_input_mode) != 0,
              "CONIN$ is not a usable console input handle");
        check(::GetConsoleMode(output, &original_output_mode) != 0,
              "CONOUT$ is not a usable console output handle");

        const UINT original_input_cp = ::GetConsoleCP();
        const UINT original_output_cp = ::GetConsoleOutputCP();

        auto device = createNativeTerminalDevice();
        check(device != nullptr, "native terminal factory returned null");
        check(device->isInteractive(), "ConPTY-backed stdin/stdout must be interactive");

        {
            TerminalSession session{*device};

            check(session.size() == Size{42, 11},
                  "native Windows terminal size must reflect ConPTY dimensions");

            DWORD active_input_mode = 0;
            DWORD active_output_mode = 0;
            check(::GetConsoleMode(input, &active_input_mode) != 0,
                  "GetConsoleMode(input) failed during session");
            check(::GetConsoleMode(output, &active_output_mode) != 0,
                  "GetConsoleMode(output) failed during session");

            check((active_input_mode & ENABLE_VIRTUAL_TERMINAL_INPUT) != 0,
                  "raw Windows session must enable Virtual Terminal input");
            check((active_input_mode & ENABLE_LINE_INPUT) == 0,
                  "raw Windows session must disable line input");
            check((active_input_mode & ENABLE_ECHO_INPUT) == 0,
                  "raw Windows session must disable echo input");
            check((active_output_mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0,
                  "Windows session must enable Virtual Terminal output");
            check(::GetConsoleCP() == CP_UTF8,
                  "raw Windows session must use UTF-8 input code page");
            check(::GetConsoleOutputCP() == CP_UTF8,
                  "Windows session must use UTF-8 output code page");

            /*
             * The parent waits for this marker before sending input so the bytes enter after raw/VT
             * mode is definitely active. The marker itself travels through the real ConPTY output
             * stream, just like application frames.
             */
            writeConsoleMarker(ready_marker);

            std::string input_bytes;
            const std::string expected_input{"\x1B[Aq"};

            /*
             * Console input is record-oriented internally. ConPTY may expose the arrow-key record and
             * the following printable key on separate ReadFile/poll cycles even though the host wrote
             * one contiguous VT byte sequence. Accumulate exactly as TerminalEventPump/AnsiInputDecoder
             * are designed to tolerate split native reads.
             */
            for (int attempt = 0;
                 attempt < 1000 && input_bytes.size() < expected_input.size();
                 ++attempt) {
                const std::string chunk = session.pollInputBytes();
                input_bytes.append(chunk);

                if (chunk.empty()) {
                    std::this_thread::sleep_for(1ms);
                }
            }

            if (input_bytes != expected_input) {
                throw std::runtime_error(
                    "native ConPTY input mismatch; expected [" +
                    hexBytes(expected_input) + "] received [" +
                    hexBytes(input_bytes) + "]");
            }

            ScreenBuffer buffer{{2, 1}};
            buffer.set({0, 0}, Cell{U'O'});
            buffer.set({1, 0}, Cell{U'K'});
            session.present(buffer, Point{1, 0});
        }

        DWORD restored_input_mode = 0;
        DWORD restored_output_mode = 0;
        check(::GetConsoleMode(input, &restored_input_mode) != 0,
              "GetConsoleMode(input) failed after restoration");
        check(::GetConsoleMode(output, &restored_output_mode) != 0,
              "GetConsoleMode(output) failed after restoration");

        check(restored_input_mode == original_input_mode,
              "TerminalSession must restore original Windows input mode");
        check(restored_output_mode == original_output_mode,
              "TerminalSession must restore original Windows output mode");
        check(::GetConsoleCP() == original_input_cp,
              "TerminalSession must restore original Windows input code page");
        check(::GetConsoleOutputCP() == original_output_cp,
              "TerminalSession must restore original Windows output code page");

        writeConsoleMarker(restored_marker);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ConPTY child failure: " << error.what() << '\n';
        return 21;
    } catch (...) {
        std::cerr << "ConPTY child failure: unknown exception\n";
        return 22;
    }
}

struct AttributeList {
    LPPROC_THREAD_ATTRIBUTE_LIST value{nullptr};
    std::vector<std::byte> storage;

    AttributeList() {
        SIZE_T bytes = 0;
        (void)::InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
        if (bytes == 0) {
            throw win32Error("InitializeProcThreadAttributeList(size)");
        }

        storage.resize(bytes);
        value = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());

        if (::InitializeProcThreadAttributeList(value, 1, 0, &bytes) == 0) {
            throw win32Error("InitializeProcThreadAttributeList");
        }
    }

    ~AttributeList() {
        if (value != nullptr) {
            ::DeleteProcThreadAttributeList(value);
        }
    }

    AttributeList(const AttributeList&) = delete;
    AttributeList& operator=(const AttributeList&) = delete;
};

int parentMain() {
    UniqueHandle input_read;
    UniqueHandle input_write;
    UniqueHandle output_read;
    UniqueHandle output_write;

    HANDLE raw_input_read = INVALID_HANDLE_VALUE;
    HANDLE raw_input_write = INVALID_HANDLE_VALUE;
    if (::CreatePipe(&raw_input_read, &raw_input_write, nullptr, 0) == 0) {
        throw win32Error("CreatePipe(ConPTY input)");
    }
    input_read.reset(raw_input_read);
    input_write.reset(raw_input_write);

    HANDLE raw_output_read = INVALID_HANDLE_VALUE;
    HANDLE raw_output_write = INVALID_HANDLE_VALUE;
    if (::CreatePipe(&raw_output_read, &raw_output_write, nullptr, 0) == 0) {
        throw win32Error("CreatePipe(ConPTY output)");
    }
    output_read.reset(raw_output_read);
    output_write.reset(raw_output_write);

    PseudoConsole pseudo_console;
    pseudo_console.create(COORD{42, 11}, input_read.get(), output_write.get());

    /*
     * CreatePseudoConsole owns duplicated references internally. Closing these parent-side ends now is
     * important: it lets EOF/BROKEN_PIPE later reflect the child/ConPTY lifetime instead of our own
     * accidental extra references.
     */
    input_read.reset();
    output_write.reset();

    AttributeList attributes;
    HPCON console_handle = pseudo_console.get();
    if (::UpdateProcThreadAttribute(attributes.value,
                                    0,
                                    PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                    console_handle,
                                    sizeof(console_handle),
                                    nullptr,
                                    nullptr) == 0) {
        throw win32Error("UpdateProcThreadAttribute(PSEUDOCONSOLE)");
    }

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = attributes.value;

    PROCESS_INFORMATION process_info{};

    const std::wstring executable = currentExecutablePath();
    std::wstring command_line =
        quoteCommandArgument(executable) + L" --sasd-conpty-child";

    if (::CreateProcessW(executable.c_str(),
                         command_line.data(),
                         nullptr,
                         nullptr,
                         FALSE,
                         EXTENDED_STARTUPINFO_PRESENT,
                         nullptr,
                         nullptr,
                         &startup.StartupInfo,
                         &process_info) == 0) {
        throw win32Error("CreateProcessW(ConPTY child)");
    }

    UniqueHandle process{process_info.hProcess};
    UniqueHandle thread{process_info.hThread};

    std::string output;
    bool input_sent = false;
    const auto deadline = std::chrono::steady_clock::now() + 5s;

    while (std::chrono::steady_clock::now() < deadline) {
        output += drainPipe(output_read.get());

        if (!input_sent && output.find(ready_marker) != std::string::npos) {
            writeAll(input_write.get(), "\x1B[Aq");
            input_sent = true;
        }

        const DWORD wait = ::WaitForSingleObject(process.get(), 1);
        if (wait == WAIT_OBJECT_0) {
            break;
        }
        if (wait != WAIT_TIMEOUT) {
            throw win32Error("WaitForSingleObject(ConPTY child)");
        }
    }

    DWORD exit_code = STILL_ACTIVE;
    if (::GetExitCodeProcess(process.get(), &exit_code) == 0) {
        throw win32Error("GetExitCodeProcess");
    }

    if (exit_code == STILL_ACTIVE) {
        (void)::TerminateProcess(process.get(), 99);
        (void)::WaitForSingleObject(process.get(), 2000);
        fail("native ConPTY child exceeded internal 5 second timeout");
    }

    /*
     * Child-process exit and ConPTY pipe delivery are not the same event. conhost may still have a
     * final small output batch queued after the client process has terminated. In particular, the
     * restoration marker is written only after TerminalSession has restored console state. Continue
     * draining for a bounded period instead of treating one temporarily-empty PeekNamedPipe result as
     * end-of-stream.
     *
     * This also keeps the pipe actively consumed before ClosePseudoConsole(), avoiding the documented
     * synchronous-pipe teardown deadlock class.
     */
    const auto drain_deadline = std::chrono::steady_clock::now() + 1000ms;
    while (std::chrono::steady_clock::now() < drain_deadline &&
           output.find(restored_marker) == std::string::npos) {
        const std::string chunk = drainPipe(output_read.get());
        output += chunk;

        if (chunk.empty()) {
            std::this_thread::sleep_for(1ms);
        }
    }

    check(input_sent, "ConPTY child never reached raw/VT readiness marker");
    check(exit_code == 0, "native ConPTY child reported failure");
    check(output.find(restored_marker) != std::string::npos,
          "ConPTY child did not confirm mode/code-page restoration");

    check(output.find("\x1B[?1049h") != std::string::npos,
          "native Windows session must enter alternate screen");
    check(output.find("\x1B[?25l") != std::string::npos,
          "native Windows session must hide cursor while active");
    check(output.find("OK") != std::string::npos,
          "TerminalSession::present must transport frame glyphs through ConPTY");
    check(output.find("\x1B[1;2H\x1B[?25h") != std::string::npos,
          "presented caret must become ANSI cursor positioning/show sequence");
    check(output.find("\x1B[?25h\x1B[?1049l") != std::string::npos,
          "native Windows session must restore cursor and leave alternate screen");

    /*
     * Stop accepting more input before closing the pseudo console. The Child has already exited and
     * the test has consumed all required output.
     */
    input_write.reset();
    pseudo_console.reset();
    output_read.reset();

    std::cout << "[PASS] native Windows ConPTY terminal smoke test\n";
    return 0;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    try {
        if (argc >= 2 && std::wstring_view{argv[1]} == L"--sasd-conpty-child") {
            return childMain();
        }

        return parentMain();
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] native Windows ConPTY terminal smoke test\n"
                  << "       " << error.what() << '\n';
        return 1;
    }
}

#else

int main() {
    return 0;
}

#endif
