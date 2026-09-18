#pragma once

#include <sasd/ui/geometry.hpp>

#include <cstdint>
#include <string_view>

namespace sasd::ui {

/**
 * Backend-/presentation-specific measurement services used by otherwise semantic Widgets.
 *
 * MeasurementContext deliberately contains no terminal, native-window or renderer types. A Widget
 * asks semantic questions such as "how large is this UTF-8 text in the current presentation
 * environment?" and receives logical Size values in the same coordinate system used by layout.
 *
 * Concrete contexts may represent terminal cells, rendered font metrics or native-control metrics.
 */
class MeasurementContext {
public:
    virtual ~MeasurementContext() = default;

    /**
     * Measures unwrapped UTF-8 text, honoring explicit line breaks.
     *
     * The returned Size must use non-negative logical units. Widget's normal size constraints still
     * clamp the result after this service returns.
     */
    [[nodiscard]] virtual Size measureText(std::string_view utf8_text) const = 0;

    /**
     * Identifies all measurement-affecting state for cache purposes.
     *
     * Widgets combine the dynamic MeasurementContext type with this revision. Two instances of the
     * same concrete context type and revision are therefore promising equivalent measurement
     * semantics. A mutable context must change revision whenever fonts, DPI, terminal width policy,
     * theme metrics or any other measurement-relevant state changes.
     *
     * The value does not need to be globally unique and may start at zero.
     */
    [[nodiscard]] virtual std::uint64_t revision() const noexcept = 0;
};

} // namespace sasd::ui
