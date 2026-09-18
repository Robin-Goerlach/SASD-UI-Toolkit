#pragma once

#include <sasd/ui/style.hpp>

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
 * Besides Unicode occupancy, a cell stores the already-resolved backend-neutral TextStyle that must
 * apply when its glyph is emitted. Keeping style in the off-screen model lets presentation tests stay
 * independent from ANSI serialization and gives a later diff encoder enough information to compare
 * both glyph and appearance.
 */
struct Cell {
    char32_t code_point{U' '};
    CellRole role{CellRole::normal};
    TextStyle style{};

    friend constexpr bool operator==(const Cell&, const Cell&) = default;
};

} // namespace sasd::ui::terminal
