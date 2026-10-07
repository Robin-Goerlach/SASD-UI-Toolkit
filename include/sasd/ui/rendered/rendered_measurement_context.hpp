#pragma once

#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/rendered/rendered_theme_metrics.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <string_view>

namespace sasd::ui::rendered {

/**
 * MeasurementContext specialization for rendered-desktop text and control metrics.
 *
 * Core Widgets continue to depend only on MeasurementContext. RenderedPresentationSink uses this
 * narrower derived contract when it additionally needs caret geometry for a TextField. A concrete
 * device/font adapter therefore provides one coherent metric source to both layout and presentation
 * without leaking SDL, FreeType, HarfBuzz or native font handles into semantic Widgets.
 *
 * The initial M3 TextField path is deliberately a single-line, left-to-right contract. Rich
 * bidirectional visual ordering requires a richer shaped-text object and is intentionally deferred
 * rather than approximated in the Core API.
 */
class RenderedMeasurementContext : public MeasurementContext {
public:
    ~RenderedMeasurementContext() override = default;

    /**
     * Returns the logical line height of the active rendered font metrics.
     *
     * The value must be strictly positive and use the same logical coordinate system as measureText()
     * and Widget bounds. It lets empty controls reserve real text/caret height without inventing a
     * sample glyph such as "M".
     */
    [[nodiscard]] virtual Coordinate lineHeight() const noexcept = 0;

    /**
     * Returns geometry-only control theme metrics used by rendered measurement and presentation.
     *
     * The default preserves the original M3 one-unit chrome. A concrete rendered environment may
     * override this when its theme requires different logical border/caret geometry. If those values
     * can change at runtime, revision() must change with them so Widget measurement caches cannot
     * retain sizes produced by an obsolete theme.
     */
    [[nodiscard]] virtual RenderedThemeMetrics themeMetrics() const noexcept {
        return {};
    }

    /**
     * Returns the horizontal logical advance from the text origin to a Unicode-scalar boundary.
     *
     * scalar_index is in [0, utf8::scalarCount(utf8_text)]. For one unchanged string, boundary zero
     * must resolve to logical advance zero; subsequent available advances must be non-negative and
     * monotonically non-decreasing. A real font implementation should obtain these values from the
     * same shaping/layout result it uses to draw the complete string, so kerning and ligatures do not
     * have to be guessed by TextField.
     *
     * std::nullopt explicitly means that the current metric/shaping provider cannot represent this
     * caret boundary with the initial M3 left-to-right advance model. This is important for future
     * bidirectional or otherwise complex visual ordering: the sink can keep the TextField deferred
     * instead of manufacturing an incorrect caret. A later shaping milestone may replace this narrow
     * query with a richer visual-caret map.
     */
    [[nodiscard]] virtual std::optional<Coordinate> textAdvanceToScalar(
        std::string_view utf8_text,
        std::size_t scalar_index) const = 0;

    /**
     * Measures rendered Button chrome used by RenderedPresentationSink.
     *
     * One logical unit is reserved on every side for the current border/inset contract. Saturating
     * arithmetic keeps pathological metric values from overflowing Coordinate.
     */
    [[nodiscard]] Size measureButton(std::string_view utf8_text) const override {
        const Size text = measureText(utf8_text);
        const Coordinate content_height = std::max(text.height, positiveLineHeight());
        const RenderedThemeMetrics theme = themeMetrics().normalized();

        /*
         * Apply the same border on both sides using repeated saturating addition instead of
         * multiplying first. Coordinate is signed and a theme provider may legally choose a large
         * logical border; widened/unchecked arithmetic must not overflow before saturation.
         */
        Coordinate width = saturatingAdd(text.width, theme.control_border_thickness);
        width = saturatingAdd(width, theme.control_border_thickness);

        Coordinate height = saturatingAdd(content_height, theme.control_border_thickness);
        height = saturatingAdd(height, theme.control_border_thickness);

        return {width, height};
    }

    /**
     * Measures the collapsed rendered ComboBox using the same font/theme source as presentation.
     *
     * The selected text area is followed by one logical gap and a stable indicator lane. The lane is
     * at least one line-height wide so the drop affordance remains visible even for an empty item. The
     * complete content is then wrapped in the normal control border. ComboBox itself asks this hook for
     * every owned item and keeps the component-wise maximum, so changing selection never changes the
     * intrinsic size.
     */
    [[nodiscard]] Size measureComboBox(std::string_view utf8_text) const override {
        const Size text = measureText(utf8_text);
        const Size indicator_text = measureText("v");
        const Coordinate line_height = positiveLineHeight();
        const Coordinate content_height =
            std::max(std::max(text.height, indicator_text.height), line_height);
        const Coordinate indicator_width =
            std::max(indicator_text.width, line_height);
        const RenderedThemeMetrics theme = themeMetrics().normalized();
        const Coordinate gap =
            std::max(Coordinate{1}, theme.control_border_thickness);

        Coordinate width = saturatingAdd(text.width, gap);
        width = saturatingAdd(width, indicator_width);
        width = saturatingAdd(width, theme.control_border_thickness);
        width = saturatingAdd(width, theme.control_border_thickness);

        Coordinate height = saturatingAdd(content_height, theme.control_border_thickness);
        height = saturatingAdd(height, theme.control_border_thickness);

        return {width, height};
    }

    /**
     * Measures rendered TextField chrome plus one logical unit of end-caret room.
     *
     * The extra caret unit is reserved even while the field is unfocused. This mirrors the terminal
     * backend's layout-stability rule: focusing a naturally sized field must not immediately force a
     * horizontal scroll merely because the insertion caret becomes visible.
     */
    /**
     * Measures the first rendered CheckBox presentation.
     *
     * The indicator is a square whose side follows the current font line height. One logical gap is
     * always reserved between indicator and caption; if the active control border is thicker than
     * that, the border thickness becomes the gap. The same policy is repeated by
     * RenderedPresentationSink so measurement and drawing agree without leaking CheckBox geometry
     * into Core.
     */
    [[nodiscard]] Size measureCheckBox(std::string_view utf8_text) const override {
        const Size text = measureText(utf8_text);
        const Coordinate indicator = positiveLineHeight();
        const RenderedThemeMetrics theme = themeMetrics().normalized();
        const Coordinate gap =
            std::max(Coordinate{1}, theme.control_border_thickness);

        Coordinate width = saturatingAdd(indicator, gap);
        width = saturatingAdd(width, text.width);

        return {width, std::max(text.height, indicator)};
    }

    /**
     * Measures the initial rendered RadioButton using the same line-height-sized selector box and
     * gap policy as CheckBox.
     *
     * Sharing geometry at this stage keeps layout predictable while presentation remains free to use
     * a smaller centered selection mark so radio and checkbox state are visually distinguishable.
     */
    [[nodiscard]] Size measureRadioButton(std::string_view utf8_text) const override {
        const Size text = measureText(utf8_text);
        const Coordinate indicator = positiveLineHeight();
        const RenderedThemeMetrics theme = themeMetrics().normalized();
        const Coordinate gap =
            std::max(Coordinate{1}, theme.control_border_thickness);

        Coordinate width = saturatingAdd(indicator, gap);
        width = saturatingAdd(width, text.width);

        return {width, std::max(text.height, indicator)};
    }

    [[nodiscard]] Size measureTextField(std::string_view utf8_text) const override {
        const Size text = measureText(utf8_text);
        const Coordinate content_height = std::max(text.height, positiveLineHeight());
        const RenderedThemeMetrics theme = themeMetrics().normalized();

        Coordinate width = saturatingAdd(text.width, theme.control_border_thickness);
        width = saturatingAdd(width, theme.control_border_thickness);
        width = saturatingAdd(width, theme.text_field_caret_width);

        Coordinate height = saturatingAdd(content_height, theme.control_border_thickness);
        height = saturatingAdd(height, theme.control_border_thickness);

        return {width, height};
    }

private:
    [[nodiscard]] Coordinate positiveLineHeight() const noexcept {
        const Coordinate value = lineHeight();
        return value > 0 ? value : 1;
    }

    [[nodiscard]] static Coordinate saturatingAdd(Coordinate value,
                                                  Coordinate delta) noexcept {
        if (value <= 0) {
            return delta;
        }

        constexpr Coordinate maximum = std::numeric_limits<Coordinate>::max();
        return value > maximum - delta
                   ? maximum
                   : static_cast<Coordinate>(value + delta);
    }
};

} // namespace sasd::ui::rendered
