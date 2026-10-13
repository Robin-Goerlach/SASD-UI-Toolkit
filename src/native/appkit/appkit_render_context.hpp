#pragma once

#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/render_device.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

#include <cstddef>
#include <optional>
#include <string_view>

namespace sasd::ui::native::appkit::detail {

class AppKitRenderContext final : public rendered::RenderDevice {
public:
    AppKitRenderContext(CGContextRef context, CGFloat font_size);
    ~AppKitRenderContext() override;
    AppKitRenderContext(const AppKitRenderContext&) = delete;
    AppKitRenderContext& operator=(const AppKitRenderContext&) = delete;

    void clear(Color color);
    void fillRect(const rendered::FillRectCommand& command) override;
    void strokeRect(const rendered::StrokeRectCommand& command) override;
    void drawText(const rendered::DrawTextCommand& command) override;

    [[nodiscard]] Size measureText(std::string_view text) const;
    [[nodiscard]] Coordinate lineHeight() const noexcept { return line_height_; }
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view text, std::size_t scalar_index) const;

private:
    CGContextRef context_{};
    CTFontRef font_{};
    Coordinate line_height_{1};
};

} // namespace sasd::ui::native::appkit::detail
