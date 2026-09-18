#include <sasd/ui/terminal/ansi_frame_encoder.hpp>

#include <sasd/ui/text/utf8.hpp>

#include <cstdint>
#include <string>

namespace sasd::ui::terminal {
namespace {

constexpr char escape = '\x1B';

void appendCursorPosition(std::string& output, Coordinate x, Coordinate y) {
    /*
     * ANSI/VT cursor addressing is 1-based while toolkit geometry is 0-based. Widen before adding one
     * so the conversion remains well-defined even if Coordinate eventually grows or reaches its
     * current maximum in synthetic tests.
     */
    const auto row = static_cast<std::int64_t>(y) + 1;
    const auto column = static_cast<std::int64_t>(x) + 1;

    output.push_back(escape);
    output.push_back('[');
    output += std::to_string(row);
    output.push_back(';');
    output += std::to_string(column);
    output.push_back('H');
}

} // namespace

std::string AnsiFrameEncoder::encode(const ScreenBuffer& buffer,
                                     std::optional<Point> caret) {
    std::string output;

    /*
     * Full-frame output is intentionally conservative for the first real terminal slice. A later
     * diff encoder can optimize changed runs without changing ScreenBuffer or widget presentation.
     */
    output.append("\x1B[?25l"); // hide hardware cursor during repaint
    output.append("\x1B[2J");   // clear complete display

    const Size size = buffer.size();

    for (Coordinate y = 0; y < size.height; ++y) {
        appendCursorPosition(output, 0, y);

        const auto row = buffer.row(y);
        for (const Cell& cell : row) {
            if (cell.role == CellRole::wide_continuation) {
                /*
                 * The corresponding wide_lead glyph already occupies this physical column after UTF-8
                 * output. Emitting the continuation as a blank would shift every following glyph.
                 */
                continue;
            }

            utf8::appendScalar(output, cell.code_point);
        }
    }

    if (caret.has_value() && buffer.contains(*caret)) {
        appendCursorPosition(output, caret->x, caret->y);
        output.append("\x1B[?25h"); // show cursor only at a valid semantic caret request
    }

    return output;
}

} // namespace sasd::ui::terminal
