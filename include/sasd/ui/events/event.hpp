#pragma once

#include <sasd/ui/geometry.hpp>

#include <cstdint>
#include <string>
#include <variant>

namespace sasd::ui {

struct QuitEvent {};

struct FocusEvent {
    bool gained{false};
};

enum class Key : std::uint16_t {
    unknown,
    enter,
    escape,
    tab,
    backspace,
    space,
    left,
    right,
    up,
    down,
    home,
    end,
    page_up,
    page_down,
};

enum class KeyModifier : std::uint8_t {
    none = 0,
    shift = 1U << 0U,
    control = 1U << 1U,
    alt = 1U << 2U,
    meta = 1U << 3U,
};

[[nodiscard]] constexpr KeyModifier operator|(KeyModifier left, KeyModifier right) noexcept {
    return static_cast<KeyModifier>(static_cast<std::uint8_t>(left) |
                                    static_cast<std::uint8_t>(right));
}

[[nodiscard]] constexpr bool hasModifier(KeyModifier value, KeyModifier modifier) noexcept {
    return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(modifier)) != 0U;
}

struct KeyEvent {
    Key key{Key::unknown};
    bool pressed{true};
    KeyModifier modifiers{KeyModifier::none};
};

/** Text input is separate from physical key input to support Unicode, IME and terminal backends. */
struct TextInputEvent {
    std::string text;
};

struct ResizeEvent {
    Size size{};
};

using Event = std::variant<QuitEvent, FocusEvent, KeyEvent, TextInputEvent, ResizeEvent>;

} // namespace sasd::ui
