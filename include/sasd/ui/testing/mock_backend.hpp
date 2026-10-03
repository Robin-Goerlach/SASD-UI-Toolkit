#pragma once

#include <sasd/ui/backend/backend.hpp>
#include <sasd/ui/events/event_queue.hpp>
#include <sasd/ui/testing/memory_clipboard.hpp>

#include <cstddef>
#include <string_view>

namespace sasd::ui::testing {

/**
 * Deterministic headless backend used to validate the platform-neutral core and backend contracts.
 * It is test infrastructure, not a user-facing presentation backend.
 *
 * Optional services follow both the configured capability set and the backend lifecycle. In
 * particular, clipboard() returns the backend-owned MemoryClipboard only while the backend is
 * initialized and BackendCapabilities::clipboard is true. Capability reporting describes what the
 * backend can provide; service discovery describes what is usable now. Keeping those two concepts
 * distinct gives future consumer tests a realistic lifecycle without teaching production backends
 * fake clipboard behavior.
 */
class MockBackend final : public Backend {
public:
    explicit MockBackend(BackendCapabilities capabilities = {});

    [[nodiscard]] std::string_view name() const noexcept override { return "mock"; }
    [[nodiscard]] BackendCapabilities capabilities() const noexcept override { return capabilities_; }

    void initialize() override;
    void shutdown() noexcept override;
    [[nodiscard]] std::optional<Event> pollEvent() override;

    /**
     * Exposes the backend-owned clipboard only while this backend is active.
     *
     * A configured capability is deliberately not enough on its own. Native services frequently
     * depend on platform state created by initialize(), so the headless backend follows the same
     * lifecycle shape instead of making clipboard access accidentally valid before startup or after
     * shutdown. The pointer remains non-owning and is valid only while this backend stays alive and
     * initialized.
     */
    [[nodiscard]] Clipboard* clipboard() noexcept override {
        return initialized_ && capabilities_.clipboard ? &clipboard_ : nullptr;
    }

    [[nodiscard]] const Clipboard* clipboard() const noexcept override {
        return initialized_ && capabilities_.clipboard ? &clipboard_ : nullptr;
    }

    void postEvent(Event event);

    [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }
    [[nodiscard]] std::size_t initializeCount() const noexcept { return initialize_count_; }
    [[nodiscard]] std::size_t shutdownCount() const noexcept { return shutdown_count_; }
    [[nodiscard]] std::size_t pendingEventCount() const { return events_.size(); }

private:
    BackendCapabilities capabilities_{};
    EventQueue events_;
    MemoryClipboard clipboard_;
    bool initialized_{false};
    std::size_t initialize_count_{0};
    std::size_t shutdown_count_{0};
};

} // namespace sasd::ui::testing
