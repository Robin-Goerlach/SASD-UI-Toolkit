#include "gtk_input_translation.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <cmath>
#include <limits>

namespace sasd::ui::native::gtk::detail {

std::optional<Key> translateKey(guint keyval) noexcept {
    if (keyval >= GDK_KEY_a && keyval <= GDK_KEY_z) {
        return static_cast<Key>(static_cast<unsigned>(Key::a) + (keyval - GDK_KEY_a));
    }
    if (keyval >= GDK_KEY_A && keyval <= GDK_KEY_Z) {
        return static_cast<Key>(static_cast<unsigned>(Key::a) + (keyval - GDK_KEY_A));
    }
    switch (keyval) {
    case GDK_KEY_Return: return Key::enter;
    case GDK_KEY_KP_Enter: return Key::enter;
    case GDK_KEY_Escape: return Key::escape;
    case GDK_KEY_Tab: return Key::tab;
    case GDK_KEY_BackSpace: return Key::backspace;
    case GDK_KEY_Delete: return Key::delete_forward;
    case GDK_KEY_space: return Key::space;
    case GDK_KEY_Left: return Key::left;
    case GDK_KEY_Right: return Key::right;
    case GDK_KEY_Up: return Key::up;
    case GDK_KEY_Down: return Key::down;
    case GDK_KEY_Home: return Key::home;
    case GDK_KEY_End: return Key::end;
    case GDK_KEY_Page_Up: return Key::page_up;
    case GDK_KEY_Page_Down: return Key::page_down;
    case GDK_KEY_F1: return Key::f1;
    case GDK_KEY_F2: return Key::f2;
    case GDK_KEY_F3: return Key::f3;
    case GDK_KEY_F4: return Key::f4;
    case GDK_KEY_F5: return Key::f5;
    case GDK_KEY_F6: return Key::f6;
    case GDK_KEY_F7: return Key::f7;
    case GDK_KEY_F8: return Key::f8;
    case GDK_KEY_F9: return Key::f9;
    case GDK_KEY_F10: return Key::f10;
    case GDK_KEY_F11: return Key::f11;
    case GDK_KEY_F12: return Key::f12;
    default: return std::nullopt;
    }
}

KeyModifier translateModifiers(GdkModifierType state) noexcept {
    KeyModifier result = KeyModifier::none;
    if ((state & GDK_SHIFT_MASK) != 0) result = result | KeyModifier::shift;
    if ((state & GDK_CONTROL_MASK) != 0) result = result | KeyModifier::control;
    if ((state & GDK_ALT_MASK) != 0) result = result | KeyModifier::alt;
    if ((state & GDK_META_MASK) != 0) result = result | KeyModifier::meta;
    return result;
}

std::optional<Point> translatePoint(double x, double y) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || x < 0.0 || y < 0.0 ||
        x > static_cast<double>(std::numeric_limits<Coordinate>::max()) ||
        y > static_cast<double>(std::numeric_limits<Coordinate>::max())) {
        return std::nullopt;
    }
    return Point{static_cast<Coordinate>(x), static_cast<Coordinate>(y)};
}

std::optional<PointerEvent> translatePointerButton(guint button, bool pressed, double x, double y,
                                                    GdkModifierType state) noexcept {
    const auto point = translatePoint(x, y);
    if (!point.has_value()) return std::nullopt;
    PointerButton semantic = PointerButton::other;
    if (button == 1) semantic = PointerButton::primary;
    else if (button == 2) semantic = PointerButton::middle;
    else if (button == 3) semantic = PointerButton::secondary;
    return PointerEvent{*point, pressed ? PointerAction::press : PointerAction::release, semantic,
                        1, translateModifiers(state)};
}

std::optional<std::string> committedText(guint keyval) noexcept {
    const gunichar scalar = gdk_keyval_to_unicode(keyval);
    if (scalar == 0 || scalar == '\n' || scalar == '\r' || scalar == '\t' ||
        !g_unichar_isprint(scalar)) {
        return std::nullopt;
    }
    std::string result;
    utf8::appendScalar(result, static_cast<char32_t>(scalar));
    return result;
}

} // namespace sasd::ui::native::gtk::detail
