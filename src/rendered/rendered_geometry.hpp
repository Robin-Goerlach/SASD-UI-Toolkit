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

/**
 * Returns an interior rectangle inset by a non-negative logical amount on every side.
 *
 * Tiny controls preserve their outer origin and become an empty interior instead of producing
 * negative extents. Invalid negative input or unrepresentable translated origins return nullopt.
 */
[[nodiscard]] std::optional<Rect> inset(Rect bounds, Coordinate amount) noexcept;

} // namespace rendered::detail
} // namespace sasd::ui
