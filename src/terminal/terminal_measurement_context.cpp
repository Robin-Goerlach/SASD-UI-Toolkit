#include <sasd/ui/terminal/terminal_measurement_context.hpp>

#include <limits>

namespace sasd::ui::terminal {

Size TerminalMeasurementContext::measureText(std::string_view utf8_text) const {
    const TextMeasurement measured = TextMetrics::measureUtf8(utf8_text, ambiguous_width_);
    return {measured.columns, measured.rows};
}

Size TerminalMeasurementContext::measureButton(std::string_view utf8_text) const {
    const TextMeasurement measured = TextMetrics::measureUtf8(utf8_text, ambiguous_width_);

    /*
     * Terminal Button presentation uses two one-cell delimiters and two one-cell spaces around the
     * caption. Saturate instead of overflowing Coordinate if synthetic/pathological text reaches the
     * representable limit.
     */
    constexpr Coordinate chrome_width = 4;
    const Coordinate maximum = std::numeric_limits<Coordinate>::max();
    const Coordinate width =
        measured.columns > maximum - chrome_width
            ? maximum
            : static_cast<Coordinate>(measured.columns + chrome_width);

    return {width, measured.rows};
}

} // namespace sasd::ui::terminal
