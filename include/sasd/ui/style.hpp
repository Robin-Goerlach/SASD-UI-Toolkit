#pragma once

#include <cstdint>

namespace sasd::ui {

/**
 * Small backend-neutral named color palette for the first portable styling slice.
 *
 * These names intentionally match the widely portable 16-color terminal palette without making the
 * type terminal-specific. Desktop/rendered backends can map the same semantic colors to their own
 * theme/rendering primitives. Arbitrary RGB/alpha colors are deliberately deferred until a second
 * visible backend can validate how color capabilities and fallback should work.
 */
enum class Color : std::uint8_t {
    default_color,
    black,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white,
    bright_black,
    bright_red,
    bright_green,
    bright_yellow,
    bright_blue,
    bright_magenta,
    bright_cyan,
    bright_white,
};

/**
 * Minimal text-oriented appearance shared by semantic text controls.
 *
 * The style affects presentation only; it never changes intrinsic text metrics in the current M2
 * contract. Background colors, font families/sizes, padding, borders, cascading inheritance and
 * themes are intentionally not part of this first slice.
 */
struct TextStyle {
    Color foreground{Color::default_color};
    bool bold{false};
    bool dim{false};
    bool underline{false};
    bool inverse{false};

    friend constexpr bool operator==(const TextStyle&, const TextStyle&) = default;
};

} // namespace sasd::ui
