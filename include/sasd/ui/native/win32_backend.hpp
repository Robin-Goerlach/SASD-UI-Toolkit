#pragma once

#include <sasd/ui/backend/backend.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/render_device.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <memory>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sasd::ui::native::win32 {

/** Configuration for the first Win32 native host and GDI Rendered consumer. */
struct Win32WindowBackendConfig {
    std::string title{"SASD UI Toolkit"};
    Size initial_size{800, 500};
    bool hidden{false};
    bool resizable{true};
    /** GDI logical font size in points; layout remains expressed in 96-DPI logical units. */
    float font_point_size{12.0F};
    /** Full-frame clear color used before every DisplayList replay. */
    Color background_color{Color::black};
};

/**
 * Native Win32 top-level host for the backend-neutral Application/Event and Rendered pipelines.
 *
 * The host owns one HWND through RAII and translates its message queue into existing Core Events.
 * It intentionally does not create native Button/TextField peers yet: the semantic Widget tree and
 * its existing EventDispatcher/FocusManager/PointerRouter contracts remain the single source of
 * interaction semantics. IME composition, native child controls and accessibility are deferred.
 */
class Win32WindowBackend final : public Backend,
                                public rendered::RenderDevice,
                                public rendered::RenderedMeasurementContext {
public:
    explicit Win32WindowBackend(Win32WindowBackendConfig config = {});
    ~Win32WindowBackend() override;

    Win32WindowBackend(const Win32WindowBackend&) = delete;
    Win32WindowBackend& operator=(const Win32WindowBackend&) = delete;
    Win32WindowBackend(Win32WindowBackend&&) = delete;
    Win32WindowBackend& operator=(Win32WindowBackend&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override { return "native-win32"; }
    [[nodiscard]] BackendCapabilities capabilities() const noexcept override;

    void initialize() override;
    void shutdown() noexcept override;
    [[nodiscard]] std::optional<Event> pollEvent() override;

    [[nodiscard]] bool isInitialized() const noexcept;
    [[nodiscard]] Size windowSize() const;

    /** True when the window surface needs a complete replay of the last frame. */
    [[nodiscard]] bool presentationRequested() const noexcept;

    /** Replays a complete logical frame into the real Win32 client surface. */
    [[nodiscard]] std::size_t presentFrame(const rendered::DisplayList& list);

    // RenderDevice
    void fillRect(const rendered::FillRectCommand& command) override;
    void strokeRect(const rendered::StrokeRectCommand& command) override;
    void drawText(const rendered::DrawTextCommand& command) override;

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

} // namespace sasd::ui::native::win32
