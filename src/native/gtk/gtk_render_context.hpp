#pragma once

#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <cairo.h>
#include <pango/pangocairo.h>

#include <cstddef>
#include <optional>
#include <string_view>

namespace sasd::ui::native::gtk::detail {

class GtkRenderContext final {
public:
    GtkRenderContext(cairo_t& cairo, std::string_view font_description);
    ~GtkRenderContext();
    GtkRenderContext(const GtkRenderContext&) = delete;
    GtkRenderContext& operator=(const GtkRenderContext&) = delete;

    void clear(Color color);
    void fillRect(const rendered::FillRectCommand& command);
    void strokeRect(const rendered::StrokeRectCommand& command);
    void drawText(const rendered::DrawTextCommand& command);

    [[nodiscard]] Size measureText(std::string_view text) const;
    [[nodiscard]] Coordinate lineHeight() const noexcept { return line_height_; }
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view text, std::size_t scalar_index) const;

private:
    cairo_t* cairo_;
    PangoFontDescription* font_;
    Coordinate line_height_;
};

} // namespace sasd::ui::native::gtk::detail
