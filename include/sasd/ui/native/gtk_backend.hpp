#pragma once

#include <sasd/ui/backend/backend.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/render_device.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace sasd::ui::native::gtk {

struct GtkWindowBackendConfig {
    std::string title{"SASD UI Toolkit"};
    Size initial_size{800, 500};
    bool hidden{false};
    bool resizable{true};
    double font_point_size{12.0};
    Color background_color{Color::black};
};

/**
 * GTK4 top-level host for the backend-neutral Core and Rendered pipelines.
 *
 * GTK is deliberately an implementation detail of this optional adapter. The host owns the GTK
 * window and drawing area through RAII, pumps GLib from pollEvent(), and keeps one complete copied
 * DisplayList for expose/resize replay. All calls must be made on the thread that initialized it.
 * Committed text uses the initial GTK key-commit fallback; IME preedit is intentionally deferred.
 */
class GtkWindowBackend final : public Backend,
                               public rendered::RenderDevice,
                               public rendered::RenderedMeasurementContext {
public:
    explicit GtkWindowBackend(GtkWindowBackendConfig config = {});
    ~GtkWindowBackend() override;

    GtkWindowBackend(const GtkWindowBackend&) = delete;
    GtkWindowBackend& operator=(const GtkWindowBackend&) = delete;
    GtkWindowBackend(GtkWindowBackend&&) = delete;
    GtkWindowBackend& operator=(GtkWindowBackend&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override { return "native-gtk"; }
    [[nodiscard]] BackendCapabilities capabilities() const noexcept override;

    void initialize() override;
    void shutdown() noexcept override;
    [[nodiscard]] std::optional<Event> pollEvent() override;

    [[nodiscard]] bool isInitialized() const noexcept;
    [[nodiscard]] Size windowSize() const;
    [[nodiscard]] bool presentationRequested() const noexcept;
    [[nodiscard]] std::size_t presentFrame(const rendered::DisplayList& list);

    void fillRect(const rendered::FillRectCommand& command) override;
    void strokeRect(const rendered::StrokeRectCommand& command) override;
    void drawText(const rendered::DrawTextCommand& command) override;

    [[nodiscard]] Size measureText(std::string_view utf8_text) const override;
    [[nodiscard]] Coordinate lineHeight() const noexcept override;
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view utf8_text, std::size_t scalar_index) const override;
    [[nodiscard]] std::uint64_t revision() const noexcept override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sasd::ui::native::gtk
