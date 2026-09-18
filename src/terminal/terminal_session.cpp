#include <sasd/ui/terminal/terminal_session.hpp>

#include <sasd/ui/terminal/ansi_frame_encoder.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>

#include <stdexcept>

namespace sasd::ui::terminal {

TerminalSession::TerminalSession(TerminalDevice& device,
                                 TerminalSessionOptions options)
    : device_{device} {
    if (!device_.isInteractive()) {
        throw std::runtime_error("TerminalSession requires an interactive terminal device");
    }

    /*
     * Publish active_ only after beginSession() succeeds. TerminalDevice implementations guarantee
     * transactional rollback on failure, so a throwing constructor neither owns nor needs to tear
     * down a half-open native session.
     */
    device_.beginSession(options);
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

void TerminalSession::close() noexcept {
    if (!active_) {
        return;
    }

    /*
     * Clear our ownership flag first. Even if a custom device violates its noexcept promise
     * internally, re-entrant close/destructor paths will not attempt a second restoration.
     */
    active_ = false;
    device_.endSession();
}

} // namespace sasd::ui::terminal
