#pragma once

#include <sasd/ui/shortcut.hpp>

#include <string>
#include <string_view>

namespace sasd::ui {

/**
 * Returns the deterministic, backend-neutral display token for one semantic Key.
 *
 * The initial shortcut model intentionally contains only navigation/control/function keys, so this
 * formatter uses compact ASCII labels that are safe in both terminal and rendered presentations.
 * Returning an empty view for Key::unknown is deliberate: unknown represents failed/unsupported input
 * normalization and must not be presented as if it were a usable shortcut gesture.
 *
 * This function is presentation metadata only. It does not participate in event matching or command
 * routing, and changing a label in a future localization/theme layer must not change Shortcut identity.
 */
[[nodiscard]] constexpr std::string_view shortcutKeyDisplayText(Key key) noexcept {
    switch (key) {
    case Key::unknown:
        return {};
    case Key::enter:
        return "Enter";
    case Key::escape:
        return "Esc";
    case Key::tab:
        return "Tab";
    case Key::backspace:
        return "Backspace";
    case Key::delete_forward:
        return "Delete";
    case Key::space:
        return "Space";
    case Key::left:
        return "Left";
    case Key::right:
        return "Right";
    case Key::up:
        return "Up";
    case Key::down:
        return "Down";
    case Key::home:
        return "Home";
    case Key::end:
        return "End";
    case Key::page_up:
        return "PageUp";
    case Key::page_down:
        return "PageDown";
    case Key::f1:
        return "F1";
    case Key::f2:
        return "F2";
    case Key::f3:
        return "F3";
    case Key::f4:
        return "F4";
    case Key::f5:
        return "F5";
    case Key::f6:
        return "F6";
    case Key::f7:
        return "F7";
    case Key::f8:
        return "F8";
    case Key::f9:
        return "F9";
    case Key::f10:
        return "F10";
    case Key::f11:
        return "F11";
    case Key::f12:
        return "F12";
    }

    return {};
}

/**
 * Formats one Shortcut for presentation, for example "Ctrl+Alt+F5".
 *
 * Modifier order is a toolkit presentation convention, not part of Shortcut equality. The order is
 * fixed as Ctrl, Alt, Shift, Meta so Terminal and Rendered menu presenters cannot drift into subtly
 * different labels for the same semantic gesture. Only the four currently defined modifier bits are
 * emitted; any future modifier additions must be handled explicitly rather than accidentally appearing
 * through integer formatting.
 *
 * Key::unknown produces an empty string even when modifier bits are present. A modifier-only string
 * would imply an invokable gesture that Shortcut::matches() can never accept.
 */
[[nodiscard]] inline std::string shortcutDisplayText(const Shortcut& shortcut) {
    const std::string_view key_text = shortcutKeyDisplayText(shortcut.key);
    if (key_text.empty()) {
        return {};
    }

    std::string result;

    const auto append_modifier = [&result](std::string_view text) {
        if (!result.empty()) {
            result.push_back('+');
        }
        result.append(text);
    };

    if (hasModifier(shortcut.modifiers, KeyModifier::control)) {
        append_modifier("Ctrl");
    }
    if (hasModifier(shortcut.modifiers, KeyModifier::alt)) {
        append_modifier("Alt");
    }
    if (hasModifier(shortcut.modifiers, KeyModifier::shift)) {
        append_modifier("Shift");
    }
    if (hasModifier(shortcut.modifiers, KeyModifier::meta)) {
        append_modifier("Meta");
    }

    if (!result.empty()) {
        result.push_back('+');
    }
    result.append(key_text);
    return result;
}

} // namespace sasd::ui
