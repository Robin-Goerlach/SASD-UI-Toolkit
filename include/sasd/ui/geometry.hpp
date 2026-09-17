#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace sasd::ui {

using Coordinate = std::int32_t;

struct Point {
    Coordinate x{0};
    Coordinate y{0};

    friend constexpr bool operator==(const Point&, const Point&) = default;
};

struct Size {
    Coordinate width{0};
    Coordinate height{0};

    [[nodiscard]] constexpr bool isEmpty() const noexcept {
        return width <= 0 || height <= 0;
    }

    friend constexpr bool operator==(const Size&, const Size&) = default;
};

struct Rect {
    Coordinate x{0};
    Coordinate y{0};
    Coordinate width{0};
    Coordinate height{0};

    [[nodiscard]] constexpr bool contains(Point point) const noexcept {
        return point.x >= x && point.y >= y && point.x < x + width && point.y < y + height;
    }

    [[nodiscard]] constexpr Size size() const noexcept {
        return {width, height};
    }

    friend constexpr bool operator==(const Rect&, const Rect&) = default;
};

struct SizeConstraints {
    Size minimum{0, 0};
    Size preferred{0, 0};
    Size maximum{std::numeric_limits<Coordinate>::max(), std::numeric_limits<Coordinate>::max()};

    [[nodiscard]] constexpr Size clamp(Size value) const noexcept {
        return {
            std::clamp(value.width, minimum.width, maximum.width),
            std::clamp(value.height, minimum.height, maximum.height),
        };
    }
};

} // namespace sasd::ui
