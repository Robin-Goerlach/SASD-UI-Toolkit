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

namespace sasd::ui::rendered::sdl3 {

/**
 * Construction policy for the first window-backed SDL3 M3 host.
 *
 * Font data is supplied explicitly instead of hidden behind a platform-specific font search. That
 * keeps resource discovery/licensing out of the semantic UI architecture while the adapter is still
 * experimental and build-tree-only.
 */
struct Sdl3WindowBackendConfig {
    std::string title{"SASD UI Toolkit"};
    Size initial_size{800, 500};
    std::string font_path;
    float font_point_size{16.0F};

    /** Lets automated tests create a real SDL_Window without mapping it on a desktop. */
    bool hidden{false};

    /** User resize is useful for M3 layout validation and enabled by default. */
    bool resizable{true};

    /**
     * Requests a high-density back buffer when the platform supports it.
     *
     * Toolkit geometry still stays in logical window coordinates. The backend configures SDL logical
     * presentation so the same Widget bounds remain stable when the physical pixel density changes.
     */
    bool high_pixel_density{true};

    /** Full frames are cleared to this semantic background before DisplayList replay. */
    Color background_color{Color::black};
};

/**
 * First real desktop-window Backend for the Rendered M3 path.
 *
 * One object deliberately spans three narrow adapter interfaces:
 *
 * - Backend: lifecycle and non-blocking translation of SDL events into semantic toolkit Events;
 * - RenderDevice: execution of the existing backend-neutral DisplayList;
 * - RenderedMeasurementContext: real SDL_ttf metrics used by layout/TextField caret placement.
 *
 * SDL types remain entirely behind this build-tree-only adapter header. Core Widgets, DisplayList,
 * PresentationCoordinator and normal application code continue to know nothing about SDL handles.
 *
 * The class is non-copyable/non-movable because SDL window/renderer/font resources and event polling
 * are main-thread-affine. Application should own the lifecycle indirectly through Backend exactly as
 * it already does for TerminalBackend.
 */
class Sdl3WindowBackend final : public Backend,
                                public RenderDevice,
                                public RenderedMeasurementContext {
public:
    explicit Sdl3WindowBackend(Sdl3WindowBackendConfig config);
    ~Sdl3WindowBackend() override;

    Sdl3WindowBackend(const Sdl3WindowBackend&) = delete;
    Sdl3WindowBackend& operator=(const Sdl3WindowBackend&) = delete;
    Sdl3WindowBackend(Sdl3WindowBackend&&) = delete;
    Sdl3WindowBackend& operator=(Sdl3WindowBackend&&) = delete;

    // Backend
    [[nodiscard]] std::string_view name() const noexcept override {
        return "rendered-sdl3-window";
    }

    [[nodiscard]] BackendCapabilities capabilities() const noexcept override {
        /*
         * Capabilities describe semantic facilities exposed by this SASD adapter, not everything SDL
         * can do natively. Pointer input becomes true now that Mouse motion/button transitions are
         * normalized to PointerEvent. IME remains false until composition semantics exist in Core.
         */
        BackendCapabilities result;
        result.pointer_input = true;
        return result;
    }

    void initialize() override;
    void shutdown() noexcept override;
    [[nodiscard]] std::optional<Event> pollEvent() override;

    [[nodiscard]] bool isInitialized() const noexcept;

    /** Current logical window size used by Widget layout and ResizeEvent. */
    [[nodiscard]] Size windowSize() const;

    /** Current physical back-buffer size, useful for DPI diagnostics/tests. */
    [[nodiscard]] Size pixelSize() const;

    /** SDL's current logical-to-display scale for this window. */
    [[nodiscard]] float displayScale() const;

    /**
     * True when expose/resize/scale/lifecycle state requires a fresh full-frame presentation.
     *
     * The first window host deliberately uses full-frame replay rather than assuming SDL's window
     * back buffer preserves prior incremental drawing across SDL_RenderPresent().
     */
    [[nodiscard]] bool presentationRequested() const noexcept;

    /**
     * Enables/disables SDL Unicode text input for the window.
     *
     * Desktop text input is not left permanently enabled because SDL may activate an IME or on-screen
     * keyboard. The application can mirror logical FocusManager state: enable while a TextField owns
     * focus and disable for ordinary buttons/labels.
     */
    void setTextInputEnabled(bool enabled);
    [[nodiscard]] bool textInputEnabled() const noexcept;

    /**
     * Presents one complete logical frame.
     *
     * The target is cleared first, then list is replayed in order and finally SDL_RenderPresent()
     * swaps/publishes the result. Callers must provide a full current frame, not only incremental
     * commands. The M3 demo obtains that frame through PresentationCoordinator::replay(), which
     * reconstructs a clean semantic tree without manufacturing Widget invalidation. This favors
     * correctness while dirty-region/back-buffer policy is still open.
     *
     * Returns the number of commands successfully replayed.
     */
    [[nodiscard]] std::size_t presentFrame(const DisplayList& list);

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
