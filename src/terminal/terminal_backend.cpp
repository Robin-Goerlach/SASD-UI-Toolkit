#include <sasd/ui/terminal/terminal_backend.hpp>

#include <sasd/ui/terminal/native_terminal_device.hpp>

#include <stdexcept>
#include <utility>

namespace sasd::ui::terminal {

TerminalBackend::TerminalBackend()
    : TerminalBackend{createNativeTerminalDevice()} {}

TerminalBackend::TerminalBackend(std::unique_ptr<TerminalDevice> device,
                                 TerminalSessionOptions session_options,
                                 TerminalEventPumpOptions pump_options)
    : device_{std::move(device)},
      session_options_{session_options},
      pump_options_{pump_options} {
    if (!device_) {
        throw std::invalid_argument("TerminalBackend requires a non-null TerminalDevice");
    }
}

TerminalBackend::~TerminalBackend() {
    shutdown();
}

void TerminalBackend::initialize() {
    if (isInitialized()) {
        throw std::logic_error("TerminalBackend is already initialized");
    }

    /*
     * Construct into locals first. If TerminalSession or TerminalEventPump throws, local RAII restores
     * the native terminal and this backend remains cleanly uninitialized.
     */
    auto session = std::make_unique<TerminalSession>(*device_, session_options_);
    auto pump = std::make_unique<TerminalEventPump>(*session, pump_options_);

    queued_events_.clear();
    session_ = std::move(session);
    pump_ = std::move(pump);
}

void TerminalBackend::shutdown() noexcept {
    if (!isInitialized()) {
        return;
    }

    /*
     * Drop decoder/timing state before closing the native session. TerminalSession then restores raw
     * mode, code pages, cursor visibility and alternate-screen state through TerminalDevice.
     */
    queued_events_.clear();
    pump_.reset();
    session_.reset();
}

std::optional<Event> TerminalBackend::popQueuedEvent() {
    if (queued_events_.empty()) {
        return std::nullopt;
    }

    Event event = std::move(queued_events_.front());
    queued_events_.pop_front();
    return event;
}

std::optional<Event> TerminalBackend::pollEvent() {
    if (!isInitialized()) {
        throw std::logic_error("TerminalBackend::pollEvent requires initialize()");
    }

    if (auto queued = popQueuedEvent()) {
        return queued;
    }

    std::vector<Event> events = pump_->poll();
    for (Event& event : events) {
        queued_events_.push_back(std::move(event));
    }

    return popQueuedEvent();
}

TerminalSession& TerminalBackend::session() {
    if (!session_) {
        throw std::logic_error("TerminalBackend::session requires initialize()");
    }
    return *session_;
}

const TerminalSession& TerminalBackend::session() const {
    if (!session_) {
        throw std::logic_error("TerminalBackend::session requires initialize()");
    }
    return *session_;
}

Size TerminalBackend::terminalSize() const {
    if (!pump_) {
        throw std::logic_error("TerminalBackend::terminalSize requires initialize()");
    }
    return pump_->lastKnownSize();
}

} // namespace sasd::ui::terminal
