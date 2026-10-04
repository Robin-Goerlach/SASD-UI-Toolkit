#pragma once

#include <sasd/ui/terminal/terminal_device.hpp>

#include <optional>
#include <string>

namespace sasd::ui::terminal {

class ScreenBuffer;
struct TerminalPresentationFrame;

/**
 * RAII owner of one active TerminalDevice session.
 *
 * TerminalSession deliberately contains no widget/event-loop policy. It ensures native terminal state
 * is restored, owns session-scoped ANSI protocols such as optional pointer reporting, and provides the
 * narrow output bridge from presentation-frame data to TerminalDevice bytes.
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

    /**
     * True while this session owns enabled terminal pointer reporting.
     *
     * This reports protocol lifetime only; it does not imply that a particular Widget currently owns
     * PointerRouter capture or that a pointer event is pending.
     */
    [[nodiscard]] bool pointerInputEnabled() const noexcept { return pointer_input_enabled_; }

    /** Delegates current visible cell dimensions to the active native device. */
    [[nodiscard]] Size size() const;

    /**
     * Encodes and writes one complete ScreenBuffer frame with optional hardware-caret metadata.
     *
     * This is the primitive transport operation. Higher presentation layers may use the
     * TerminalPresentationFrame overload below when buffer and caret already travel as one owned value.
     * Keeping this overload public remains useful for simple callers and preserves the established API.
     *
     * The session remains active if transport throws; callers may retry or close explicitly.
     */
    void present(const ScreenBuffer& buffer,
                 std::optional<Point> caret = std::nullopt);

    /**
     * Presents one complete TerminalPresentationFrame without unpacking it at the call site.
     *
     * TerminalPresentationFrame is deliberately a passive presentation value. This overload adds no
     * composition, focus, menu, or event-loop policy to TerminalSession; it merely delegates the frame's
     * buffer and caret to the existing primitive present(buffer, caret) operation. Consequently both
     * overloads share the same active-session checks, ANSI encoding, transport behavior, and retry semantics.
     *
     * Keeping delegation one-way is important for maintainability: there remains exactly one implementation
     * of terminal byte emission, while callers that already own a complete frame do not have to split its
     * metadata apart merely to cross the transport boundary.
     */
    void present(const TerminalPresentationFrame& frame);

    /**
     * Polls bytes that are already available from the active device without blocking.
     *
     * Decoding those bytes into semantic Events belongs to AnsiInputDecoder, not to session
     * lifetime/transport.
     */
    [[nodiscard]] std::string pollInputBytes();

    /**
     * Restores session-owned ANSI protocols and then the native device immediately.
     *
     * Idempotent and noexcept. Pointer-reporting shutdown is best-effort because close() is also the
     * destructor path; native endSession() is still called even when the final protocol write fails.
     */
    void close() noexcept;

private:
    TerminalDevice& device_;
    bool active_{false};
    bool pointer_input_enabled_{false};
};

} // namespace sasd::ui::terminal
