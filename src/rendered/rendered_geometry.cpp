#include "rendered_geometry.hpp"

#include <sasd/ui/container.hpp>
#include <sasd/ui/widget.hpp>

#include <limits>

namespace sasd::ui::rendered::detail {

std::optional<Coordinate> narrowCoordinate(std::int64_t value) noexcept {
    constexpr auto minimum =
        static_cast<std::int64_t>(std::numeric_limits<Coordinate>::min());
    constexpr auto maximum =
        static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max());

    if (value < minimum || value > maximum) {
        return std::nullopt;
    }
    return static_cast<Coordinate>(value);
}

std::optional<Rect> absoluteRectOf(const Widget& widget) noexcept {
    std::int64_t x = static_cast<std::int64_t>(widget.bounds().x);
    std::int64_t y = static_cast<std::int64_t>(widget.bounds().y);

    for (const Container* parent = widget.parent(); parent != nullptr; parent = parent->parent()) {
        x += static_cast<std::int64_t>(parent->bounds().x);
        y += static_cast<std::int64_t>(parent->bounds().y);
    }

    const auto narrowed_x = narrowCoordinate(x);
    const auto narrowed_y = narrowCoordinate(y);
    if (!narrowed_x.has_value() || !narrowed_y.has_value()) {
        return std::nullopt;
    }

    return Rect{
        *narrowed_x,
        *narrowed_y,
        widget.bounds().width,
        widget.bounds().height};
}

std::optional<Rect> insetOne(Rect bounds) noexcept {
    if (bounds.width <= 2 || bounds.height <= 2) {
        return Rect{bounds.x, bounds.y, 0, 0};
    }

    const auto x = narrowCoordinate(static_cast<std::int64_t>(bounds.x) + 1);
    const auto y = narrowCoordinate(static_cast<std::int64_t>(bounds.y) + 1);
    if (!x.has_value() || !y.has_value()) {
        return std::nullopt;
    }

    return Rect{
        *x,
        *y,
        static_cast<Coordinate>(bounds.width - 2),
        static_cast<Coordinate>(bounds.height - 2)};
}

} // namespace sasd::ui::rendered::detail
