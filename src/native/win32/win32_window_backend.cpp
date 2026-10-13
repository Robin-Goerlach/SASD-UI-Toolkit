#include <sasd/ui/native/win32_backend.hpp>

#include "win32_input_translation.hpp"
#include "win32_render_context.hpp"

#include <windows.h>

#include <sasd/ui/events/event_queue.hpp>
#include <sasd/ui/rendered/display_list_executor.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
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
        if (!(config.font_point_size > 0.0F) || !std::isfinite(config.font_point_size)) {
            throw std::invalid_argument{"Win32 font point size must be finite and positive"};
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
        case WM_PAINT:
            paintInvalidatedSurface(native_window);
            return 0;
        case WM_DPICHANGED:
            dpi = HIWORD(w_param) == 0 ? 96U : HIWORD(w_param);
            try {
                recreateFont();
            } catch (...) {
                // Window procedures cannot propagate exceptions across the Win32 ABI. Keep the
                // surface marked for replay; the next explicit presentation reports the failure.
                presentation_requested = true;
                return 0;
            }
            presentation_requested = true;
            return 0;
        case WM_CLOSE:
            // Application observes the request first; shutdown owns the only destruction path.
            if (const auto event = detail::translateWindowMessage(message, 0, 0)) {
                events.push(*event);
            }
            return 0;
        case WM_SIZE:
            if (const auto event = detail::translateWindowMessage(message, w_param, l_param)) {
                current_size = logicalSizeFromPhysical(std::get<ResizeEvent>(*event).size);
                std::get<ResizeEvent>(*event).size = current_size;
                presentation_requested = true;
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
                PointerEvent logical = *event;
                logical.position = logicalPointFromPhysical(logical.position);
                events.push(Event{logical});
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

    [[nodiscard]] Size logicalSizeFromPhysical(Size physical) const noexcept {
        return {static_cast<Coordinate>((static_cast<std::int64_t>(physical.width) * 96) / dpi),
                static_cast<Coordinate>((static_cast<std::int64_t>(physical.height) * 96) / dpi)};
    }

    [[nodiscard]] Point logicalPointFromPhysical(Point physical) const noexcept {
        return {static_cast<Coordinate>((static_cast<std::int64_t>(physical.x) * 96) / dpi),
                static_cast<Coordinate>((static_cast<std::int64_t>(physical.y) * 96) / dpi)};
    }

    void recreateFont() {
        if (font != nullptr) {
            DeleteObject(font);
            font = nullptr;
        }
        LOGFONTW description{};
        description.lfHeight = -MulDiv(static_cast<int>(std::lround(config.font_point_size)),
                                       static_cast<int>(dpi), 72);
        description.lfWeight = FW_NORMAL;
        description.lfCharSet = DEFAULT_CHARSET;
        wcscpy_s(description.lfFaceName, L"Segoe UI");
        font = CreateFontIndirectW(&description);
        if (font == nullptr) throw std::runtime_error{"CreateFontIndirectW failed"};
        ++revision;
    }

    void paintInvalidatedSurface(HWND native_window) noexcept {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(native_window, &paint);
        if (dc != nullptr && !last_frame.empty() && font != nullptr) {
            try {
                detail::Win32RenderContext context{dc, font, dpi};
                context.clear(config.background_color);
                active_context = &context;
                (void)::sasd::ui::rendered::DisplayListExecutor::execute(last_frame, *this);
                active_context = nullptr;
                presentation_requested = false;
            } catch (...) {
                active_context = nullptr;
                presentation_requested = true;
            }
        }
        EndPaint(native_window, &paint);
    }

    void requireRenderContext(const char* operation) const {
        requireInitialized(operation);
        if (active_context == nullptr) {
            throw std::logic_error{std::string{"Win32WindowBackend::"} + operation +
                                   " requires an active frame"};
        }
    }

    void initialize() {
        if (initialized()) throw std::logic_error{"Win32WindowBackend is already initialized"};
        owner_thread = std::this_thread::get_id();
        dpi = 96U;
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
        dpi = GetDpiForWindow(window);
        if (dpi == 0) dpi = 96U;
        recreateFont();
        current_size = logicalSizeFromPhysical(current_size);
        last_frame.clear();
        presentation_requested = true;
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
        active_context = nullptr;
        if (font != nullptr) {
            DeleteObject(font);
            font = nullptr;
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

    std::size_t presentFrame(const ::sasd::ui::rendered::DisplayList& list) {
        requireInitialized("presentFrame");
        HDC dc = GetDC(window);
        if (dc == nullptr) throw std::runtime_error{"GetDC failed"};
        try {
            detail::Win32RenderContext context{dc, font, dpi};
            context.clear(config.background_color);
            active_context = &context;
            const std::size_t executed = ::sasd::ui::rendered::DisplayListExecutor::execute(list, *this);
            active_context = nullptr;
            ReleaseDC(window, dc);
            last_frame = list;
            presentation_requested = false;
            return executed;
        } catch (...) {
            active_context = nullptr;
            ReleaseDC(window, dc);
            presentation_requested = true;
            throw;
        }
    }

    Win32WindowBackendConfig config;
    std::wstring title;
    HWND window{nullptr};
    Size current_size{};
    EventQueue events;
    std::thread::id owner_thread;
    bool destroying{false};
    bool mouse_tracking{false};
    UINT dpi{96};
    HFONT font{nullptr};
    ::sasd::ui::rendered::DisplayList last_frame;
    detail::Win32RenderContext* active_context{nullptr};
    bool presentation_requested{false};
    std::uint64_t revision{0};
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

bool Win32WindowBackend::presentationRequested() const noexcept {
    return impl_->presentation_requested;
}

std::size_t Win32WindowBackend::presentFrame(const ::sasd::ui::rendered::DisplayList& list) {
    return impl_->presentFrame(list);
}

void Win32WindowBackend::fillRect(const rendered::FillRectCommand& command) {
    impl_->requireRenderContext("fillRect");
    impl_->active_context->fillRect(command);
}

void Win32WindowBackend::strokeRect(const rendered::StrokeRectCommand& command) {
    impl_->requireRenderContext("strokeRect");
    impl_->active_context->strokeRect(command);
}

void Win32WindowBackend::drawText(const rendered::DrawTextCommand& command) {
    impl_->requireRenderContext("drawText");
    impl_->active_context->drawText(command);
}

Size Win32WindowBackend::measureText(std::string_view text) const {
    impl_->requireInitialized("measureText");
    HDC dc = GetDC(impl_->window);
    if (dc == nullptr) throw std::runtime_error{"GetDC failed"};
    detail::Win32RenderContext context{dc, impl_->font, impl_->dpi};
    const Size result = context.measureText(text);
    ReleaseDC(impl_->window, dc);
    return result;
}

Coordinate Win32WindowBackend::lineHeight() const noexcept {
    if (impl_->font == nullptr) return 0;
    HDC dc = GetDC(impl_->window);
    if (dc == nullptr) return 0;
    detail::Win32RenderContext context{dc, impl_->font, impl_->dpi};
    const Coordinate result = context.lineHeight();
    ReleaseDC(impl_->window, dc);
    return result;
}

std::optional<Coordinate> Win32WindowBackend::textAdvanceToScalar(
    std::string_view text, std::size_t scalar_index) const {
    impl_->requireInitialized("textAdvanceToScalar");
    HDC dc = GetDC(impl_->window);
    if (dc == nullptr) throw std::runtime_error{"GetDC failed"};
    detail::Win32RenderContext context{dc, impl_->font, impl_->dpi};
    const auto result = context.textAdvanceToScalar(text, scalar_index);
    ReleaseDC(impl_->window, dc);
    return result;
}

std::uint64_t Win32WindowBackend::revision() const noexcept { return impl_->revision; }

} // namespace sasd::ui::native::win32
