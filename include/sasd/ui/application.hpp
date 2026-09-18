#pragma once

#include <sasd/ui/backend/backend.hpp>

#include <cstddef>
#include <functional>

namespace sasd::ui {

class Widget;

/**
 * Owns toolkit-level runtime state and coordinates a selected backend.
 *
 * M1 deliberately implements deterministic event pumping rather than a blocking desktop event
 * loop. Visible backends will later add the waiting/wakeup behavior needed by Application::run().
 */
class Application {
public:
    using EventHandler = std::function<void(const Event&)>;

    /**
     * Resolves a backend-neutral event to the widget that should receive it first.
     *
     * Application deliberately does not know how keyboard focus, pointer hit testing or top-level
     * window activation choose a target. Those policies can therefore evolve independently while
     * the event pump only coordinates polling and dispatch.
     *
     * The returned pointer is non-owning and must remain valid for the duration of the synchronous
     * EventDispatcher call made by processRoutedEvents().
     */
    using EventTargetResolver = std::function<Widget*(const Event&)>;

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

    /**
     * Drains all events currently available from the backend in FIFO order.
     *
     * This compatibility/raw-pump form does not route events into the widget tree. The optional
     * handler receives every event after Application has processed its own lifecycle semantics.
     */
    std::size_t processEvents(const EventHandler& handler = {});

    /**
     * Drains backend events and routes each non-QuitEvent to a resolved widget target.
     *
     * For every normal semantic event, target_resolver may return a widget. EventDispatcher then
     * performs target-to-parent bubbling. If no target exists or the complete route ignores the
     * event, unhandled_handler receives it when supplied.
     *
     * QuitEvent is intentionally application-level: it sets exitRequested() and bypasses widget
     * routing so a widget cannot accidentally suppress application shutdown. It is still offered to
     * unhandled_handler, which keeps logging/diagnostic code able to observe the complete pump.
     */
    std::size_t processRoutedEvents(const EventTargetResolver& target_resolver,
                                    const EventHandler& unhandled_handler = {});

private:
    Backend& backend_;
    bool exit_requested_{false};
};

} // namespace sasd::ui
