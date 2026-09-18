#pragma once

namespace sasd::ui::terminal {

/**
 * One logical terminal cell.
 *
 * M2 starts deliberately with the smallest useful cell state: one Unicode code point. Styling,
 * colors and grapheme/width metadata will be added only when the first visible widgets require them.
 *
 * A code point is not assumed to be the final display-width model. Combining marks and wide
 * graphemes may span or affect multiple terminal cells; keeping char32_t here merely avoids the much
 * worse architectural assumption that one UTF-8 byte equals one terminal cell.
 */
struct Cell {
    char32_t code_point{U' '};

    friend constexpr bool operator==(const Cell&, const Cell&) = default;
};

} // namespace sasd::ui::terminal
