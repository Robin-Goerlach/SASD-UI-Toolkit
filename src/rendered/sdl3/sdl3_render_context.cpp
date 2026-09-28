#include "rendered/sdl3/sdl3_render_context.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace sasd::ui::rendered::sdl3::detail {
namespace {

struct Rgba8 {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{255};
};

[[noreturn]] void throwSdlError(const char* operation) {
    throw std::runtime_error{
        std::string{operation} + " failed: " +
        (SDL_GetError() != nullptr ? SDL_GetError() : "unknown SDL error")};
}

[[nodiscard]] int checkedPositiveInt(Coordinate value, const char* what) {
    if (value < 0 ||
        static_cast<std::int64_t>(value) >
            static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument{std::string{what} + " is outside SDL integer range"};
    }
    return static_cast<int>(value);
}

[[nodiscard]] int checkedSignedInt(Coordinate value, const char* what) {
    /*
     * Logical origins and clip rectangles may legitimately be negative while a widget is partially
     * outside the visible surface. Keep that distinct from extents, which must remain non-negative.
     */
    const auto widened = static_cast<std::int64_t>(value);
    if (widened < static_cast<std::int64_t>(std::numeric_limits<int>::min()) ||
        widened > static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument{std::string{what} + " is outside SDL integer range"};
    }
    return static_cast<int>(value);
}

[[nodiscard]] Coordinate checkedCoordinate(int value, const char* what) {
    if (value < 0 ||
        static_cast<std::int64_t>(value) >
            static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max())) {
        throw std::runtime_error{std::string{what} + " is outside SASD Coordinate range"};
    }
    return static_cast<Coordinate>(value);
}

[[nodiscard]] SDL_FRect toFRect(Rect value) noexcept {
    return {
        static_cast<float>(value.x),
        static_cast<float>(value.y),
        static_cast<float>(value.width),
        static_cast<float>(value.height),
    };
}

[[nodiscard]] SDL_Rect toClipRect(Rect value) {
    return {
        checkedSignedInt(value.x, "clip x"),
        checkedSignedInt(value.y, "clip y"),
        checkedPositiveInt(value.width, "clip width"),
        checkedPositiveInt(value.height, "clip height"),
    };
}

[[nodiscard]] Rgba8 paletteColor(Color color, bool default_is_background) noexcept {
    /*
     * The exact RGB values are adapter policy, not Core semantics. The initial palette intentionally
     * resembles common ANSI/xterm colors so terminal and rendered previews feel related without
     * claiming pixel-identical theming.
     */
    switch (color) {
    case Color::default_color:
        return default_is_background ? Rgba8{0, 0, 0, 255} : Rgba8{224, 224, 224, 255};
    case Color::black:          return {0, 0, 0, 255};
    case Color::red:            return {170, 0, 0, 255};
    case Color::green:          return {0, 170, 0, 255};
    case Color::yellow:         return {170, 85, 0, 255};
    case Color::blue:           return {0, 0, 170, 255};
    case Color::magenta:        return {170, 0, 170, 255};
    case Color::cyan:           return {0, 170, 170, 255};
    case Color::white:          return {170, 170, 170, 255};
    case Color::bright_black:   return {85, 85, 85, 255};
    case Color::bright_red:     return {255, 85, 85, 255};
    case Color::bright_green:   return {85, 255, 85, 255};
    case Color::bright_yellow:  return {255, 255, 85, 255};
    case Color::bright_blue:    return {85, 85, 255, 255};
    case Color::bright_magenta: return {255, 85, 255, 255};
    case Color::bright_cyan:    return {85, 255, 255, 255};
    case Color::bright_white:   return {255, 255, 255, 255};
    }

    return default_is_background ? Rgba8{0, 0, 0, 255} : Rgba8{224, 224, 224, 255};
}

[[nodiscard]] Rgba8 dimmed(Rgba8 color) noexcept {
    color.r = static_cast<std::uint8_t>(color.r / 2);
    color.g = static_cast<std::uint8_t>(color.g / 2);
    color.b = static_cast<std::uint8_t>(color.b / 2);
    return color;
}

void setDrawColor(SDL_Renderer* renderer, Rgba8 color) {
    if (!SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a)) {
        throwSdlError("SDL_SetRenderDrawColor");
    }
}

class ClipScope final {
public:
    ClipScope(SDL_Renderer* renderer, const std::optional<Rect>& clip)
        : renderer_{renderer} {
        if (!clip.has_value()) {
            if (!SDL_SetRenderClipRect(renderer_, nullptr)) {
                throwSdlError("SDL_SetRenderClipRect(disable)");
            }
            return;
        }

        rect_ = toClipRect(*clip);
        if (!SDL_SetRenderClipRect(renderer_, &rect_)) {
            throwSdlError("SDL_SetRenderClipRect");
        }
    }

    ~ClipScope() {
        /*
         * DrawTextCommand clipping is command-local. Always return the renderer to unclipped state so
         * a later rectangle/text command cannot accidentally inherit this text command's clip.
         * Destructors are non-throwing; a restoration failure will be exposed by a later SDL call.
         */
        (void)SDL_SetRenderClipRect(renderer_, nullptr);
    }

    ClipScope(const ClipScope&) = delete;
    ClipScope& operator=(const ClipScope&) = delete;

private:
    SDL_Renderer* renderer_;
    SDL_Rect rect_{};
};

[[nodiscard]] std::size_t byteOffsetAtScalar(std::string_view text,
                                             std::size_t scalar_index) {
    std::size_t byte_offset = 0;
    std::size_t scalar = 0;

    while (byte_offset < text.size() && scalar < scalar_index) {
        const utf8::DecodedScalar decoded = utf8::decodeOne(text, byte_offset);
        if (decoded.consumed == 0) {
            break;
        }
        byte_offset += decoded.consumed;
        ++scalar;
    }

    return byte_offset;
}

} // namespace

Sdl3RenderContext::Sdl3RenderContext(SDL_Renderer& renderer, TTF_Font& font) noexcept
    : renderer_{&renderer},
      font_{&font},
      line_height_{std::max(1, TTF_GetFontHeight(&font))} {}

void Sdl3RenderContext::clear(Color color) {
    setDrawColor(renderer_, paletteColor(color, true));

    if (!SDL_SetRenderClipRect(renderer_, nullptr)) {
        throwSdlError("SDL_SetRenderClipRect(clear)");
    }
    if (!SDL_RenderClear(renderer_)) {
        throwSdlError("SDL_RenderClear");
    }
}

void Sdl3RenderContext::fillRect(const FillRectCommand& command) {
    const bool default_is_background = command.role == FillRole::background;
    setDrawColor(renderer_, paletteColor(command.color, default_is_background));

    const SDL_FRect rect = toFRect(command.bounds);
    if (!SDL_RenderFillRect(renderer_, &rect)) {
        throwSdlError("SDL_RenderFillRect");
    }
}

void Sdl3RenderContext::strokeRect(const StrokeRectCommand& command) {
    setDrawColor(renderer_, paletteColor(command.color, false));

    const Coordinate smallest_extent = std::min(command.bounds.width, command.bounds.height);

    /*
     * Widen before adding one. Coordinate is signed int32_t; (INT32_MAX + 1) would otherwise be
     * undefined behavior for a perfectly valid, if extreme, rectangle.
     */
    const Coordinate maximum_layers =
        smallest_extent <= 0
            ? 0
            : static_cast<Coordinate>(
                  (static_cast<std::int64_t>(smallest_extent) + 1) / 2);
    const Coordinate layers = std::min(command.thickness, maximum_layers);

    for (Coordinate inset = 0; inset < layers; ++inset) {
        SDL_FRect rect{
            static_cast<float>(static_cast<std::int64_t>(command.bounds.x) + inset),
            static_cast<float>(static_cast<std::int64_t>(command.bounds.y) + inset),
            static_cast<float>(
                static_cast<std::int64_t>(command.bounds.width) - (2LL * inset)),
            static_cast<float>(
                static_cast<std::int64_t>(command.bounds.height) - (2LL * inset)),
        };
        if (!SDL_RenderRect(renderer_, &rect)) {
            throwSdlError("SDL_RenderRect");
        }
    }
}

void Sdl3RenderContext::drawText(const DrawTextCommand& command) {
    ClipScope clip{renderer_, command.clip_bounds};

    /*
     * MeasurementContext promises explicit line-break handling. SDL_ttf's one-line rendering helper
     * does not turn '\n' into layout, so split here and keep every line on the same font metric grid.
     */
    std::size_t line_start = 0;
    std::size_t line_index = 0;

    while (line_start <= command.text.size()) {
        const std::size_t newline = command.text.find('\n', line_start);
        const std::size_t line_end =
            newline == std::string::npos ? command.text.size() : newline;

        const std::string_view line{
            command.text.data() + line_start,
            line_end - line_start};

        const auto y64 =
            static_cast<std::int64_t>(command.origin.y) +
            static_cast<std::int64_t>(line_index) *
                static_cast<std::int64_t>(line_height_);

        if (y64 < std::numeric_limits<Coordinate>::min() ||
            y64 > std::numeric_limits<Coordinate>::max()) {
            throw std::overflow_error{"rendered text line position exceeds Coordinate range"};
        }

        renderLine(
            line,
            {command.origin.x, static_cast<Coordinate>(y64)},
            command.style);

        if (newline == std::string::npos) {
            break;
        }
        line_start = newline + 1;
        ++line_index;
    }
}

Size Sdl3RenderContext::measureText(std::string_view utf8_text) const {
    Coordinate maximum_width = 0;
    Coordinate total_height = 0;
    std::size_t line_start = 0;
    bool saw_line = false;

    while (line_start <= utf8_text.size()) {
        const std::size_t newline = utf8_text.find('\n', line_start);
        const std::size_t line_end =
            newline == std::string_view::npos ? utf8_text.size() : newline;

        const Size line = measureSingleLine(
            utf8_text.substr(line_start, line_end - line_start));
        maximum_width = std::max(maximum_width, line.width);

        const std::int64_t accumulated =
            static_cast<std::int64_t>(total_height) +
            static_cast<std::int64_t>(line_height_);
        total_height = accumulated > std::numeric_limits<Coordinate>::max()
                           ? std::numeric_limits<Coordinate>::max()
                           : static_cast<Coordinate>(accumulated);
        saw_line = true;

        if (newline == std::string_view::npos) {
            break;
        }
        line_start = newline + 1;
    }

    return {
        maximum_width,
        saw_line ? total_height : line_height_,
    };
}

std::optional<Coordinate> Sdl3RenderContext::textAdvanceToScalar(
    std::string_view utf8_text,
    std::size_t scalar_index) const {
    /*
     * TextField is single-line. Returning nullopt for a line break keeps the first rendered caret
     * contract honest instead of pretending multi-line visual ordering fits a one-dimensional
     * advance.
     */
    if (utf8_text.find('\n') != std::string_view::npos) {
        return std::nullopt;
    }

    const std::size_t scalar_count = utf8::scalarCount(utf8_text);
    if (scalar_index > scalar_count) {
        return std::nullopt;
    }
    if (scalar_index == 0) {
        return Coordinate{0};
    }

    TTF_Text* text = TTF_CreateText(
        nullptr,
        font_,
        utf8_text.data(),
        utf8_text.size());
    if (text == nullptr) {
        throwSdlError("TTF_CreateText");
    }

    if (scalar_index == scalar_count) {
        int width = 0;
        int height = 0;
        const bool ok = TTF_GetTextSize(text, &width, &height);
        TTF_DestroyText(text);
        if (!ok) {
            throwSdlError("TTF_GetTextSize");
        }
        (void)height;
        return checkedCoordinate(width, "text advance");
    }

    const std::size_t byte_offset = byteOffsetAtScalar(utf8_text, scalar_index);
    if (byte_offset > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        TTF_DestroyText(text);
        return std::nullopt;
    }

    TTF_SubString substring{};
    const bool ok = TTF_GetTextSubString(
        text,
        static_cast<int>(byte_offset),
        &substring);
    TTF_DestroyText(text);

    if (!ok) {
        throwSdlError("TTF_GetTextSubString");
    }

    /*
     * SDL_ttf returns the shaped cluster surrounding the byte offset. If that cluster starts before
     * the requested Unicode-scalar boundary, the boundary lies inside a combining/ligature cluster
     * and cannot be represented faithfully by the current monotonic scalar-caret contract.
     */
    if (substring.offset != static_cast<int>(byte_offset) || substring.rect.x < 0) {
        return std::nullopt;
    }

    return checkedCoordinate(substring.rect.x, "text advance");
}

Size Sdl3RenderContext::measureSingleLine(std::string_view text) const {
    if (text.empty()) {
        return {0, line_height_};
    }

    int width = 0;
    int height = 0;
    if (!TTF_GetStringSize(font_, text.data(), text.size(), &width, &height)) {
        throwSdlError("TTF_GetStringSize");
    }

    return {
        checkedCoordinate(width, "text width"),
        checkedCoordinate(height, "text height"),
    };
}

void Sdl3RenderContext::renderLine(std::string_view line,
                                   Point origin,
                                   const TextStyle& style) {
    if (line.empty()) {
        return;
    }

    const Size measured = measureSingleLine(line);

    Rgba8 foreground = paletteColor(style.foreground, false);
    if (style.dim) {
        foreground = dimmed(foreground);
    }

    Rgba8 text_color = foreground;
    if (style.inverse) {
        /*
         * TextStyle currently has no semantic background color. The first rendered adapter therefore
         * treats inverse as foreground-colored run background plus adapter-default-background glyph
         * ink. It stays presentation-only and does not change font metrics.
         */
        setDrawColor(renderer_, foreground);
        SDL_FRect inverse_rect{
            static_cast<float>(origin.x),
            static_cast<float>(origin.y),
            static_cast<float>(measured.width),
            static_cast<float>(line_height_),
        };
        if (!SDL_RenderFillRect(renderer_, &inverse_rect)) {
            throwSdlError("SDL_RenderFillRect(inverse text)");
        }
        text_color = paletteColor(Color::default_color, true);
    }

    SDL_Color sdl_color{text_color.r, text_color.g, text_color.b, text_color.a};
    SDL_Surface* text_surface =
        TTF_RenderText_Blended(font_, line.data(), line.size(), sdl_color);
    if (text_surface == nullptr) {
        throwSdlError("TTF_RenderText_Blended");
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, text_surface);
    SDL_DestroySurface(text_surface);
    if (texture == nullptr) {
        throwSdlError("SDL_CreateTextureFromSurface");
    }

    SDL_FRect destination{
        static_cast<float>(origin.x),
        static_cast<float>(origin.y),
        static_cast<float>(measured.width),
        static_cast<float>(measured.height),
    };

    if (!SDL_RenderTexture(renderer_, texture, nullptr, &destination)) {
        SDL_DestroyTexture(texture);
        throwSdlError("SDL_RenderTexture");
    }

    if (style.bold) {
        /*
         * ADR 0025 makes bold presentation-only, so it must not invalidate measurement. Drawing a
         * second one-unit-offset pass provides deterministic emphasis while leaving font metrics
         * unchanged. A later font/theme model can replace this policy.
         */
        destination.x += 1.0F;
        if (!SDL_RenderTexture(renderer_, texture, nullptr, &destination)) {
            SDL_DestroyTexture(texture);
            throwSdlError("SDL_RenderTexture(bold overlay)");
        }
    }

    SDL_DestroyTexture(texture);

    if (style.underline && measured.width > 0) {
        setDrawColor(renderer_, text_color);
        SDL_FRect underline{
            static_cast<float>(origin.x),
            static_cast<float>(
                static_cast<std::int64_t>(origin.y) +
                static_cast<std::int64_t>(line_height_) - 1),
            static_cast<float>(measured.width),
            1.0F,
        };
        if (!SDL_RenderFillRect(renderer_, &underline)) {
            throwSdlError("SDL_RenderFillRect(underline)");
        }
    }
}

} // namespace sasd::ui::rendered::sdl3::detail
