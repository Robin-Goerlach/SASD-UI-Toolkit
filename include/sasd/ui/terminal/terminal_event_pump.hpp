#pragma once

#include <sasd/ui/events/event.hpp>
#include <sasd/ui/terminal/ansi_input_decoder.hpp>
#include <sasd/ui/terminal/terminal_session.hpp>

#include <chrono>
#include <optional>
#include <vector>

namespace sasd::ui::terminal {

/**
 * Timing policy for one non-blocking terminal event source.
 *
 * incomplete_sequence_timeout resolves byte prefixes that remain incomplete after successive polls.
 * Its most visible purpose is distinguishing a lone Escape key from the beginning of CSI/SS3, but it
 * also prevents a truncated UTF-8 scalar from remaining buffered forever.
 */
struct TerminalEventPumpOptions {
    std::chrono::milliseconds incomplete_sequence_timeout{30};

    friend constexpr bool operator==(const TerminalEventPumpOptions&,
                                     const TerminalEventPumpOptions&) = default;
};

/**
 * Converts non-blocking TerminalSession input/size state into semantic toolkit Events.
 *
 * TerminalEventPump does not route events, mutate widgets, sleep, repaint or own the terminal
 * session. One poll() call is one deterministic observation step:
 *
 *   size check -> optional ResizeEvent
 *   available bytes -> AnsiInputDecoder
 *   timeout expiry -> decoder flush
 *
 * Keeping sleep/wakeup policy outside lets Application integrations choose a simple polling loop now
 * and a platform wait/wakeup mechanism later without changing terminal decoding semantics.
 */
class TerminalEventPump final {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit TerminalEventPump(TerminalSession& session,
                               TerminalEventPumpOptions options = {});

    [[nodiscard]] const TerminalEventPumpOptions& options() const noexcept {
        return options_;
    }

    /** Last terminal dimensions observed by the pump. Constructor captures the initial size. */
    [[nodiscard]] Size lastKnownSize() const noexcept {
        return last_size_;
    }

    /**
     * Performs one non-blocking observation step at the supplied monotonic time.
     *
     * Supplying time explicitly makes ESC/incomplete-sequence timeout behavior deterministic in tests.
     * Production callers normally use the convenience overload below.
     */
    [[nodiscard]] std::vector<Event> poll(TimePoint now);

    /** Performs one observation using std::chrono::steady_clock::now(). */
    [[nodiscard]] std::vector<Event> poll() {
        return poll(Clock::now());
    }

    /** Discards pending decoder/timing state while retaining the last observed terminal size. */
    void resetInput() noexcept;

private:
    void append(std::vector<Event>& destination, std::vector<Event> source);

    TerminalSession& session_;
    TerminalEventPumpOptions options_{};
    AnsiInputDecoder decoder_;
    Size last_size_{};
    std::optional<TimePoint> pending_since_;
};

} // namespace sasd::ui::terminal
