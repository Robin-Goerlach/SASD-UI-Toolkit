#pragma once

#include <sasd/ui/geometry.hpp>

#include <cstdint>
#include <optional>

namespace sasd::ui {

class Widget;

namespace rendered::detail {

/** Safely narrows widened rendered-coordinate arithmetic back to public Coordinate. */
[[nodiscard]] std::optional<Coordinate> narrowCoordinate(std::int64_t value) noexcept;

/**
 * Resolves parent-relative Widget bounds into the rendered root coordinate system.
 *
 * Parent offsets are accumulated in 64 bits; unrepresentable final origins return nullopt rather
 * than wrapping to unrelated screen coordinates.
 */
[[nodiscard]] std::optional<Rect> absoluteRectOf(const Widget& widget) noexcept;

/** Returns a one-unit interior rectangle, preserving tiny controls as an empty interior. */
[[nodiscard]] std::optional<Rect> insetOne(Rect bounds) noexcept;

} // namespace rendered::detail
} // namespace sasd::ui
