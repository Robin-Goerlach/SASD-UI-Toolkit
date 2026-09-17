#pragma once

#include <sasd/ui/backend/backend.hpp>
#include <sasd/ui/events/event_queue.hpp>

#include <cstddef>
#include <string_view>

namespace sasd::ui::testing {

/**
 * Deterministic headless backend used to validate the platform-neutral core and backend contracts.
 * It is test infrastructure, not a user-facing presentation backend.
 */
class MockBackend final : public Backend {
public:
    explicit MockBackend(BackendCapabilities capabilities = {});

    [[nodiscard]] std::string_view name() const noexcept override { return "mock"; }
    [[nodiscard]] BackendCapabilities capabilities() const noexcept override { return capabilities_; }

    void initialize() override;
    void shutdown() noexcept override;
    [[nodiscard]] std::optional<Event> pollEvent() override;

    void postEvent(Event event);

    [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }
    [[nodiscard]] std::size_t initializeCount() const noexcept { return initialize_count_; }
    [[nodiscard]] std::size_t shutdownCount() const noexcept { return shutdown_count_; }
    [[nodiscard]] std::size_t pendingEventCount() const { return events_.size(); }

private:
    BackendCapabilities capabilities_{};
    EventQueue events_;
    bool initialized_{false};
    std::size_t initialize_count_{0};
    std::size_t shutdown_count_{0};
};

} // namespace sasd::ui::testing
