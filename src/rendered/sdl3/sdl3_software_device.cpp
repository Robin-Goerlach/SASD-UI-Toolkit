#include "rendered/sdl3/sdl3_software_device.hpp"

#include "rendered/sdl3/sdl3_render_context.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cmath>
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

} // namespace

struct Sdl3SoftwareDevice::Impl {
    explicit Impl(Sdl3SoftwareDeviceConfig config)
        : size{config.surface_size},
          owner_thread{std::this_thread::get_id()} {
        /*
         * SDL3 documents renderer drawing operations as main-thread-only even when the software
         * renderer itself can be created elsewhere. Rejecting an off-main-thread device up front is
         * safer than accepting construction and failing later on the first FillRect/DrawText call.
         */
        if (!SDL_IsMainThread()) {
            throw std::runtime_error{
                "SDL3 software device must be created and used on the process main thread"};
        }

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
         * SDL surfaces/software renderers do not require a video window. This adapter intentionally
         * stays headless so command execution can be tested without a desktop display server.
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

        /*
         * Construct the shared policy object only after both non-owning dependencies exist. This is
         * the exact command/metric implementation reused by the window-backed adapter, preventing
         * the headless proof and visible backend from drifting apart.
         */
        render_context =
            std::make_unique<detail::Sdl3RenderContext>(*renderer, *font);
    }

    ~Impl() {
        render_context.reset();

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

    Size size{};
    SDL_Surface* surface{nullptr};
    SDL_Renderer* renderer{nullptr};
    TTF_Font* font{nullptr};
    bool ttf_initialized{false};
    std::unique_ptr<detail::Sdl3RenderContext> render_context;
    std::thread::id owner_thread;
    std::uint64_t revision{1};
};

Sdl3SoftwareDevice::Sdl3SoftwareDevice(Sdl3SoftwareDeviceConfig config)
    : impl_{std::make_unique<Impl>(std::move(config))} {}

Sdl3SoftwareDevice::~Sdl3SoftwareDevice() = default;

void Sdl3SoftwareDevice::clear(Color color) {
    impl_->requireOwnerThread();
    impl_->render_context->clear(color);
}

Size Sdl3SoftwareDevice::surfaceSize() const noexcept {
    return impl_->size;
}

Rgba8 Sdl3SoftwareDevice::pixelAt(Point point) const {
    impl_->requireOwnerThread();

    if (point.x < 0 || point.y < 0 ||
        point.x >= impl_->size.width || point.y >= impl_->size.height) {
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
    impl_->render_context->fillRect(command);
}

void Sdl3SoftwareDevice::strokeRect(const StrokeRectCommand& command) {
    impl_->requireOwnerThread();
    impl_->render_context->strokeRect(command);
}

void Sdl3SoftwareDevice::drawText(const DrawTextCommand& command) {
    impl_->requireOwnerThread();
    impl_->render_context->drawText(command);
}

Size Sdl3SoftwareDevice::measureText(std::string_view utf8_text) const {
    impl_->requireOwnerThread();
    return impl_->render_context->measureText(utf8_text);
}

Coordinate Sdl3SoftwareDevice::lineHeight() const noexcept {
    return impl_->render_context->lineHeight();
}

std::optional<Coordinate> Sdl3SoftwareDevice::textAdvanceToScalar(
    std::string_view utf8_text,
    std::size_t scalar_index) const {
    impl_->requireOwnerThread();
    return impl_->render_context->textAdvanceToScalar(utf8_text, scalar_index);
}

std::uint64_t Sdl3SoftwareDevice::revision() const noexcept {
    return impl_->revision;
}

} // namespace sasd::ui::rendered::sdl3
