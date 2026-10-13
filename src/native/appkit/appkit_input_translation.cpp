#include "appkit_input_translation.hpp"

#include <cmath>
#include <limits>

namespace sasd::ui::native::appkit::detail {
namespace {
constexpr std::uint64_t shift = 1ULL << 17U;
constexpr std::uint64_t control = 1ULL << 18U;
constexpr std::uint64_t alt = 1ULL << 19U;
constexpr std::uint64_t meta = 1ULL << 20U;
}

std::optional<Key> translateKey(std::uint16_t key_code) noexcept {
    // ANSI keyboard key codes, as documented by NSEvent's keyCode property.
    constexpr std::uint16_t letters[] = {
        0, 11, 8, 2, 14, 3, 5, 4, 34, 38, 40, 37, 46,
        45, 31, 35, 12, 15, 1, 17, 32, 9, 13, 7, 16, 6};
    for (std::uint16_t index = 0; index != 26; ++index) {
        if (key_code == letters[index]) {
            return static_cast<Key>(static_cast<unsigned>(Key::a) + index);
        }
    }
    switch (key_code) {
    case 36: return Key::enter;
    case 53: return Key::escape;
    case 48: return Key::tab;
    case 51: return Key::backspace;
    case 117: return Key::delete_forward;
    case 49: return Key::space;
    case 123: return Key::left;
    case 124: return Key::right;
    case 126: return Key::up;
    case 125: return Key::down;
    case 115: return Key::home;
    case 119: return Key::end;
    case 116: return Key::page_up;
    case 121: return Key::page_down;
    case 122: return Key::f1;
    case 120: return Key::f2;
    case 99: return Key::f3;
    case 118: return Key::f4;
    case 96: return Key::f5;
    case 97: return Key::f6;
    case 98: return Key::f7;
    case 100: return Key::f8;
    case 101: return Key::f9;
    case 109: return Key::f10;
    case 103: return Key::f11;
    case 111: return Key::f12;
    default: return std::nullopt;
    }
}

std::optional<Key> translateLogicalLetter(std::uint32_t scalar) noexcept {
    if (scalar >= 'A' && scalar <= 'Z') scalar += 'a' - 'A';
    if (scalar < 'a' || scalar > 'z') return std::nullopt;
    return static_cast<Key>(static_cast<unsigned>(Key::a) + (scalar - 'a'));
}

KeyModifier translateModifiers(std::uint64_t flags) noexcept {
    KeyModifier result = KeyModifier::none;
    if ((flags & shift) != 0) result = result | KeyModifier::shift;
    if ((flags & control) != 0) result = result | KeyModifier::control;
    if ((flags & alt) != 0) result = result | KeyModifier::alt;
    if ((flags & meta) != 0) result = result | KeyModifier::meta;
    return result;
}

std::optional<Point> translatePoint(double x, double y) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || x < 0.0 || y < 0.0 ||
        x > static_cast<double>(std::numeric_limits<Coordinate>::max()) ||
        y > static_cast<double>(std::numeric_limits<Coordinate>::max())) return std::nullopt;
    return Point{static_cast<Coordinate>(x), static_cast<Coordinate>(y)};
}

std::optional<PointerEvent> translatePointerButton(std::uint16_t button, bool pressed,
                                                    double x, double y,
                                                    std::uint64_t flags) noexcept {
    const auto point = translatePoint(x, y);
    if (!point) return std::nullopt;
    PointerButton semantic = PointerButton::other;
    if (button == 0) semantic = PointerButton::primary;
    else if (button == 1) semantic = PointerButton::middle;
    else if (button == 2) semantic = PointerButton::secondary;
    return PointerEvent{*point, pressed ? PointerAction::press : PointerAction::release,
                        semantic, 1, translateModifiers(flags)};
}

} // namespace sasd::ui::native::appkit::detail
