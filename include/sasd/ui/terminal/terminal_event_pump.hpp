#pragma once

#include <sasd/ui/events/event.hpp>
#include <sasd/ui/terminal/ansi_input_decoder.hpp>
#include <sasd/ui/terminal/terminal_session.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace sasd::ui::terminal {

/**
 * Timing policy for one non-blocking terminal event source.
 *
 * incomplete_sequence_timeout resolves byte prefixes that remain incomplete after successive polls.
 * Its most visible purpose is distinguishing a lone Escape key from the beginning of CSI/SS3, but it
 * also prevents a truncated UTF-8 scalar from remaining buffered forever.
 *
 * multi_click_interval is the maximum observed time between one completed terminal click and the next
 * press for both transitions to belong to the same click chain. SGR mouse reports carry no native
 * click count, so TerminalEventPump derives the backend-neutral PointerEvent::click_count here, where
 * a monotonic observation time already exists. Terminal cells are discrete; continuation additionally
 * requires the same button and exact same cell rather than inventing sub-cell distance tolerance.
 */
struct TerminalEventPumpOptions {
    std::chrono::milliseconds incomplete_sequence_timeout{30};
    std::chrono::milliseconds multi_click_interval{500};

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
 *   decoded pointer transitions -> terminal multi-click count synthesis
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
     * Supplying time explicitly makes ESC/incomplete-sequence timeout and terminal multi-click
     * classification deterministic in tests. Production callers normally use the convenience overload
     * below. Terminal protocols do not carry event timestamps, so all transitions decoded from one device
     * read necessarily share this observation time.
     */
    [[nodiscard]] std::vector<Event> poll(TimePoint now);

    /** Performs one observation using std::chrono::steady_clock::now(). */
    [[nodiscard]] std::vector<Event> poll() {
        return poll(Clock::now());
    }

    /**
     * Discards pending decoder and terminal input-gesture timing state while retaining terminal size.
     *
     * Resetting a decoder boundary must also end any remembered click chain. Otherwise a click before an
     * explicit input reset could be combined with a later click even though the host intentionally
     * discarded the intervening input history.
     */
    void resetInput() noexcept;

private:
    struct ActivePointerPress {
        PointerButton button{PointerButton::none};
        Point position{};
        std::uint8_t click_count{1};
        bool moved{false};
    };

    struct CompletedPointerClick {
        PointerButton button{PointerButton::none};
        Point position{};
        std::uint8_t click_count{1};
        TimePoint released_at{};
    };

    void append(std::vector<Event>& destination, std::vector<Event> source);
    void synthesizePointerClickCounts(std::vector<Event>& events, TimePoint now) noexcept;

    TerminalSession& session_;
    TerminalEventPumpOptions options_{};
    AnsiInputDecoder decoder_;
    Size last_size_{};
    std::optional<TimePoint> pending_since_;
    std::optional<ActivePointerPress> active_pointer_press_;
    std::optional<CompletedPointerClick> last_completed_pointer_click_;
};

} // namespace sasd::ui::terminal
