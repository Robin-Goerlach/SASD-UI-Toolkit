#pragma once

#include <sasd/ui/backend/backend.hpp>

#include <cstddef>
#include <functional>

namespace sasd::ui {

/**
 * Owns toolkit-level runtime state and coordinates a selected backend.
 *
 * M1 deliberately implements deterministic event pumping rather than a blocking desktop event
 * loop. Visible backends will later add the waiting/wakeup behavior needed by Application::run().
 */
class Application {
public:
    using EventHandler = std::function<void(const Event&)>;

    explicit Application(Backend& backend);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    [[nodiscard]] Backend& backend() noexcept { return backend_; }
    [[nodiscard]] const Backend& backend() const noexcept { return backend_; }

    [[nodiscard]] bool exitRequested() const noexcept { return exit_requested_; }
    void requestExit() noexcept { exit_requested_ = true; }

    /** Drains all events currently available from the backend in FIFO order. */
    std::size_t processEvents(const EventHandler& handler = {});

private:
    Backend& backend_;
    bool exit_requested_{false};
};

} // namespace sasd::ui
