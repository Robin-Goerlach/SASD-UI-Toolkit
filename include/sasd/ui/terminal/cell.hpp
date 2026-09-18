#pragma once

#include <cstdint>

namespace sasd::ui::terminal {

/**
 * Structural role of one terminal cell in a fixed-cell presentation buffer.
 */
enum class CellRole : std::uint8_t {
    /** Ordinary one-cell glyph/background cell. */
    normal,

    /** First cell occupied by a glyph whose terminal width is two columns. */
    wide_lead,

    /**
     * Second cell reserved by a preceding wide glyph.
     *
     * A future terminal diff/output writer must not emit this cell as an independent blank because
     * the lead glyph already advances the real terminal cursor across both columns.
     */
    wide_continuation,
};

/**
 * One logical terminal cell.
 *
 * The cell stores a Unicode scalar for ordinary/wide-lead content plus an occupancy role. This is
 * sufficient for narrow and two-column glyphs. It intentionally does not pretend to solve grapheme
 * clusters: combining/ZWJ sequences are currently deferred before they reach ScreenBuffer.
 */
struct Cell {
    char32_t code_point{U' '};
    CellRole role{CellRole::normal};

    friend constexpr bool operator==(const Cell&, const Cell&) = default;
};

} // namespace sasd::ui::terminal
