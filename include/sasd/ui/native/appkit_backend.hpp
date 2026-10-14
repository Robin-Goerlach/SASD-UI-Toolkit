#pragma once

#include <sasd/ui/backend/backend.hpp>
#include <sasd/ui/menu_model.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/render_device.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace sasd::ui::native::appkit {

struct AppKitWindowBackendConfig {
    std::string title{"SASD UI Toolkit"};
    Size initial_size{800, 500};
    bool hidden{false};
    bool resizable{true};
    double font_point_size{12.0};
    Color background_color{Color::black};
};

/**
 * AppKit top-level host for the existing Core and Rendered pipelines.
 *
 * The native Objective-C++ implementation owns one NSWindow and one custom NSView. It does not
 * create native child controls: Core Button/TextField semantics and the owned DisplayList remain
 * the only semantic and presentation sources.
 */
class AppKitWindowBackend final : public Backend,
                                  public rendered::RenderDevice,
                                  public rendered::RenderedMeasurementContext {
public:
    explicit AppKitWindowBackend(AppKitWindowBackendConfig config = {});
    ~AppKitWindowBackend() override;

    AppKitWindowBackend(const AppKitWindowBackend&) = delete;
    AppKitWindowBackend& operator=(const AppKitWindowBackend&) = delete;
    AppKitWindowBackend(AppKitWindowBackend&&) = delete;
    AppKitWindowBackend& operator=(AppKitWindowBackend&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override { return "native-appkit"; }
    [[nodiscard]] BackendCapabilities capabilities() const noexcept override;

    void initialize() override;
    void shutdown() noexcept override;
    [[nodiscard]] std::optional<Event> pollEvent() override;

    [[nodiscard]] bool isInitialized() const noexcept;
    [[nodiscard]] Size windowSize() const;
    [[nodiscard]] bool presentationRequested() const noexcept;
    [[nodiscard]] std::size_t presentFrame(const rendered::DisplayList& list);

    /**
     * Installs an application-global NSMenu projection of the current semantic menu bar.
     *
     * AppKit has one application-global NSApplication::mainMenu rather than a per-window menu slot.
     * On the AppKit main thread, the first backend that synchronizes while mainMenu is empty becomes
     * the sole SASD native-menu owner. A second backend is rejected, while an existing process menu
     * is retained as the restoration target rather than silently discarded. Structural changes
     * require another explicit call. The projection owns copied native labels and
     * Command::Reference values only; it retains no MenuModel/MenuItem.
     */
    void syncMenuBar(const MenuBarModel& menu_bar);

    /**
     * Removes this backend's application-global mainMenu projection and restores the menu that
     * preceded installation when it is still the currently installed menu. It is harmless after
     * shutdown, from a non-AppKit thread, or when another owner has already replaced the process
     * menu.
     */
    void clearMenuBar() noexcept;

    void fillRect(const rendered::FillRectCommand& command) override;
    void strokeRect(const rendered::StrokeRectCommand& command) override;
    void drawText(const rendered::DrawTextCommand& command) override;

    [[nodiscard]] Size measureText(std::string_view utf8_text) const override;
    [[nodiscard]] Coordinate lineHeight() const noexcept override;
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view utf8_text, std::size_t scalar_index) const override;
    [[nodiscard]] std::uint64_t revision() const noexcept override;

    // The definition remains private to Objective-C++; naming the incomplete PImpl type here keeps
    // the public header free of AppKit types while allowing the private NSView delegate to hold it.
    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace sasd::ui::native::appkit
