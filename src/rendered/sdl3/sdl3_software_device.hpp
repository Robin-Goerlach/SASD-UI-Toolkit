#pragma once

#include <sasd/ui/rendered/render_device.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace sasd::ui::rendered::sdl3 {

/**
 * Small RGBA value used only for deterministic inspection of the software-rendered surface.
 *
 * This is intentionally an adapter-local diagnostic value, not a replacement for the toolkit's
 * semantic Color type and not the beginning of a general bitmap API. It lets the first concrete SDL3
 * adapter prove drawing behavior headlessly before a real desktop window is introduced.
 */
struct Rgba8 {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{255};

    friend constexpr bool operator==(const Rgba8&, const Rgba8&) = default;
};

/**
 * Configuration for the first concrete SDL3 rendered-device slice.
 *
 * The surface is deliberately off-screen. This isolates and validates command execution, clipping,
 * UTF-8 font rendering and text metrics without introducing window/event-loop lifetime at the same
 * time. The same RenderDevice contract can then be reused by a later window-backed SDL3 adapter.
 */
struct Sdl3SoftwareDeviceConfig {
    Size surface_size{640, 480};
    std::string font_path;
    float font_point_size{16.0F};
};

/**
 * Headless SDL3 + SDL_ttf implementation of both RenderDevice and RenderedMeasurementContext.
 *
 * No SDL type appears in this header. Native handles/resources live in the private implementation.
 * That is the same dependency direction intended for the later desktop-window adapter: semantic
 * Widgets and generic Rendered code never acquire SDL ownership.
 *
 * The object is intentionally non-copyable/non-movable because SDL renderer/font operations are
 * thread-affine. SDL's rendering API is a main-thread API, while SDL_ttf text objects must stay on
 * the thread that created their font/text engine. Construction therefore requires the process main
 * thread, and subsequent calls require that same owning thread.
 */
class Sdl3SoftwareDevice final : public RenderDevice, public RenderedMeasurementContext {
public:
    explicit Sdl3SoftwareDevice(Sdl3SoftwareDeviceConfig config);
    ~Sdl3SoftwareDevice() override;

    Sdl3SoftwareDevice(const Sdl3SoftwareDevice&) = delete;
    Sdl3SoftwareDevice& operator=(const Sdl3SoftwareDevice&) = delete;
    Sdl3SoftwareDevice(Sdl3SoftwareDevice&&) = delete;
    Sdl3SoftwareDevice& operator=(Sdl3SoftwareDevice&&) = delete;

    /** Clears the complete off-screen surface. default_color resolves as the adapter background. */
    void clear(Color color = Color::default_color);

    /** Returns the fixed logical/pixel size of the software surface. */
    [[nodiscard]] Size surfaceSize() const noexcept;

    /**
     * Reads one rendered pixel for deterministic tests/diagnostics.
     *
     * This method is intentionally correctness-oriented, not a high-throughput image API.
     */
    [[nodiscard]] Rgba8 pixelAt(Point point) const;

    // RenderDevice
    void fillRect(const FillRectCommand& command) override;
    void strokeRect(const StrokeRectCommand& command) override;
    void drawText(const DrawTextCommand& command) override;

    // RenderedMeasurementContext
    [[nodiscard]] Size measureText(std::string_view utf8_text) const override;
    [[nodiscard]] Coordinate lineHeight() const noexcept override;
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view utf8_text,
        std::size_t scalar_index) const override;
    [[nodiscard]] std::uint64_t revision() const noexcept override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sasd::ui::rendered::sdl3
