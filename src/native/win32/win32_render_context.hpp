#pragma once

#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <windows.h>

#include <cstddef>
#include <optional>
#include <string_view>

namespace sasd::ui::native::win32::detail {

/** GDI implementation of the backend-neutral RenderDevice/measurement contract. */
class Win32RenderContext final {
public:
    Win32RenderContext(HDC device_context, HFONT font, UINT dpi) noexcept;

    void clear(Color color);
    void fillRect(const rendered::FillRectCommand& command);
    void strokeRect(const rendered::StrokeRectCommand& command);
    void drawText(const rendered::DrawTextCommand& command);

    [[nodiscard]] Size measureText(std::string_view utf8_text) const;
    [[nodiscard]] Coordinate lineHeight() const noexcept { return line_height_; }
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view utf8_text,
        std::size_t scalar_index) const;

private:
    [[nodiscard]] HFONT selectFont() const noexcept;
    [[nodiscard]] RECT toPhysical(Rect value) const;
    [[nodiscard]] POINT toPhysical(Point value) const;
    [[nodiscard]] COLORREF color(Color value, bool background) const noexcept;

    HDC device_context_;
    HFONT font_;
    UINT dpi_;
    Coordinate line_height_;
};

} // namespace sasd::ui::native::win32::detail
