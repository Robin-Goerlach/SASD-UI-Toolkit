#pragma once

#include <cstdint>
#include <limits>

namespace sasd::ui {

/**
 * Signed coordinate type used by the backend-neutral geometry primitives.
 *
 * Coordinates are deliberately logical units rather than pixels. A desktop backend may interpret
 * them as device-independent units while a terminal backend may interpret them as character cells.
 */
using Coordinate = std::int32_t;

/** A backend-neutral point in logical UI coordinates. */
struct Point {
    Coordinate x{0};
    Coordinate y{0};

    friend constexpr bool operator==(const Point&, const Point&) = default;
};

/** A backend-neutral two-dimensional extent in logical UI units. */
struct Size {
    Coordinate width{0};
    Coordinate height{0};

    /**
     * Returns true when the size cannot describe a positive two-dimensional area.
     *
     * Negative extents are treated as empty as well. Keeping this definition explicit prevents
     * later layout and hit-testing code from accidentally treating malformed extents as visible.
     */
    [[nodiscard]] constexpr bool isEmpty() const noexcept {
        return width <= 0 || height <= 0;
    }

    friend constexpr bool operator==(const Size&, const Size&) = default;
};

/**
 * Axis-aligned rectangle using half-open bounds: [x, x + width) x [y, y + height).
 *
 * Half-open bounds make adjacent rectangles share an edge without sharing a point. The arithmetic
 * in contains() is intentionally widened before computing the far edges so values close to the
 * Coordinate limits cannot trigger signed integer overflow.
 */
struct Rect {
    Coordinate x{0};
    Coordinate y{0};
    Coordinate width{0};
    Coordinate height{0};

    /** Returns true when the rectangle has no positive area. */
    [[nodiscard]] constexpr bool isEmpty() const noexcept {
        return width <= 0 || height <= 0;
    }

    /**
     * Tests whether a point lies inside this rectangle's half-open bounds.
     *
     * Empty or malformed rectangles never contain a point. Edge calculations use int64_t because
     * Coordinate is int32_t; therefore x + width and y + height remain representable even when the
     * rectangle reaches beyond the positive or negative Coordinate limit.
     */
    [[nodiscard]] constexpr bool contains(Point point) const noexcept {
        if (isEmpty()) {
            return false;
        }

        using WideCoordinate = std::int64_t;
        const auto left = static_cast<WideCoordinate>(x);
        const auto top = static_cast<WideCoordinate>(y);
        const auto right = left + static_cast<WideCoordinate>(width);
        const auto bottom = top + static_cast<WideCoordinate>(height);
        const auto point_x = static_cast<WideCoordinate>(point.x);
        const auto point_y = static_cast<WideCoordinate>(point.y);

        return point_x >= left && point_y >= top && point_x < right && point_y < bottom;
    }

    [[nodiscard]] constexpr Size size() const noexcept {
        return {width, height};
    }

    friend constexpr bool operator==(const Rect&, const Rect&) = default;
};

/**
 * Minimum, preferred and maximum size hints used by backend-neutral layout code.
 *
 * This remains a lightweight aggregate so callers can construct constraints naturally. Because an
 * aggregate cannot prevent a caller from supplying minimum > maximum, clamp() is defensive and
 * never delegates invalid bounds to std::clamp (whose contract requires an ordered range).
 */
struct SizeConstraints {
    Size minimum{0, 0};
    Size preferred{0, 0};
    Size maximum{std::numeric_limits<Coordinate>::max(), std::numeric_limits<Coordinate>::max()};

    /** Returns whether each minimum dimension is less than or equal to its maximum dimension. */
    [[nodiscard]] constexpr bool hasValidRange() const noexcept {
        return minimum.width <= maximum.width && minimum.height <= maximum.height;
    }

    /**
     * Clamps a size to the configured minimum/maximum range.
     *
     * A range with minimum > maximum is invalid and has no mathematically correct clamp result. To
     * keep this low-level value type deterministic and free of undefined behavior, that dimension
     * collapses to its declared minimum. Higher-level layout code should detect invalid constraints
     * with hasValidRange() and report/reject them rather than relying on this fallback.
     */
    [[nodiscard]] constexpr Size clamp(Size value) const noexcept {
        return {
            clampDimension(value.width, minimum.width, maximum.width),
            clampDimension(value.height, minimum.height, maximum.height),
        };
    }

private:
    [[nodiscard]] static constexpr Coordinate clampDimension(Coordinate value,
                                                             Coordinate minimum_value,
                                                             Coordinate maximum_value) noexcept {
        if (minimum_value > maximum_value) {
            return minimum_value;
        }
        if (value < minimum_value) {
            return minimum_value;
        }
        if (value > maximum_value) {
            return maximum_value;
        }
        return value;
    }
};

} // namespace sasd::ui
