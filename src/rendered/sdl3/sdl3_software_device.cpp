#include "rendered/sdl3/sdl3_software_device.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace sasd::ui::rendered::sdl3 {
namespace {

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
     * Coordinate is int32_t today, but making the conversion explicit prevents a future platform or
     * type change from turning implementation-defined narrowing into an adapter bug.
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

[[nodiscard]] SDL_FRect toFRect(Rect value) {
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
     * claiming pixel-identical theming. A future theme system can replace this table behind the same
     * semantic Color values.
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
        : renderer_{renderer}, active_{clip.has_value()} {
        if (!active_) {
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
         * RenderDevice owns the renderer, so command-local clipping always returns to "disabled".
         * Destructors must not throw; a restoration failure will be surfaced by the next SDL call.
         */
        (void)SDL_SetRenderClipRect(renderer_, nullptr);
    }

    ClipScope(const ClipScope&) = delete;
    ClipScope& operator=(const ClipScope&) = delete;

private:
    SDL_Renderer* renderer_;
    SDL_Rect rect_{};
    bool active_{false};
};

[[nodiscard]] std::size_t byteOffsetAtScalar(std::string_view text, std::size_t scalar_index) {
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

struct Sdl3SoftwareDevice::Impl {
    explicit Impl(Sdl3SoftwareDeviceConfig config)
        : size{config.surface_size},
          owner_thread{std::this_thread::get_id()} {
        if (size.width <= 0 || size.height <= 0) {
            throw std::invalid_argument{"SDL3 software surface size must be positive"};
        }
        if (config.font_path.empty()) {
            throw std::invalid_argument{"SDL3 software device requires a font file path"};
        }
        if (!(config.font_point_size > 0.0F) || !std::isfinite(config.font_point_size)) {
            throw std::invalid_argument{"SDL3 font point size must be finite and positive"};
        }

        /*
         * SDL surfaces/software renderers do not require a video window. This first adapter slice is
         * deliberately headless, which makes actual SDL command execution testable on CI without an
         * X11/Wayland/WindowServer/Win32 desktop session.
         */
        surface = SDL_CreateSurface(
            checkedPositiveInt(size.width, "surface width"),
            checkedPositiveInt(size.height, "surface height"),
            SDL_PIXELFORMAT_RGBA32);
        if (surface == nullptr) {
            throwSdlError("SDL_CreateSurface");
        }

        renderer = SDL_CreateSoftwareRenderer(surface);
        if (renderer == nullptr) {
            SDL_DestroySurface(surface);
            surface = nullptr;
            throwSdlError("SDL_CreateSoftwareRenderer");
        }

        if (!TTF_Init()) {
            SDL_DestroyRenderer(renderer);
            SDL_DestroySurface(surface);
            renderer = nullptr;
            surface = nullptr;
            throwSdlError("TTF_Init");
        }
        ttf_initialized = true;

        font = TTF_OpenFont(config.font_path.c_str(), config.font_point_size);
        if (font == nullptr) {
            TTF_Quit();
            ttf_initialized = false;
            SDL_DestroyRenderer(renderer);
            SDL_DestroySurface(surface);
            renderer = nullptr;
            surface = nullptr;
            throwSdlError("TTF_OpenFont");
        }
    }

    ~Impl() {
        if (font != nullptr) {
            TTF_CloseFont(font);
        }
        if (ttf_initialized) {
            TTF_Quit();
        }
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
        }
        if (surface != nullptr) {
            SDL_DestroySurface(surface);
        }
    }

    void requireOwnerThread() const {
        if (std::this_thread::get_id() != owner_thread) {
            throw std::runtime_error{
                "SDL3 software device must be used from the thread that created it"};
        }
    }

    [[nodiscard]] Size measureSingleLine(std::string_view text) const {
        if (text.empty()) {
            return {0, lineHeight()};
        }

        int width = 0;
        int height = 0;
        if (!TTF_GetStringSize(font, text.data(), text.size(), &width, &height)) {
            throwSdlError("TTF_GetStringSize");
        }
        return {
            checkedCoordinate(width, "text width"),
            checkedCoordinate(height, "text height"),
        };
    }

    [[nodiscard]] Coordinate lineHeight() const noexcept {
        const int value = TTF_GetFontHeight(font);
        if (value <= 0) {
            return 1;
        }

        const auto maximum = static_cast<std::int64_t>(
            std::numeric_limits<Coordinate>::max());
        return static_cast<std::int64_t>(value) > maximum
                   ? std::numeric_limits<Coordinate>::max()
                   : static_cast<Coordinate>(value);
    }

    void renderLine(std::string_view line,
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
             * TextStyle currently has no semantic background color. For the first rendered adapter,
             * inverse therefore means "paint the text run with its foreground color as background,
             * then draw glyphs using the adapter's default background". This keeps focus visible
             * without changing measurement or inventing a Core background-style model.
             */
            setDrawColor(renderer, foreground);
            SDL_FRect inverse_rect{
                static_cast<float>(origin.x),
                static_cast<float>(origin.y),
                static_cast<float>(measured.width),
                static_cast<float>(lineHeight()),
            };
            if (!SDL_RenderFillRect(renderer, &inverse_rect)) {
                throwSdlError("SDL_RenderFillRect(inverse text)");
            }
            text_color = paletteColor(Color::default_color, true);
        }

        SDL_Color sdl_color{text_color.r, text_color.g, text_color.b, text_color.a};
        SDL_Surface* text_surface =
            TTF_RenderText_Blended(font, line.data(), line.size(), sdl_color);
        if (text_surface == nullptr) {
            throwSdlError("TTF_RenderText_Blended");
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text_surface);
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

        const bool first_draw = SDL_RenderTexture(renderer, texture, nullptr, &destination);
        if (!first_draw) {
            SDL_DestroyTexture(texture);
            throwSdlError("SDL_RenderTexture");
        }

        if (style.bold) {
            /*
             * ADR 0025 makes bold presentation-only, so it must not invalidate measurement. Drawing
             * a second one-pixel-offset pass provides a deterministic initial visual emphasis while
             * keeping the font metrics unchanged. A later theme/font model can replace this policy.
             */
            destination.x += 1.0F;
            if (!SDL_RenderTexture(renderer, texture, nullptr, &destination)) {
                SDL_DestroyTexture(texture);
                throwSdlError("SDL_RenderTexture(bold overlay)");
            }
        }

        SDL_DestroyTexture(texture);

        if (style.underline && measured.width > 0) {
            setDrawColor(renderer, text_color);
            SDL_FRect underline{
                static_cast<float>(origin.x),
                static_cast<float>(
                    static_cast<std::int64_t>(origin.y) +
                    static_cast<std::int64_t>(lineHeight()) - 1),
                static_cast<float>(measured.width),
                1.0F,
            };
            if (!SDL_RenderFillRect(renderer, &underline)) {
                throwSdlError("SDL_RenderFillRect(underline)");
            }
        }

    }

    Size size{};
    SDL_Surface* surface{nullptr};
    SDL_Renderer* renderer{nullptr};
    TTF_Font* font{nullptr};
    bool ttf_initialized{false};
    std::thread::id owner_thread;
    std::uint64_t revision{1};
};

Sdl3SoftwareDevice::Sdl3SoftwareDevice(Sdl3SoftwareDeviceConfig config)
    : impl_{std::make_unique<Impl>(std::move(config))} {}

Sdl3SoftwareDevice::~Sdl3SoftwareDevice() = default;

void Sdl3SoftwareDevice::clear(Color color) {
    impl_->requireOwnerThread();
    setDrawColor(impl_->renderer, paletteColor(color, true));

    if (!SDL_SetRenderClipRect(impl_->renderer, nullptr)) {
        throwSdlError("SDL_SetRenderClipRect(clear)");
    }
    if (!SDL_RenderClear(impl_->renderer)) {
        throwSdlError("SDL_RenderClear");
    }
}

Size Sdl3SoftwareDevice::surfaceSize() const noexcept {
    return impl_->size;
}

Rgba8 Sdl3SoftwareDevice::pixelAt(Point point) const {
    impl_->requireOwnerThread();

    if (!impl_->size.isEmpty() &&
        (point.x < 0 || point.y < 0 ||
         point.x >= impl_->size.width || point.y >= impl_->size.height)) {
        throw std::out_of_range{"SDL3 software pixel lies outside the surface"};
    }

    /*
     * SDL3 renderers batch commands. Flush before reading the underlying surface so diagnostics see
     * all work already replayed by DisplayListExecutor.
     */
    if (!SDL_FlushRenderer(impl_->renderer)) {
        throwSdlError("SDL_FlushRenderer");
    }

    Rgba8 result;
    if (!SDL_ReadSurfacePixel(
            impl_->surface,
            static_cast<int>(point.x),
            static_cast<int>(point.y),
            &result.r,
            &result.g,
            &result.b,
            &result.a)) {
        throwSdlError("SDL_ReadSurfacePixel");
    }
    return result;
}

void Sdl3SoftwareDevice::fillRect(const FillRectCommand& command) {
    impl_->requireOwnerThread();

    const bool default_is_background = command.role == FillRole::background;
    setDrawColor(impl_->renderer, paletteColor(command.color, default_is_background));

    const SDL_FRect rect = toFRect(command.bounds);
    if (!SDL_RenderFillRect(impl_->renderer, &rect)) {
        throwSdlError("SDL_RenderFillRect");
    }
}

void Sdl3SoftwareDevice::strokeRect(const StrokeRectCommand& command) {
    impl_->requireOwnerThread();
    setDrawColor(impl_->renderer, paletteColor(command.color, false));

    const Coordinate smallest_extent = std::min(command.bounds.width, command.bounds.height);

    /*
     * Widen before adding one. Coordinate is signed int32_t; (INT32_MAX + 1) would otherwise be
     * undefined behavior for a perfectly valid, if extreme, rectangle. This is deliberately boring
     * defensive arithmetic at the platform boundary rather than an optimization.
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
        if (!SDL_RenderRect(impl_->renderer, &rect)) {
            throwSdlError("SDL_RenderRect");
        }
    }
}

void Sdl3SoftwareDevice::drawText(const DrawTextCommand& command) {
    impl_->requireOwnerThread();
    ClipScope clip{impl_->renderer, command.clip_bounds};

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
                static_cast<std::int64_t>(impl_->lineHeight());

        if (y64 < std::numeric_limits<Coordinate>::min() ||
            y64 > std::numeric_limits<Coordinate>::max()) {
            throw std::overflow_error{"rendered text line position exceeds Coordinate range"};
        }

        impl_->renderLine(
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

Size Sdl3SoftwareDevice::measureText(std::string_view utf8_text) const {
    impl_->requireOwnerThread();

    Coordinate maximum_width = 0;
    Coordinate total_height = 0;
    std::size_t line_start = 0;
    bool saw_line = false;

    while (line_start <= utf8_text.size()) {
        const std::size_t newline = utf8_text.find('\n', line_start);
        const std::size_t line_end =
            newline == std::string_view::npos ? utf8_text.size() : newline;

        const Size line = impl_->measureSingleLine(
            utf8_text.substr(line_start, line_end - line_start));
        maximum_width = std::max(maximum_width, line.width);

        const std::int64_t accumulated =
            static_cast<std::int64_t>(total_height) +
            static_cast<std::int64_t>(impl_->lineHeight());
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
        saw_line ? total_height : impl_->lineHeight(),
    };
}

Coordinate Sdl3SoftwareDevice::lineHeight() const noexcept {
    return impl_->lineHeight();
}

std::optional<Coordinate> Sdl3SoftwareDevice::textAdvanceToScalar(
    std::string_view utf8_text,
    std::size_t scalar_index) const {
    impl_->requireOwnerThread();

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
        impl_->font,
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
     * SDL_ttf returns the shaped cluster that surrounds the byte offset. If that cluster starts
     * before our requested Unicode-scalar boundary, the boundary lies inside a combining/ligature
     * cluster and cannot be represented faithfully by the initial monotonic scalar-caret contract.
     */
    if (substring.offset != static_cast<int>(byte_offset) || substring.rect.x < 0) {
        return std::nullopt;
    }

    return checkedCoordinate(substring.rect.x, "text advance");
}

std::uint64_t Sdl3SoftwareDevice::revision() const noexcept {
    return impl_->revision;
}

} // namespace sasd::ui::rendered::sdl3
