#pragma once

#include <sasd/ui/measurement_context.hpp>

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
        return {
            saturatingAdd(text.width, 2),
            saturatingAdd(content_height, 2),
        };
    }

    /**
     * Measures rendered TextField chrome plus one logical unit of end-caret room.
     *
     * The extra caret unit is reserved even while the field is unfocused. This mirrors the terminal
     * backend's layout-stability rule: focusing a naturally sized field must not immediately force a
     * horizontal scroll merely because the insertion caret becomes visible.
     */
    [[nodiscard]] Size measureTextField(std::string_view utf8_text) const override {
        const Size text = measureText(utf8_text);
        const Coordinate content_height = std::max(text.height, positiveLineHeight());
        return {
            saturatingAdd(text.width, 3),
            saturatingAdd(content_height, 2),
        };
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
