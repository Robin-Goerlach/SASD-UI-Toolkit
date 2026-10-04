#include <sasd/ui/terminal/terminal_session.hpp>

#include <sasd/ui/terminal/ansi_frame_encoder.hpp>
#include <sasd/ui/terminal/presentation_frame.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>

#include <stdexcept>
#include <string_view>

namespace sasd::ui::terminal {
namespace {

/*
 * Button-event tracking (DECSET 1002) reports presses, releases, and motion while a button is held.
 * That is the narrowest tracking mode that supports click-and-drag selection without asking the
 * terminal to stream hover motion continuously. SGR mode (DECSET 1006) supplies unambiguous decimal,
 * one-based coordinates that AnsiInputDecoder already knows how to convert into PointerEvent values.
 *
 * Both modes are enabled in one transport write so normal devices cannot observe an application-level
 * gap between the two requests. Teardown reverses the order: first stop SGR encoding, then stop
 * button-event tracking. The byte sequences are kept private to the terminal session boundary rather
 * than leaking protocol details into callers or Widgets.
 */
constexpr std::string_view enable_pointer_input = "\x1B[?1002h\x1B[?1006h";
constexpr std::string_view disable_pointer_input = "\x1B[?1006l\x1B[?1002l";

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

    /*
     * Native setup is transactional by TerminalDevice contract. Cross-platform ANSI session protocols
     * are layered only after that setup succeeds because their bytes require a usable output transport
     * (and, on Windows, the VT mode established by the native adapter).
     */
    device_.beginSession(options);

    if (options.pointer_input) {
        try {
            device_.write(enable_pointer_input);
            pointer_input_enabled_ = true;
        } catch (...) {
            /*
             * A real transport can fail after having emitted some bytes. We therefore attempt the full
             * inverse sequence even though pointer_input_enabled_ was not published yet. endSession()
             * follows regardless, giving construction the same strong lifetime guarantee as the native
             * beginSession() contract: a throwing TerminalSession constructor does not leave us owning
             * an active device session.
             */
            bestEffortWrite(device_, disable_pointer_input);
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

    if (pointer_input_enabled_) {
        pointer_input_enabled_ = false;
        bestEffortWrite(device_, disable_pointer_input);
    }

    /*
     * Native restoration is always last. In particular, a failed pointer-disable write must never skip
     * termios/Win32 restoration. endSession() is idempotent and noexcept by contract.
     */
    device_.endSession();
}

} // namespace sasd::ui::terminal
