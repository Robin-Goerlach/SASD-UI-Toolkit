#include "rendered_text_field_viewport.hpp"

#include "rendered_geometry.hpp"

#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/text_field.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace sasd::ui::rendered::detail {

std::optional<RenderedTextFieldViewport> buildTextFieldViewport(
    const TextField& field,
    Rect bounds,
    const RenderedMeasurementContext& metrics) {
    const auto content = insetOne(bounds);
    if (!content.has_value()) {
        return std::nullopt;
    }

    RenderedTextFieldViewport result;
    result.bounds = bounds;
    result.content = *content;
    result.text_origin = {content->x, content->y};

    /*
     * A tiny control can still represent its border/focus state. There is no text interior to hit or
     * draw, so no font query is necessary.
     */
    if (content->isEmpty()) {
        return result;
    }

    result.line_height = metrics.lineHeight();
    if (result.line_height <= 0) {
        return std::nullopt;
    }

    result.scalar_count = utf8::scalarCount(field.text());
    result.cursor_index = std::min(field.cursorPosition(), result.scalar_count);

    /*
     * Query scalar boundaries in the complete shaped run, never independently measured prefixes.
     * Only boundaries required to position the current cursor are needed for presentation.
     */
    std::vector<Coordinate> advances;
    advances.reserve(result.cursor_index + 1);

    for (std::size_t index = 0; index <= result.cursor_index; ++index) {
        const auto advance = metrics.textAdvanceToScalar(field.text(), index);
        if (!advance.has_value() ||
            *advance < 0 ||
            (index == 0 && *advance != 0) ||
            (!advances.empty() && *advance < advances.back())) {
            return std::nullopt;
        }
        advances.push_back(*advance);
    }

    result.cursor_advance = advances.back();
    result.text_capacity =
        content->width > 0 ? static_cast<Coordinate>(content->width - 1) : 0;

    result.start_index = 0;
    while (result.start_index < result.cursor_index) {
        const std::int64_t visible_advance =
            static_cast<std::int64_t>(result.cursor_advance) -
            static_cast<std::int64_t>(advances[result.start_index]);

        if (visible_advance <= static_cast<std::int64_t>(result.text_capacity)) {
            break;
        }
        ++result.start_index;
    }

    result.start_advance = advances[result.start_index];

    const auto text_x = narrowCoordinate(
        static_cast<std::int64_t>(content->x) -
        static_cast<std::int64_t>(result.start_advance));
    if (!text_x.has_value()) {
        return std::nullopt;
    }
    result.text_origin.x = *text_x;

    if (field.hasFocus()) {
        const std::int64_t relative_caret =
            static_cast<std::int64_t>(result.cursor_advance) -
            static_cast<std::int64_t>(result.start_advance);

        const auto caret_x = narrowCoordinate(
            static_cast<std::int64_t>(content->x) + relative_caret);
        if (!caret_x.has_value() ||
            relative_caret < 0 ||
            relative_caret >= static_cast<std::int64_t>(content->width)) {
            return std::nullopt;
        }

        const Coordinate caret_height =
            std::min(result.line_height, content->height);
        if (caret_height > 0) {
            result.caret = Rect{*caret_x, content->y, 1, caret_height};
        }
    }

    return result;
}

} // namespace sasd::ui::rendered::detail
