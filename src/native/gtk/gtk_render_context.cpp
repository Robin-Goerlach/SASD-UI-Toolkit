#include "gtk_render_context.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace sasd::ui::native::gtk::detail {
namespace {

struct Rgb { double r; double g; double b; };

Rgb color(Color value, bool background) noexcept {
    switch (value) {
    case Color::default_color: return background ? Rgb{0, 0, 0} : Rgb{224.0 / 255, 224.0 / 255, 224.0 / 255};
    case Color::black: return {0, 0, 0}; case Color::red: return {170.0 / 255, 0, 0};
    case Color::green: return {0, 170.0 / 255, 0}; case Color::yellow: return {170.0 / 255, 85.0 / 255, 0};
    case Color::blue: return {0, 0, 170.0 / 255}; case Color::magenta: return {170.0 / 255, 0, 170.0 / 255};
    case Color::cyan: return {0, 170.0 / 255, 170.0 / 255}; case Color::white: return {170.0 / 255, 170.0 / 255, 170.0 / 255};
    case Color::bright_black: return {85.0 / 255, 85.0 / 255, 85.0 / 255};
    case Color::bright_red: return {1, 85.0 / 255, 85.0 / 255}; case Color::bright_green: return {85.0 / 255, 1, 85.0 / 255};
    case Color::bright_yellow: return {1, 1, 85.0 / 255}; case Color::bright_blue: return {85.0 / 255, 85.0 / 255, 1};
    case Color::bright_magenta: return {1, 85.0 / 255, 1}; case Color::bright_cyan: return {85.0 / 255, 1, 1};
    case Color::bright_white: return {1, 1, 1};
    }
    return background ? Rgb{0, 0, 0} : Rgb{224.0 / 255, 224.0 / 255, 224.0 / 255};
}

std::string validUtf8(std::string_view input) {
    std::string output;
    output.reserve(input.size());
    for (std::size_t offset = 0; offset < input.size();) {
        const auto decoded = utf8::decodeOne(input, offset);
        if (decoded.consumed == 0) break;
        if (decoded.valid) output.append(input.substr(offset, decoded.consumed));
        else utf8::appendScalar(output, U'\uFFFD');
        offset += decoded.consumed;
    }
    return output;
}

void setSource(cairo_t* cairo, Color value, bool background = false) noexcept {
    const auto rgb = color(value, background);
    cairo_set_source_rgb(cairo, rgb.r, rgb.g, rgb.b);
}

void rectangle(cairo_t* cairo, Rect bounds) noexcept {
    cairo_rectangle(cairo, static_cast<double>(bounds.x), static_cast<double>(bounds.y),
                    static_cast<double>(bounds.width), static_cast<double>(bounds.height));
}

} // namespace

GtkRenderContext::GtkRenderContext(cairo_t& cairo, std::string_view font_description)
    : cairo_{&cairo}, font_{pango_font_description_from_string(std::string{font_description}.c_str())},
      line_height_{1} {
    if (font_ == nullptr) throw std::runtime_error{"Pango font description creation failed"};
    PangoLayout* layout = pango_cairo_create_layout(cairo_);
    pango_layout_set_font_description(layout, font_);
    PangoFontMetrics* metrics = pango_context_get_metrics(
        pango_layout_get_context(layout), font_, pango_language_get_default());
    line_height_ = std::max<Coordinate>(1, (pango_font_metrics_get_ascent(metrics) +
                                             pango_font_metrics_get_descent(metrics) + PANGO_SCALE - 1) /
                                            PANGO_SCALE);
    pango_font_metrics_unref(metrics);
    g_object_unref(layout);
}

void GtkRenderContext::clear(Color value) {
    setSource(cairo_, value, true);
    cairo_paint(cairo_);
}

void GtkRenderContext::fillRect(const rendered::FillRectCommand& command) {
    setSource(cairo_, command.color, command.role == rendered::FillRole::background);
    rectangle(cairo_, command.bounds);
    cairo_fill(cairo_);
}

void GtkRenderContext::strokeRect(const rendered::StrokeRectCommand& command) {
    setSource(cairo_, command.color);
    cairo_set_line_width(cairo_, static_cast<double>(std::max(Coordinate{1}, command.thickness)));
    rectangle(cairo_, command.bounds);
    cairo_stroke(cairo_);
}

void GtkRenderContext::drawText(const rendered::DrawTextCommand& command) {
    const std::string text = validUtf8(command.text);
    if (text.empty()) return;
    cairo_save(cairo_);
    if (command.clip_bounds.has_value()) {
        rectangle(cairo_, *command.clip_bounds);
        cairo_clip(cairo_);
    }
    setSource(cairo_, command.style.foreground);
    PangoLayout* layout = pango_cairo_create_layout(cairo_);
    pango_layout_set_font_description(layout, font_);
    pango_layout_set_text(layout, text.data(), static_cast<int>(text.size()));
    cairo_move_to(cairo_, command.origin.x, command.origin.y);
    pango_cairo_show_layout(cairo_, layout);
    g_object_unref(layout);
    cairo_restore(cairo_);
}

Size GtkRenderContext::measureText(std::string_view text) const {
    const std::string value = validUtf8(text);
    PangoLayout* layout = pango_cairo_create_layout(cairo_);
    pango_layout_set_font_description(layout, font_);
    pango_layout_set_text(layout, value.data(), static_cast<int>(value.size()));
    int width = 0; int height = 0;
    pango_layout_get_pixel_size(layout, &width, &height);
    g_object_unref(layout);
    if (width < 0 || height < 0 || width > std::numeric_limits<Coordinate>::max() ||
        height > std::numeric_limits<Coordinate>::max()) {
        throw std::overflow_error{"GTK text measurement is outside SASD Coordinate range"};
    }
    return {static_cast<Coordinate>(width), static_cast<Coordinate>(height)};
}

std::optional<Coordinate> GtkRenderContext::textAdvanceToScalar(std::string_view text,
                                                                 std::size_t scalar_index) const {
    if (text.find('\n') != std::string_view::npos || scalar_index > utf8::scalarCount(text)) {
        return std::nullopt;
    }
    const std::size_t bytes = utf8::byteOffsetForScalarIndex(text, scalar_index);
    const Size measured = measureText(text.substr(0, bytes));
    return measured.width;
}

GtkRenderContext::~GtkRenderContext() { pango_font_description_free(font_); }

} // namespace sasd::ui::native::gtk::detail
