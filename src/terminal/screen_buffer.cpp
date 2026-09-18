#include <sasd/ui/terminal/screen_buffer.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace sasd::ui::terminal {

ScreenBuffer::ScreenBuffer(Size size) {
    resize(size);
}

bool ScreenBuffer::contains(Point point) const noexcept {
    return point.x >= 0 && point.y >= 0 &&
           point.x < size_.width && point.y < size_.height;
}

void ScreenBuffer::resize(Size size, Cell fill) {
    const std::size_t count = checkedCellCount(size);

    /*
     * Build new storage before publishing the new dimensions. If allocation throws, the existing
     * buffer remains completely unchanged rather than exposing a Size that does not match storage.
     */
    std::vector<Cell> replacement(count, fill);

    size_ = size;
    cells_ = std::move(replacement);
}

void ScreenBuffer::clear(Cell fill) noexcept {
    std::fill(cells_.begin(), cells_.end(), fill);
}

Cell& ScreenBuffer::at(Point point) {
    return cells_.at(checkedIndex(point));
}

const Cell& ScreenBuffer::at(Point point) const {
    return cells_.at(checkedIndex(point));
}

void ScreenBuffer::set(Point point, Cell cell) {
    at(point) = cell;
}

std::span<Cell> ScreenBuffer::row(Coordinate y) {
    if (y < 0 || y >= size_.height) {
        throw std::out_of_range("terminal::ScreenBuffer::row y outside buffer");
    }

    const std::size_t width = widthAsSize();
    const std::size_t offset = static_cast<std::size_t>(y) * width;
    return std::span<Cell>{cells_}.subspan(offset, width);
}

std::span<const Cell> ScreenBuffer::row(Coordinate y) const {
    if (y < 0 || y >= size_.height) {
        throw std::out_of_range("terminal::ScreenBuffer::row y outside buffer");
    }

    const std::size_t width = widthAsSize();
    const std::size_t offset = static_cast<std::size_t>(y) * width;
    return std::span<const Cell>{cells_}.subspan(offset, width);
}

void ScreenBuffer::fill(Rect area, Cell cell) noexcept {
    if (area.isEmpty() || size_.isEmpty()) {
        return;
    }

    /*
     * Rect::contains() already uses widened arithmetic for hit-testing. fill() needs the actual
     * clipped edges, so perform the same widening explicitly before adding width/height.
     */
    using WideCoordinate = std::int64_t;

    const WideCoordinate area_left = static_cast<WideCoordinate>(area.x);
    const WideCoordinate area_top = static_cast<WideCoordinate>(area.y);
    const WideCoordinate area_right = area_left + static_cast<WideCoordinate>(area.width);
    const WideCoordinate area_bottom = area_top + static_cast<WideCoordinate>(area.height);

    const WideCoordinate left = std::max<WideCoordinate>(0, area_left);
    const WideCoordinate top = std::max<WideCoordinate>(0, area_top);
    const WideCoordinate right =
        std::min<WideCoordinate>(static_cast<WideCoordinate>(size_.width), area_right);
    const WideCoordinate bottom =
        std::min<WideCoordinate>(static_cast<WideCoordinate>(size_.height), area_bottom);

    if (left >= right || top >= bottom) {
        return;
    }

    const std::size_t width = widthAsSize();

    for (WideCoordinate y = top; y < bottom; ++y) {
        const std::size_t row_offset = static_cast<std::size_t>(y) * width;
        const std::size_t begin = row_offset + static_cast<std::size_t>(left);
        const std::size_t end = row_offset + static_cast<std::size_t>(right);

        std::fill(cells_.begin() + static_cast<std::ptrdiff_t>(begin),
                  cells_.begin() + static_cast<std::ptrdiff_t>(end),
                  cell);
    }
}

std::size_t ScreenBuffer::checkedCellCount(Size size) {
    if (size.width < 0 || size.height < 0) {
        throw std::invalid_argument("terminal::ScreenBuffer requires non-negative dimensions");
    }

    const std::size_t width = static_cast<std::size_t>(size.width);
    const std::size_t height = static_cast<std::size_t>(size.height);

    /*
     * Guard multiplication before it happens. Coordinate is currently int32_t, but size_t may be
     * narrower on a future target and the product of two otherwise valid dimensions can overflow.
     */
    if (height != 0 && width > std::numeric_limits<std::size_t>::max() / height) {
        throw std::length_error("terminal::ScreenBuffer cell count overflows size_t");
    }

    const std::size_t count = width * height;

    /*
     * std::vector may have a maximum element count smaller than SIZE_MAX / sizeof(Cell). Check that
     * limit explicitly so absurd dimensions fail as a deterministic length error instead of
     * attempting an allocation that can never be represented by the container.
     */
    const std::vector<Cell> empty;
    if (count > empty.max_size()) {
        throw std::length_error("terminal::ScreenBuffer exceeds vector maximum size");
    }

    return count;
}

std::size_t ScreenBuffer::checkedIndex(Point point) const {
    if (!contains(point)) {
        throw std::out_of_range("terminal::ScreenBuffer point outside buffer");
    }

    return static_cast<std::size_t>(point.y) * widthAsSize() +
           static_cast<std::size_t>(point.x);
}

std::size_t ScreenBuffer::widthAsSize() const noexcept {
    return static_cast<std::size_t>(size_.width);
}

} // namespace sasd::ui::terminal
