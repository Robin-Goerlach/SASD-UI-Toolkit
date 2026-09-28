#pragma once

#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

struct SDL_Renderer;
struct TTF_Font;

namespace sasd::ui::rendered::sdl3::detail {

/**
 * Shared command-execution and font-metric core for SDL3 rendered devices.
 *
 * The class owns neither renderer nor font. Concrete adapter hosts (the current headless software
 * device and the next window-backed device) own those native resources and keep them alive for this
 * object's complete lifetime.
 *
 * Centralizing this code is more than deduplication: drawing and measurement must use the same font,
 * clipping and style policy regardless of whether commands target an off-screen test surface or a
 * real desktop window. Otherwise the headless proof could silently diverge from the visible backend
 * exactly at the boundary it is meant to validate.
 *
 * Thread-affinity checks intentionally remain in the owning host. This helper is an implementation
 * detail and assumes every call is made on the thread on which its SDL renderer/font are valid.
 */
class Sdl3RenderContext final {
public:
    Sdl3RenderContext(SDL_Renderer& renderer, TTF_Font& font) noexcept;

    Sdl3RenderContext(const Sdl3RenderContext&) = delete;
    Sdl3RenderContext& operator=(const Sdl3RenderContext&) = delete;

    /** Clears the current render target using a semantic background color. */
    void clear(Color color = Color::default_color);

    void fillRect(const FillRectCommand& command);
    void strokeRect(const StrokeRectCommand& command);
    void drawText(const DrawTextCommand& command);

    [[nodiscard]] Size measureText(std::string_view utf8_text) const;
    [[nodiscard]] Coordinate lineHeight() const noexcept { return line_height_; }

    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view utf8_text,
        std::size_t scalar_index) const;

private:
    [[nodiscard]] Size measureSingleLine(std::string_view text) const;

    void renderLine(std::string_view line,
                    Point origin,
                    const TextStyle& style);

    SDL_Renderer* renderer_;
    TTF_Font* font_;
    Coordinate line_height_;
};

} // namespace sasd::ui::rendered::sdl3::detail
