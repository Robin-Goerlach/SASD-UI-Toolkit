#pragma once

#include <sasd/ui/backend/backend.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace sasd::ui::native::win32 {

/** Configuration for the first Win32 native host. */
struct Win32WindowBackendConfig {
    std::string title{"SASD UI Toolkit"};
    Size initial_size{800, 500};
    bool hidden{false};
    bool resizable{true};
};

/**
 * Native Win32 top-level host for the backend-neutral Application/Event pipeline.
 *
 * The host owns one HWND through RAII and translates its message queue into existing Core Events.
 * It intentionally does not create native Button/TextField peers yet: the semantic Widget tree and
 * its existing EventDispatcher/FocusManager/PointerRouter contracts remain the single source of
 * interaction semantics. IME composition, native child controls and accessibility are deferred.
 */
class Win32WindowBackend final : public Backend {
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

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sasd::ui::native::win32
