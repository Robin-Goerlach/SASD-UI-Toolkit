#pragma once

#include <sasd/ui/geometry.hpp>

#include <cstddef>
#include <optional>

namespace sasd::ui {

class TextField;

namespace rendered {

class RenderedMeasurementContext;

namespace detail {

/**
 * Shared rendered TextField viewport/caret geometry.
 *
 * The structure is private to the Rendered implementation. Both presentation and pointer hit
 * testing consume it so they cannot silently implement different horizontal-scroll rules.
 */
struct RenderedTextFieldViewport {
    Rect bounds{};
    Rect content{};
    Point text_origin{};
    Coordinate line_height{0};
    Coordinate text_capacity{0};
    std::size_t scalar_count{0};
    std::size_t cursor_index{0};
    std::size_t start_index{0};
    Coordinate start_advance{0};
    Coordinate cursor_advance{0};
    std::optional<Rect> caret;
};

/**
 * Computes the viewport needed to keep TextField's current cursor visible.
 *
 * Only scalar boundaries through the current cursor are queried. This preserves ADR 0028's
 * deferrable capability semantics: rendering does not become dependent on a later text boundary that
 * is irrelevant to the current viewport/caret. Pointer hit testing may query additional *visible*
 * boundaries afterwards using the same start advance.
 */
[[nodiscard]] std::optional<RenderedTextFieldViewport> buildTextFieldViewport(
    const TextField& field,
    Rect bounds,
    const RenderedMeasurementContext& metrics);

} // namespace detail
} // namespace rendered
} // namespace sasd::ui
