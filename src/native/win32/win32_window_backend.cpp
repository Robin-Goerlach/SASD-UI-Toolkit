#include <sasd/ui/native/win32_backend.hpp>

#include "win32_input_translation.hpp"

#include <windows.h>

#include <sasd/ui/events/event_queue.hpp>

#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace sasd::ui::native::win32 {
namespace {

constexpr wchar_t window_class_name[] = L"SASD_UI_Toolkit_Native_Win32_Host";
unsigned class_users = 0;

[[nodiscard]] std::wstring utf8ToWide(const std::string& value) {
    if (value.empty()) return {};
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0);
    if (length <= 0) throw std::invalid_argument{"Win32 window title must be valid UTF-8"};
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                            static_cast<int>(value.size()), result.data(), length) != length) {
        throw std::invalid_argument{"Win32 window title must be valid UTF-8"};
    }
    return result;
}

[[nodiscard]] KeyModifier keyboardModifiers() noexcept {
    KeyModifier result = KeyModifier::none;
    if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) result = result | KeyModifier::shift;
    if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) result = result | KeyModifier::control;
    if ((GetKeyState(VK_MENU) & 0x8000) != 0) result = result | KeyModifier::alt;
    if ((GetKeyState(VK_LWIN) & 0x8000) != 0 || (GetKeyState(VK_RWIN) & 0x8000) != 0) {
        result = result | KeyModifier::meta;
    }
    return result;
}

} // namespace

struct Win32WindowBackend::Impl {
    explicit Impl(Win32WindowBackendConfig value) : config{std::move(value)} {
        if (config.initial_size.width <= 0 || config.initial_size.height <= 0) {
            throw std::invalid_argument{"Win32 window initial size must be positive"};
        }
        title = utf8ToWide(config.title);
    }
    ~Impl() { shutdown(); }

    [[nodiscard]] bool initialized() const noexcept { return window != nullptr; }
    void requireInitialized(const char* operation) const {
        if (!initialized()) {
            throw std::logic_error{std::string{"Win32WindowBackend::"} + operation +
                                   " requires initialize()"};
        }
        if (std::this_thread::get_id() != owner_thread) {
            throw std::runtime_error{"Win32 window backend must be used from its UI thread"};
        }
    }

    static LRESULT CALLBACK windowProc(HWND native_window, UINT message, WPARAM w_param,
                                       LPARAM l_param) noexcept {
        auto* self = reinterpret_cast<Impl*>(GetWindowLongPtrW(native_window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
            self = static_cast<Impl*>(create->lpCreateParams);
            SetWindowLongPtrW(native_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->window = native_window;
        }
        if (self == nullptr) return DefWindowProcW(native_window, message, w_param, l_param);
        return self->handleMessage(native_window, message, w_param, l_param);
    }

    LRESULT handleMessage(HWND native_window, UINT message, WPARAM w_param,
                          LPARAM l_param) noexcept {
        switch (message) {
        case WM_CLOSE:
            // Application observes the request first; shutdown owns the only destruction path.
            if (const auto event = detail::translateWindowMessage(message, 0, 0)) {
                events.push(*event);
            }
            return 0;
        case WM_SIZE:
            if (const auto event = detail::translateWindowMessage(message, w_param, l_param)) {
                current_size = std::get<ResizeEvent>(*event).size;
                events.push(*event);
            }
            return 0;
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            if (const auto event = detail::translateKeyMessage(message, w_param,
                                                                keyboardModifiers())) {
                events.push(Event{*event});
            }
            return 0;
        case WM_CHAR:
            {
                const wchar_t utf16_unit = static_cast<wchar_t>(w_param);
                if (const auto event = detail::translateTextInput(
                        std::wstring_view{&utf16_unit, 1})) {
                events.push(Event{*event});
                }
            }
            return 0;
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            if (message == WM_MOUSEMOVE && !mouse_tracking) {
                TRACKMOUSEEVENT tracking{};
                tracking.cbSize = sizeof(tracking);
                tracking.dwFlags = TME_LEAVE;
                tracking.hwndTrack = native_window;
                mouse_tracking = TrackMouseEvent(&tracking) != FALSE;
            }
            if (const auto event = detail::translatePointerMessage(
                    message, w_param, l_param, keyboardModifiers())) {
                events.push(Event{*event});
            }
            return message == WM_XBUTTONDOWN || message == WM_XBUTTONUP ? TRUE : 0;
        case WM_MOUSELEAVE:
            mouse_tracking = false;
            if (const auto event = detail::translatePointerSurfaceMessage(message)) {
                events.push(Event{*event});
            }
            return 0;
        case WM_NCDESTROY:
            if (!destroying) events.push(Event{QuitEvent{}});
            SetWindowLongPtrW(native_window, GWLP_USERDATA, 0);
            window = nullptr;
            return DefWindowProcW(native_window, message, w_param, l_param);
        default:
            return DefWindowProcW(native_window, message, w_param, l_param);
        }
    }

    void initialize() {
        if (initialized()) throw std::logic_error{"Win32WindowBackend is already initialized"};
        owner_thread = std::this_thread::get_id();
        const HINSTANCE instance = GetModuleHandleW(nullptr);
        if (class_users == 0) {
            WNDCLASSEXW klass{};
            klass.cbSize = sizeof(klass);
            klass.lpfnWndProc = &Impl::windowProc;
            klass.hInstance = instance;
            klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            klass.lpszClassName = window_class_name;
            if (RegisterClassExW(&klass) == 0) {
                owner_thread = {};
                throw std::runtime_error{"RegisterClassExW failed"};
            }
        }
        ++class_users;
        const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                            (config.resizable ? WS_THICKFRAME | WS_MAXIMIZEBOX : 0);
        window = CreateWindowExW(0, window_class_name, title.c_str(), style,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 config.initial_size.width, config.initial_size.height,
                                 nullptr, nullptr, instance, this);
        if (window == nullptr) {
            --class_users;
            if (class_users == 0) UnregisterClassW(window_class_name, instance);
            owner_thread = {};
            throw std::runtime_error{"CreateWindowExW failed"};
        }
        current_size = config.initial_size;
        events.clear();
        if (!config.hidden) ShowWindow(window, SW_SHOW);
    }

    void shutdown() noexcept {
        if (owner_thread != std::thread::id{} && std::this_thread::get_id() != owner_thread) return;
        destroying = true;
        events.clear();
        if (window != nullptr) {
            DestroyWindow(window);
            window = nullptr;
        }
        const HINSTANCE instance = GetModuleHandleW(nullptr);
        if (class_users > 0 && --class_users == 0) UnregisterClassW(window_class_name, instance);
        current_size = {};
        owner_thread = {};
        destroying = false;
    }

    [[nodiscard]] std::optional<Event> pollEvent() {
        requireInitialized("pollEvent");
        if (auto event = events.tryPop()) return event;
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) return Event{QuitEvent{}};
            TranslateMessage(&message);
            DispatchMessageW(&message);
            if (auto event = events.tryPop()) return event;
        }
        return std::nullopt;
    }

    Win32WindowBackendConfig config;
    std::wstring title;
    HWND window{nullptr};
    Size current_size{};
    EventQueue events;
    std::thread::id owner_thread;
    bool destroying{false};
    bool mouse_tracking{false};
};

Win32WindowBackend::Win32WindowBackend(Win32WindowBackendConfig config)
    : impl_{std::make_unique<Impl>(std::move(config))} {}
Win32WindowBackend::~Win32WindowBackend() { shutdown(); }

BackendCapabilities Win32WindowBackend::capabilities() const noexcept {
    BackendCapabilities result;
    result.pointer_input = true;
    return result;
}
void Win32WindowBackend::initialize() { impl_->initialize(); }
void Win32WindowBackend::shutdown() noexcept { impl_->shutdown(); }
std::optional<Event> Win32WindowBackend::pollEvent() { return impl_->pollEvent(); }
bool Win32WindowBackend::isInitialized() const noexcept { return impl_->initialized(); }
Size Win32WindowBackend::windowSize() const {
    impl_->requireInitialized("windowSize");
    return impl_->current_size;
}

} // namespace sasd::ui::native::win32
