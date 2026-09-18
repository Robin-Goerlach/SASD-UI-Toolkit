#pragma once

#include <sasd/ui/geometry.hpp>
#include <sasd/ui/terminal/cell.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace sasd::ui::terminal {

/**
 * Backend-internal-style off-screen model of a rectangular terminal cell surface.
 *
 * ScreenBuffer does not emit ANSI/VT sequences and does not read from an operating-system console.
 * It provides the deterministic cell storage that later TerminalPresentationSink and output/device
 * layers can use. Keeping this storage independent from terminal I/O makes clipping, layout and
 * widget rendering testable on every CI platform, including Windows and macOS.
 *
 * Coordinates use the same logical Point/Size/Rect types as the toolkit core. For this backend one
 * logical unit maps to one terminal cell.
 */
class ScreenBuffer final {
public:
    /** Creates an empty or sized buffer filled with blank cells. */
    explicit ScreenBuffer(Size size = {});

    [[nodiscard]] Size size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return cells_.empty(); }
    [[nodiscard]] std::size_t cellCount() const noexcept { return cells_.size(); }

    /** Returns true when point addresses an existing cell. */
    [[nodiscard]] bool contains(Point point) const noexcept;

    /**
     * Replaces the complete buffer with a new size and fill cell.
     *
     * Existing contents are intentionally not preserved. Terminal resize handling needs a freshly
     * defined frame, while preserving/reflowing semantic content belongs to Widget/layout logic.
     *
     * @throws std::invalid_argument for negative extents.
     * @throws std::length_error when the requested dimensions cannot be represented safely.
     * @throws std::bad_alloc when a valid but very large allocation cannot be satisfied.
     */
    void resize(Size size, Cell fill = {});

    /** Fills every existing cell without changing dimensions. */
    void clear(Cell fill = {}) noexcept;

    /**
     * Returns one mutable/const cell.
     *
     * @throws std::out_of_range when point is outside the buffer.
     */
    [[nodiscard]] Cell& at(Point point);
    [[nodiscard]] const Cell& at(Point point) const;

    /** Assigns one cell, with the same range checking as at(). */
    void set(Point point, Cell cell);

    /**
     * Returns a contiguous row view.
     *
     * Exposing a span lets future renderers write efficient horizontal runs without exposing the
     * owning std::vector or allowing callers to resize storage behind ScreenBuffer's invariants.
     *
     * @throws std::out_of_range when y is outside the buffer.
     */
    [[nodiscard]] std::span<Cell> row(Coordinate y);
    [[nodiscard]] std::span<const Cell> row(Coordinate y) const;

    /**
     * Fills the intersection of area and this buffer.
     *
     * Areas partially outside the screen are clipped rather than rejected. Empty/negative rectangles
     * have no effect. Edge arithmetic is widened before addition so rectangles near Coordinate limits
     * cannot overflow signed 32-bit arithmetic.
     */
    void fill(Rect area, Cell cell) noexcept;

private:
    [[nodiscard]] static std::size_t checkedCellCount(Size size);
    [[nodiscard]] std::size_t checkedIndex(Point point) const;
    [[nodiscard]] std::size_t widthAsSize() const noexcept;

    Size size_{};
    std::vector<Cell> cells_;
};

} // namespace sasd::ui::terminal
