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

Size TerminalMeasurementContext::measureTextField(std::string_view utf8_text) const {
    const TextMeasurement measured = TextMetrics::measureUtf8(utf8_text, ambiguous_width_);

    /*
     * TextField uses one left and one right delimiter. Even empty content reserves one interior cell
     * so a focused empty field can expose a real terminal-cursor position.
     */
    constexpr Coordinate chrome_width = 2;
    const Coordinate content_width = measured.columns > 0 ? measured.columns : 1;
    const Coordinate maximum = std::numeric_limits<Coordinate>::max();
    const Coordinate width =
        content_width > maximum - chrome_width
            ? maximum
            : static_cast<Coordinate>(content_width + chrome_width);

    return {width, 1};
}

} // namespace sasd::ui::terminal
