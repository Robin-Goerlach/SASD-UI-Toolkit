#include <sasd/ui/terminal/terminal_measurement_context.hpp>

namespace sasd::ui::terminal {

Size TerminalMeasurementContext::measureText(std::string_view utf8_text) const {
    const TextMeasurement measured = TextMetrics::measureUtf8(utf8_text, ambiguous_width_);
    return {measured.columns, measured.rows};
}

} // namespace sasd::ui::terminal
