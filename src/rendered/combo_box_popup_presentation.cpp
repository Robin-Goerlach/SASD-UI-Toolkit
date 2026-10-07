#include <sasd/ui/rendered/combo_box_popup_presentation.hpp>

#include "rendered_geometry.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>

namespace sasd::ui::rendered {
namespace {

[[nodiscard]] std::optional<Coordinate>
checkedAdd(Coordinate left, Coordinate right) noexcept {
    const std::int64_t value =
        static_cast<std::int64_t>(left) +
        static_cast<std::int64_t>(right);

    if (value < static_cast<std::int64_t>(std::numeric_limits<Coordinate>::min()) ||
        value > static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max())) {
        return std::nullopt;
    }
    return static_cast<Coordinate>(value);
}

[[nodiscard]] std::optional<Coordinate>
checkedMultiply(Coordinate value, std::size_t count) noexcept {
    if (value < 0) {
        return std::nullopt;
    }

    if (value == 0 || count == 0U) {
        return Coordinate{0};
    }

    const auto maximum =
        static_cast<std::uint64_t>(std::numeric_limits<Coordinate>::max());

    /*
     * Coordinate's positive range is much smaller than size_t on normal hosts. Reject an oversized
     * row count before narrowing it to uint64_t so this helper also stays correct on an exotic target
     * whose size_t is wider than 64 bits.
     */
    if (count > static_cast<std::size_t>(std::numeric_limits<Coordinate>::max())) {
        return std::nullopt;
    }

    const auto unsigned_value = static_cast<std::uint64_t>(value);
    const auto unsigned_count = static_cast<std::uint64_t>(count);
    if (unsigned_count > maximum / unsigned_value) {
        return std::nullopt;
    }

    return static_cast<Coordinate>(unsigned_value * unsigned_count);
}

[[nodiscard]] std::optional<Coordinate>
checkedDoubleAdd(Coordinate value, Coordinate symmetric_delta) noexcept {
    const auto once = checkedAdd(value, symmetric_delta);
    return once.has_value() ? checkedAdd(*once, symmetric_delta) : std::nullopt;
}

[[nodiscard]] bool exactContentInsideOuter(
    Rect outer,
    Rect content,
    Coordinate border_thickness) noexcept {
    if (outer.isEmpty() || content.isEmpty() || border_thickness < 0) {
        return false;
    }

    const auto expected = detail::inset(outer, border_thickness);
    return expected.has_value() && *expected == content;
}

struct RenderPreflightRow {
    Rect row{};
    Point text_origin{};
    Rect text_clip{};
    TextStyle text_style{};
    bool preview{false};
};

} // namespace

std::optional<RenderedComboBoxPopupPresentationSnapshot>
buildComboBoxPopupPresentation(
    const ComboBox& combo,
    Rect absolute_anchor,
    Rect viewport,
    const RenderedMeasurementContext& metrics,
    presentation::PopupVerticalSide preferred_side) {
    if (!combo.isDropDownOpen() ||
        !combo.hasFocus() ||
        !combo.isEnabled() ||
        !combo.isVisible() ||
        absolute_anchor.isEmpty() ||
        viewport.isEmpty()) {
        return std::nullopt;
    }

    const RenderedThemeMetrics theme = metrics.themeMetrics().normalized();
    const Coordinate border = theme.control_border_thickness;
    const Coordinate padding = std::max(Coordinate{1}, border);
    const Coordinate line_height = std::max(Coordinate{1}, metrics.lineHeight());

    RenderedComboBoxPopupPresentationSnapshot snapshot;
    snapshot.preview_index = combo.previewIndex();
    snapshot.text_style = combo.textStyle();
    snapshot.side = preferred_side;
    snapshot.padding = padding;
    snapshot.border_thickness = border;
    snapshot.items.reserve(combo.itemCount());

    Coordinate maximum_text_width = 0;
    Coordinate maximum_text_height = line_height;

    for (std::size_t index = 0; index < combo.itemCount(); ++index) {
        const std::string_view text = combo.itemAt(index);
        const Size measured = metrics.measureText(text);

        /*
         * MeasurementContext is an extension point. Reject malformed custom providers explicitly:
         * negative extents cannot form a drawable fixed-row contract and must not be converted into
         * apparently valid popup geometry by later additions.
         */
        if (measured.width < 0 || measured.height < 0) {
            return std::nullopt;
        }

        maximum_text_width = std::max(maximum_text_width, measured.width);
        maximum_text_height = std::max(maximum_text_height, measured.height);

        snapshot.items.push_back(
            RenderedComboBoxPopupItemPresentation{
                std::string{text},
                measured,
            });
    }

    if (snapshot.preview_index.has_value() &&
        *snapshot.preview_index >= snapshot.items.size()) {
        return std::nullopt;
    }

    const auto row_height =
        checkedDoubleAdd(maximum_text_height, padding);
    const auto natural_content_width =
        checkedDoubleAdd(maximum_text_width, padding);
    if (!row_height.has_value() ||
        !natural_content_width.has_value() ||
        *row_height <= 0 ||
        *natural_content_width <= 0) {
        return std::nullopt;
    }
    snapshot.row_height = *row_height;

    /*
     * Empty open ComboBox is a valid semantic transaction but has no rows to present. Preserve the
     * copied/style/metric policy in an owned no-overlay snapshot rather than inventing a placeholder.
     */
    if (snapshot.items.empty()) {
        return snapshot;
    }

    const auto rows_height =
        checkedMultiply(snapshot.row_height, snapshot.items.size());
    if (!rows_height.has_value() || *rows_height <= 0) {
        return std::nullopt;
    }

    const auto natural_outer_width =
        checkedDoubleAdd(*natural_content_width, border);
    const auto outer_height =
        checkedDoubleAdd(*rows_height, border);
    if (!natural_outer_width.has_value() ||
        !outer_height.has_value() ||
        *natural_outer_width <= 0 ||
        *outer_height <= 0) {
        return std::nullopt;
    }

    const Size popup_size{
        std::max(*natural_outer_width, absolute_anchor.width),
        *outer_height,
    };

    const auto placement = presentation::placeAnchoredPopup(
        absolute_anchor,
        popup_size,
        viewport,
        preferred_side);
    if (!placement.has_value()) {
        return std::nullopt;
    }

    const auto content = detail::inset(
        placement->bounds,
        border);
    if (!content.has_value() || content->isEmpty()) {
        return std::nullopt;
    }

    /*
     * Anchor-width expansion is horizontal only. The content height must still be exactly the declared
     * fixed-row stack; asserting that relation here keeps later hit testing/rendering deterministic.
     */
    if (content->height != *rows_height) {
        return std::nullopt;
    }

    snapshot.bounds = placement->bounds;
    snapshot.content_bounds = *content;
    snapshot.side = placement->side;
    return snapshot;
}

bool renderComboBoxPopupPresentation(
    DisplayList& display_list,
    const RenderedComboBoxPopupPresentationSnapshot& snapshot,
    Color background_color) {
    if (snapshot.padding <= 0 || snapshot.border_thickness < 0 ||
        snapshot.row_height <= 0) {
        return false;
    }

    if (snapshot.preview_index.has_value() &&
        *snapshot.preview_index >= snapshot.items.size()) {
        return false;
    }

    if (snapshot.items.empty()) {
        return snapshot.bounds.isEmpty() &&
               snapshot.content_bounds.isEmpty() &&
               !snapshot.preview_index.has_value();
    }

    if (!exactContentInsideOuter(
            snapshot.bounds,
            snapshot.content_bounds,
            snapshot.border_thickness)) {
        return false;
    }

    const auto expected_content_height =
        checkedMultiply(snapshot.row_height, snapshot.items.size());
    if (!expected_content_height.has_value() ||
        snapshot.content_bounds.height != *expected_content_height) {
        return false;
    }

    const auto double_padding =
        checkedAdd(snapshot.padding, snapshot.padding);
    if (!double_padding.has_value() ||
        snapshot.content_bounds.width < *double_padding ||
        snapshot.row_height < *double_padding) {
        return false;
    }

    const Coordinate available_text_width =
        static_cast<Coordinate>(
            snapshot.content_bounds.width - *double_padding);
    const Coordinate available_text_height =
        static_cast<Coordinate>(
            snapshot.row_height - *double_padding);

    /*
     * Store complete row/text geometry in local values first. DisplayList has no rollback API, so
     * structural failure must be discovered before fillRect/strokeRect/drawText append anything.
     */
    std::vector<RenderPreflightRow> rows;
    rows.reserve(snapshot.items.size());

    for (std::size_t index = 0; index < snapshot.items.size(); ++index) {
        const auto row = presentation::fixedPopupRowBounds(
            snapshot.content_bounds,
            snapshot.items.size(),
            index,
            snapshot.row_height);
        if (!row.has_value()) {
            return false;
        }

        const Size text_size = snapshot.items[index].text_size;
        if (text_size.width < 0 ||
            text_size.height < 0 ||
            text_size.width > available_text_width ||
            text_size.height > available_text_height) {
            return false;
        }

        const auto text_x = detail::narrowCoordinate(
            static_cast<std::int64_t>(row->x) +
            static_cast<std::int64_t>(snapshot.padding));
        const auto clip_y = detail::narrowCoordinate(
            static_cast<std::int64_t>(row->y) +
            static_cast<std::int64_t>(snapshot.padding));
        const auto text_y = detail::narrowCoordinate(
            static_cast<std::int64_t>(row->y) +
            static_cast<std::int64_t>(snapshot.padding) +
            static_cast<std::int64_t>(
                (available_text_height - text_size.height) / 2));
        if (!text_x.has_value() ||
            !clip_y.has_value() ||
            !text_y.has_value()) {
            return false;
        }

        const Rect text_clip{
            *text_x,
            *clip_y,
            available_text_width,
            available_text_height,
        };

        TextStyle row_style = snapshot.text_style;
        const bool preview =
            snapshot.preview_index == std::optional<std::size_t>{index};
        if (preview) {
            row_style.inverse = !row_style.inverse;
        }

        rows.push_back(RenderPreflightRow{
            *row,
            Point{*text_x, *text_y},
            text_clip,
            row_style,
            preview,
        });
    }

    /*
     * Everything that can reject the snapshot is resolved above. The popup is opaque over the base
     * display list, then receives one outer border and deterministic row commands.
     */
    display_list.fillRect(snapshot.bounds, background_color);

    if (snapshot.border_thickness > 0) {
        display_list.strokeRect(
            snapshot.bounds,
            snapshot.text_style.foreground,
            snapshot.border_thickness);
    }

    for (std::size_t index = 0; index < rows.size(); ++index) {
        const RenderPreflightRow& row = rows[index];

        if (row.preview) {
            /*
             * The current style model has no semantic background brush. A one-unit row outline makes
             * full-row preview geometry visible without hard-coding a palette color. Toggled inverse
             * text supplies the stronger local contrast and remains visible for already-inverse base
             * styles.
             */
            display_list.strokeRect(
                row.row,
                snapshot.text_style.foreground,
                1);
        }

        display_list.drawText(
            row.text_origin,
            snapshot.items[index].text,
            row.text_style,
            row.text_clip);
    }

    return true;
}

std::optional<DisplayList>
composeComboBoxPopupDisplayList(
    const DisplayList& base,
    const ComboBox& combo,
    Rect absolute_anchor,
    Rect viewport,
    const RenderedMeasurementContext& metrics,
    Color background_color,
    presentation::PopupVerticalSide preferred_side) {
    if (!combo.isDropDownOpen()) {
        return base;
    }

    const auto snapshot = buildComboBoxPopupPresentation(
        combo,
        absolute_anchor,
        viewport,
        metrics,
        preferred_side);
    if (!snapshot.has_value()) {
        return std::nullopt;
    }

    DisplayList result = base;
    if (!renderComboBoxPopupPresentation(
            result,
            *snapshot,
            background_color)) {
        return std::nullopt;
    }

    return result;
}

} // namespace sasd::ui::rendered
