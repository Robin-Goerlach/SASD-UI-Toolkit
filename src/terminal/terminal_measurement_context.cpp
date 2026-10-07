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

Size TerminalMeasurementContext::measureComboBox(std::string_view utf8_text) const {
    const TextMeasurement measured = TextMetrics::measureUtf8(utf8_text, ambiguous_width_);

    /*
     * Collapsed ComboBox presentation is "[ choice v ]". The six fixed cells are the two
     * delimiters, the leading/trailing interior gaps, one gap before the indicator and the ASCII
     * drop indicator itself. Saturating mirrors the other terminal control-specific measurements.
     */
    constexpr Coordinate chrome_width = 6;
    const Coordinate maximum = std::numeric_limits<Coordinate>::max();
    const Coordinate width =
        measured.columns > maximum - chrome_width
            ? maximum
            : static_cast<Coordinate>(measured.columns + chrome_width);

    return {width, measured.rows};
}

Size TerminalMeasurementContext::measureCheckBox(std::string_view utf8_text) const {
    const TextMeasurement measured = TextMetrics::measureUtf8(utf8_text, ambiguous_width_);

    /*
     * "[x] " is four terminal columns. All presentation states deliberately preserve that width so
     * checking, focus, pointer press or disabling the control cannot invalidate layout by itself.
     */
    constexpr Coordinate indicator_and_gap = 4;
    const Coordinate maximum = std::numeric_limits<Coordinate>::max();
    const Coordinate width =
        measured.columns > maximum - indicator_and_gap
            ? maximum
            : static_cast<Coordinate>(measured.columns + indicator_and_gap);

    return {width, measured.rows};
}

Size TerminalMeasurementContext::measureRadioButton(std::string_view utf8_text) const {
    const TextMeasurement measured = TextMetrics::measureUtf8(utf8_text, ambiguous_width_);

    /*
     * "(o) " is four terminal columns, matching the initial CheckBox footprint while remaining a
     * separate measurement override. Keeping the explicit method matters for future backends/themes
     * where radio and checkbox chrome may diverge.
     */
    constexpr Coordinate indicator_and_gap = 4;
    const Coordinate maximum = std::numeric_limits<Coordinate>::max();
    const Coordinate width =
        measured.columns > maximum - indicator_and_gap
            ? maximum
            : static_cast<Coordinate>(measured.columns + indicator_and_gap);

    return {width, measured.rows};
}

Size TerminalMeasurementContext::measureTextField(std::string_view utf8_text) const {
    const TextMeasurement measured = TextMetrics::measureUtf8(utf8_text, ambiguous_width_);

    /*
     * TextField uses two delimiter cells plus one interior caret cell. The caret cell is reserved even
     * while unfocused so focus changes do not alter desired size or force immediate scrolling at the
     * natural end-of-text position.
     */
    constexpr Coordinate fixed_width = 3;
    const Coordinate maximum = std::numeric_limits<Coordinate>::max();
    const Coordinate width =
        measured.columns > maximum - fixed_width
            ? maximum
            : static_cast<Coordinate>(measured.columns + fixed_width);

    return {width, 1};
}

} // namespace sasd::ui::terminal
