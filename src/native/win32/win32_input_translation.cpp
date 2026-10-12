#include "win32_input_translation.hpp"

#include <limits>
#include <string>
#include <utility>

namespace sasd::ui::native::win32::detail {
namespace {

[[nodiscard]] std::optional<Key> keyForVirtualKey(WPARAM key) noexcept {
    if (key >= 'A' && key <= 'Z') {
        return static_cast<Key>(static_cast<unsigned>(Key::a) +
                                static_cast<unsigned>(key - 'A'));
    }

    switch (key) {
    case VK_RETURN: return Key::enter;
    case VK_ESCAPE: return Key::escape;
    case VK_TAB: return Key::tab;
    case VK_BACK: return Key::backspace;
    case VK_DELETE: return Key::delete_forward;
    case VK_SPACE: return Key::space;
    case VK_LEFT: return Key::left;
    case VK_RIGHT: return Key::right;
    case VK_UP: return Key::up;
    case VK_DOWN: return Key::down;
    case VK_HOME: return Key::home;
    case VK_END: return Key::end;
    case VK_PRIOR: return Key::page_up;
    case VK_NEXT: return Key::page_down;
    case VK_F1: return Key::f1;
    case VK_F2: return Key::f2;
    case VK_F3: return Key::f3;
    case VK_F4: return Key::f4;
    case VK_F5: return Key::f5;
    case VK_F6: return Key::f6;
    case VK_F7: return Key::f7;
    case VK_F8: return Key::f8;
    case VK_F9: return Key::f9;
    case VK_F10: return Key::f10;
    case VK_F11: return Key::f11;
    case VK_F12: return Key::f12;
    default: return std::nullopt;
    }
}

[[nodiscard]] std::optional<PointerButton> pointerButton(UINT message, WPARAM state) noexcept {
    switch (message) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP: return PointerButton::primary;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP: return PointerButton::middle;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP: return PointerButton::secondary;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
        return HIWORD(state) == XBUTTON1 ? PointerButton::auxiliary1
                                         : PointerButton::auxiliary2;
    default: return std::nullopt;
    }
}

} // namespace

std::optional<Event> translateWindowMessage(UINT message,
                                            WPARAM native_width,
                                            LPARAM native_size) noexcept {
    if (message == WM_CLOSE) return Event{QuitEvent{}};
    if (message != WM_SIZE || native_width == SIZE_MINIMIZED) return std::nullopt;
    return Event{ResizeEvent{{static_cast<Coordinate>(LOWORD(native_size)),
                              static_cast<Coordinate>(HIWORD(native_size))}}};
}

KeyModifier translateModifiers(WPARAM native_modifiers, KeyModifier additional) noexcept {
    KeyModifier result = additional;
    if ((native_modifiers & MK_SHIFT) != 0U) result = result | KeyModifier::shift;
    if ((native_modifiers & MK_CONTROL) != 0U) result = result | KeyModifier::control;
    return result;
}

std::optional<KeyEvent> translateKeyMessage(UINT message,
                                            WPARAM virtual_key,
                                            KeyModifier modifiers) noexcept {
    if (message != WM_KEYDOWN && message != WM_KEYUP &&
        message != WM_SYSKEYDOWN && message != WM_SYSKEYUP) {
        return std::nullopt;
    }
    const auto key = keyForVirtualKey(virtual_key);
    if (!key.has_value()) return std::nullopt;
    return KeyEvent{*key, message == WM_KEYDOWN || message == WM_SYSKEYDOWN, modifiers};
}

std::optional<TextInputEvent> translateTextInput(std::wstring_view utf16) noexcept {
    if (utf16.empty() || utf16.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    const int length = static_cast<int>(utf16.size());
    const int required = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data(), length, nullptr, 0, nullptr, nullptr);
    if (required <= 0) return std::nullopt;

    std::string utf8(static_cast<std::size_t>(required), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data(), length, utf8.data(),
                            required, nullptr, nullptr) != required) {
        return std::nullopt;
    }
    return TextInputEvent{std::move(utf8)};
}

std::optional<Point> translateClientPoint(LPARAM native_point) noexcept {
    const auto x = static_cast<short>(LOWORD(native_point));
    const auto y = static_cast<short>(HIWORD(native_point));
    return Point{static_cast<Coordinate>(x), static_cast<Coordinate>(y)};
}

std::optional<PointerEvent> translatePointerMessage(UINT message,
                                                     WPARAM native_state,
                                                     LPARAM native_point,
                                                     KeyModifier additional) noexcept {
    const auto point = translateClientPoint(native_point);
    if (!point.has_value()) return std::nullopt;
    const KeyModifier modifiers = translateModifiers(native_state, additional);
    if (message == WM_MOUSEMOVE) {
        return PointerEvent{*point, PointerAction::move, PointerButton::none, 0, modifiers};
    }
    const auto button = pointerButton(message, native_state);
    if (!button.has_value()) return std::nullopt;
    const bool pressed = message == WM_LBUTTONDOWN || message == WM_MBUTTONDOWN ||
                         message == WM_RBUTTONDOWN || message == WM_XBUTTONDOWN;
    return PointerEvent{*point, pressed ? PointerAction::press : PointerAction::release,
                        *button, 1, modifiers};
}

std::optional<PointerSurfaceEvent> translatePointerSurfaceMessage(UINT message) noexcept {
    if (message == WM_MOUSELEAVE) {
        return PointerSurfaceEvent{PointerSurfaceAction::left};
    }
    return std::nullopt;
}

} // namespace sasd::ui::native::win32::detail
