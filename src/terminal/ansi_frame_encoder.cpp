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

[[nodiscard]] int foregroundSgr(Color color) noexcept {
    switch (color) {
    case Color::black:
        return 30;
    case Color::red:
        return 31;
    case Color::green:
        return 32;
    case Color::yellow:
        return 33;
    case Color::blue:
        return 34;
    case Color::magenta:
        return 35;
    case Color::cyan:
        return 36;
    case Color::white:
        return 37;
    case Color::bright_black:
        return 90;
    case Color::bright_red:
        return 91;
    case Color::bright_green:
        return 92;
    case Color::bright_yellow:
        return 93;
    case Color::bright_blue:
        return 94;
    case Color::bright_magenta:
        return 95;
    case Color::bright_cyan:
        return 96;
    case Color::bright_white:
        return 97;
    case Color::default_color:
        return 39;
    }

    return 39;
}

void appendStyle(std::string& output, const TextStyle& style) {
    /*
     * Rebuild style from a clean SGR baseline instead of trying to invert individual previous flags.
     * The byte stream is slightly more verbose, but deterministic and much harder to leave in a stale
     * bold/dim/inverse state when adjacent cells use unrelated appearances.
     */
    output.append("\x1B[0");

    if (style.foreground != Color::default_color) {
        output.push_back(';');
        output += std::to_string(foregroundSgr(style.foreground));
    }
    if (style.bold) {
        output.append(";1");
    }
    if (style.dim) {
        output.append(";2");
    }
    if (style.underline) {
        output.append(";4");
    }
    if (style.inverse) {
        output.append(";7");
    }

    output.push_back('m');
}

} // namespace

std::string AnsiFrameEncoder::encode(const ScreenBuffer& buffer,
                                     std::optional<Point> caret) {
    std::string output;

    /*
     * Full-frame output is intentionally conservative for the first real terminal slice. Reset SGR
     * before clearing so erase behavior cannot inherit background/attributes from application output
     * that preceded this frame.
     */
    output.append("\x1B[?25l"); // hide hardware cursor during repaint
    output.append("\x1B[0m");   // establish neutral rendition before clear/draw
    output.append("\x1B[2J");   // clear complete display

    const Size size = buffer.size();
    TextStyle current_style{};

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

            if (cell.style != current_style) {
                appendStyle(output, cell.style);
                current_style = cell.style;
            }

            utf8::appendScalar(output, cell.code_point);
        }
    }

    /*
     * Never leak application styling back to the user's shell/caller. A default final cell needs no
     * extra bytes because the current state is already neutral.
     */
    if (current_style != TextStyle{}) {
        output.append("\x1B[0m");
    }

    if (caret.has_value() && buffer.contains(*caret)) {
        appendCursorPosition(output, caret->x, caret->y);
        output.append("\x1B[?25h"); // show cursor only at a valid semantic caret request
    }

    return output;
}

} // namespace sasd::ui::terminal
