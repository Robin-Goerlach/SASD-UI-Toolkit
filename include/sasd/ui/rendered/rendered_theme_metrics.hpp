#pragma once

#include <sasd/ui/geometry.hpp>

namespace sasd::ui::rendered {

/**
 * Small geometry-only theme policy for the current rendered control chrome.
 *
 * This is deliberately not a complete theme system. Colors, brushes, fonts, platform appearance and
 * style inheritance remain separate concerns. M3 needs only the few logical geometry values that are
 * already observable in both intrinsic measurement and presentation.
 *
 * A RenderedMeasurementContext supplies one snapshot of these values so layout, drawing and pointer
 * hit testing can share the same policy without leaking renderer/native types into Core widgets.
 */
struct RenderedThemeMetrics {
    /** Logical outline thickness used by current rendered Button/TextField chrome. */
    Coordinate control_border_thickness{1};

    /** Logical caption offset used only while a rendered Button is pressed. */
    Coordinate button_pressed_offset{1};

    /** Logical width reserved and painted for a rendered TextField insertion caret. */
    Coordinate text_field_caret_width{1};

    /**
     * Returns a defensive, drawable version of the metric set.
     *
     * Theme providers are expected to return non-negative geometry and a positive caret width.
     * Normalizing at the Rendered boundary keeps malformed/custom providers from manufacturing
     * negative rectangles or non-positive DisplayList stroke/caret primitives.
     */
    [[nodiscard]] constexpr RenderedThemeMetrics normalized() const noexcept {
        return {
            control_border_thickness > 0 ? control_border_thickness : Coordinate{0},
            button_pressed_offset > 0 ? button_pressed_offset : Coordinate{0},
            text_field_caret_width > 0 ? text_field_caret_width : Coordinate{1},
        };
    }

    friend constexpr bool operator==(const RenderedThemeMetrics&,
                                     const RenderedThemeMetrics&) = default;
};

} // namespace sasd::ui::rendered
