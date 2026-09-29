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

std::optional<Rect> inset(Rect bounds, Coordinate amount) noexcept {
    if (amount < 0 || bounds.width < 0 || bounds.height < 0) {
        return std::nullopt;
    }
    if (amount == 0) {
        return bounds;
    }

    const std::int64_t doubled = static_cast<std::int64_t>(amount) * 2;

    /*
     * An arranged control may be smaller than its theme chrome after a parent constrains layout.
     * Keep such a control representable: presentation can still erase/outline the outer bounds while
     * text/caret drawing sees an empty interior. Do not shift the empty rectangle to an origin that
     * could overflow merely to represent "no drawable content".
     */
    if (static_cast<std::int64_t>(bounds.width) <= doubled ||
        static_cast<std::int64_t>(bounds.height) <= doubled) {
        return Rect{bounds.x, bounds.y, 0, 0};
    }

    const auto x = narrowCoordinate(
        static_cast<std::int64_t>(bounds.x) + static_cast<std::int64_t>(amount));
    const auto y = narrowCoordinate(
        static_cast<std::int64_t>(bounds.y) + static_cast<std::int64_t>(amount));
    if (!x.has_value() || !y.has_value()) {
        return std::nullopt;
    }

    return Rect{
        *x,
        *y,
        static_cast<Coordinate>(static_cast<std::int64_t>(bounds.width) - doubled),
        static_cast<Coordinate>(static_cast<std::int64_t>(bounds.height) - doubled)};
}

} // namespace sasd::ui::rendered::detail
