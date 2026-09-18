#pragma once

#include <sasd/ui/terminal/terminal_device.hpp>

#include <optional>
#include <string>

namespace sasd::ui::terminal {

class ScreenBuffer;

/**
 * RAII owner of one active TerminalDevice session.
 *
 * TerminalSession deliberately contains no widget/event-loop policy. It ensures native terminal state
 * is restored and provides the narrow output bridge from ScreenBuffer/caret to TerminalDevice bytes.
 */
class TerminalSession final {
public:
    explicit TerminalSession(TerminalDevice& device,
                             TerminalSessionOptions options = {});
    ~TerminalSession();

    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;
    TerminalSession(TerminalSession&&) = delete;
    TerminalSession& operator=(TerminalSession&&) = delete;

    [[nodiscard]] TerminalDevice& device() noexcept { return device_; }
    [[nodiscard]] const TerminalDevice& device() const noexcept { return device_; }

    [[nodiscard]] bool active() const noexcept { return active_; }

    /** Delegates current visible cell dimensions to the active native device. */
    [[nodiscard]] Size size() const;

    /**
     * Encodes and writes one complete ScreenBuffer frame.
     *
     * The session remains active if transport throws; callers may retry or close explicitly.
     */
    void present(const ScreenBuffer& buffer,
                 std::optional<Point> caret = std::nullopt);

    /**
     * Polls bytes that are already available from the active device without blocking.
     *
     * Decoding those bytes into semantic Events belongs to AnsiInputDecoder, not to session
     * lifetime/transport.
     */
    [[nodiscard]] std::string pollInputBytes();

    /**
     * Restores the device immediately.
     *
     * Idempotent and noexcept. The destructor calls the same operation.
     */
    void close() noexcept;

private:
    TerminalDevice& device_;
    bool active_{false};
};

} // namespace sasd::ui::terminal
