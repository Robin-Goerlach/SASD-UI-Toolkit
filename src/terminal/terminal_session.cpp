#include <sasd/ui/terminal/terminal_session.hpp>

#include <sasd/ui/terminal/ansi_frame_encoder.hpp>
#include <sasd/ui/terminal/presentation_frame.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>

#include <optional>
#include <stdexcept>
#include <string_view>

namespace sasd::ui::terminal {
namespace {

/**
 * Complete enable/disable byte pair for one pointer tracking policy.
 *
 * DECSET 1002 is button-event tracking: presses, releases, and motion while a button is held. DECSET 1003
 * is all-motion tracking and additionally reports passive movement with no button pressed. The modes are
 * selected as alternatives rather than stacking both requests; either one is paired with DECSET 1006 so
 * AnsiInputDecoder receives the same SGR decimal coordinate representation.
 *
 * Keeping the pair as one value matters for failure handling. A constructor write may fail after emitting an
 * unknown prefix, so rollback always has the exact inverse sequence for the selected policy available without
 * inferring state from partially published member fields.
 */
struct PointerProtocolBytes {
    std::string_view enable;
    std::string_view disable;
};

[[nodiscard]] constexpr std::optional<PointerProtocolBytes>
pointerProtocolBytes(TerminalPointerTrackingMode mode) noexcept {
    switch (mode) {
    case TerminalPointerTrackingMode::button_events:
        return PointerProtocolBytes{
            "\x1B[?1002h\x1B[?1006h",
            "\x1B[?1006l\x1B[?1002l"};
    case TerminalPointerTrackingMode::all_motion:
        return PointerProtocolBytes{
            "\x1B[?1003h\x1B[?1006h",
            "\x1B[?1006l\x1B[?1003l"};
    }

    /*
     * Strongly typed callers cannot normally reach this branch. Returning std::nullopt still makes the
     * session fail closed if an out-of-range enum value arrives through an explicit cast or serialized
     * configuration rather than silently choosing a different terminal protocol.
     */
    return std::nullopt;
}

/**
 * Best-effort protocol restoration for noexcept/rollback paths.
 *
 * TerminalDevice::write is intentionally a throwing all-or-exception transport API. Session teardown,
 * however, must never let one failed control-sequence write prevent native terminal restoration. This
 * helper therefore suppresses transport failures only at the lifetime boundary; normal frame and input
 * operations continue to surface their errors to callers.
 */
void bestEffortWrite(TerminalDevice& device, std::string_view bytes) noexcept {
    try {
        device.write(bytes);
    } catch (...) {
        // Native endSession() must still run; destructors cannot safely propagate transport failures.
    }
}

} // namespace

TerminalSession::TerminalSession(TerminalDevice& device,
                                 TerminalSessionOptions options)
    : device_{device} {
    if (!device_.isInteractive()) {
        throw std::runtime_error("TerminalSession requires an interactive terminal device");
    }

    std::optional<PointerProtocolBytes> pointer_protocol;
    if (options.pointer_input) {
        pointer_protocol = pointerProtocolBytes(options.pointer_tracking);
        if (!pointer_protocol.has_value()) {
            /*
             * Validate portable protocol intent before native terminal mutation. Although ordinary enum use
             * cannot create an invalid value, this keeps a cast/configuration error transactional and avoids
             * entering raw/alternate-screen mode only to reject the cross-platform protocol afterwards.
             */
            throw std::invalid_argument("TerminalSession received an invalid pointer tracking mode");
        }
    }

    /*
     * Native setup is transactional by TerminalDevice contract. Cross-platform ANSI session protocols
     * are layered only after that setup succeeds because their bytes require a usable output transport
     * (and, on Windows, the VT mode established by the native adapter).
     */
    device_.beginSession(options);

    if (pointer_protocol.has_value()) {
        try {
            device_.write(pointer_protocol->enable);
            pointer_tracking_ = options.pointer_tracking;
        } catch (...) {
            /*
             * A real transport can fail after having emitted some bytes. We therefore attempt the complete
             * inverse for the requested tracking policy even though pointer_tracking_ was not published yet.
             * endSession() follows regardless, giving construction the same strong lifetime guarantee as the
             * native beginSession() contract: a throwing TerminalSession constructor does not leave us owning
             * an active device session or intentionally enabled terminal protocol.
             */
            bestEffortWrite(device_, pointer_protocol->disable);
            device_.endSession();
            throw;
        }
    }

    /*
     * Publish active_ only after every requested session layer has succeeded. A fully constructed
     * TerminalSession therefore always represents one coherent lifetime: native terminal state plus
     * all optional ANSI protocols requested by its options.
     */
    active_ = true;
}

TerminalSession::~TerminalSession() {
    close();
}

Size TerminalSession::size() const {
    if (!active_) {
        throw std::logic_error("TerminalSession::size requires an active session");
    }
    return device_.size();
}

void TerminalSession::present(const ScreenBuffer& buffer,
                              std::optional<Point> caret) {
    if (!active_) {
        throw std::logic_error("TerminalSession::present requires an active session");
    }

    const std::string frame = AnsiFrameEncoder::encode(buffer, caret);
    device_.write(frame);
}

void TerminalSession::present(const TerminalPresentationFrame& frame) {
    /*
     * Keep TerminalPresentationFrame as transport input rather than a second transport implementation.
     * Delegating to the established primitive guarantees that active-state validation, ANSI encoding,
     * write failure behavior, and future transport fixes stay identical for both public entry points.
     */
    present(frame.buffer, frame.caret);
}

std::string TerminalSession::pollInputBytes() {
    if (!active_) {
        throw std::logic_error("TerminalSession::pollInputBytes requires an active session");
    }

    return device_.readAvailable();
}

void TerminalSession::close() noexcept {
    if (!active_) {
        return;
    }

    /*
     * Clear our ownership flag first so re-entrant close/destructor paths cannot attempt a second
     * teardown. The TerminalDevice itself remains native-active until endSession() below, which lets us
     * send the protocol-disable bytes while VT output is still configured.
     */
    active_ = false;

    if (pointer_tracking_.has_value()) {
        const TerminalPointerTrackingMode mode = *pointer_tracking_;
        pointer_tracking_.reset();

        /*
         * Only a validated mode is ever published, so lookup should always succeed. Keep shutdown noexcept and
         * fail closed if memory/state corruption somehow violates that invariant: native restoration below is
         * still more important than surfacing an exception from a destructor path.
         */
        if (const auto protocol = pointerProtocolBytes(mode); protocol.has_value()) {
            bestEffortWrite(device_, protocol->disable);
        }
    }

    /*
     * Native restoration is always last. In particular, a failed pointer-disable write must never skip
     * termios/Win32 restoration. endSession() is idempotent and noexcept by contract.
     */
    device_.endSession();
}

} // namespace sasd::ui::terminal
