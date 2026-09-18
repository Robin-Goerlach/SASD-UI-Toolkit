#pragma once

#include <sasd/ui/backend/backend.hpp>
#include <sasd/ui/terminal/terminal_event_pump.hpp>
#include <sasd/ui/terminal/terminal_session.hpp>

#include <deque>
#include <memory>
#include <optional>

namespace sasd::ui::terminal {

/**
 * First production Backend implementation for a real terminal session.
 *
 * Backend responsibilities stay intentionally narrow: lifecycle and semantic input polling. Widget
 * presentation still flows through TerminalPresentationSink -> ScreenBuffer -> TerminalSession, so
 * the generic Backend interface does not acquire renderer-specific methods.
 */
class TerminalBackend final : public Backend {
public:
    /** Creates a backend using the process' native POSIX/Windows terminal device. */
    TerminalBackend();

    /**
     * Dependency-injected constructor used by tests and embedders.
     *
     * device must be non-null and is owned exclusively by the backend.
     */
    explicit TerminalBackend(std::unique_ptr<TerminalDevice> device,
                             TerminalSessionOptions session_options = {},
                             TerminalEventPumpOptions pump_options = {});

    ~TerminalBackend() override;

    TerminalBackend(const TerminalBackend&) = delete;
    TerminalBackend& operator=(const TerminalBackend&) = delete;
    TerminalBackend(TerminalBackend&&) = delete;
    TerminalBackend& operator=(TerminalBackend&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "terminal";
    }

    [[nodiscard]] BackendCapabilities capabilities() const noexcept override {
        return {};
    }

    void initialize() override;
    void shutdown() noexcept override;
    [[nodiscard]] std::optional<Event> pollEvent() override;

    [[nodiscard]] bool isInitialized() const noexcept {
        return session_ != nullptr;
    }

    /** Access to the active terminal transport for presentation. */
    [[nodiscard]] TerminalSession& session();
    [[nodiscard]] const TerminalSession& session() const;

    /** Last dimensions observed by the terminal event pump. */
    [[nodiscard]] Size terminalSize() const;

private:
    [[nodiscard]] std::optional<Event> popQueuedEvent();

    std::unique_ptr<TerminalDevice> device_;
    TerminalSessionOptions session_options_{};
    TerminalEventPumpOptions pump_options_{};

    std::unique_ptr<TerminalSession> session_;
    std::unique_ptr<TerminalEventPump> pump_;
    std::deque<Event> queued_events_;
};

} // namespace sasd::ui::terminal
