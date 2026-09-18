#pragma once

#include <sasd/ui/events/event.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace sasd::ui::terminal {

/**
 * Incremental ANSI/VT input-byte decoder.
 *
 * Terminal reads may split escape sequences and UTF-8 scalars arbitrarily. AnsiInputDecoder therefore
 * owns a small pending-byte buffer and emits only semantic Events that are unambiguous.
 *
 * A lone ESC is intentionally ambiguous: it may be the Escape key or the first byte of a later CSI
 * sequence. feed() keeps it pending; flushPending() resolves remaining ambiguity when the event loop's
 * Escape timeout expires.
 */
class AnsiInputDecoder final {
public:
    /** Appends newly available bytes and returns every semantic Event that can be decoded now. */
    [[nodiscard]] std::vector<Event> feed(std::string_view bytes);

    /**
     * Resolves all currently pending bytes without waiting for more input.
     *
     * A lone ESC becomes Key::escape. Incomplete UTF-8 bytes follow the shared U+FFFD replacement
     * policy. This method is intended for an event-loop timeout, not after every device poll.
     */
    [[nodiscard]] std::vector<Event> flushPending();

    [[nodiscard]] bool hasPendingInput() const noexcept { return !pending_.empty(); }
    [[nodiscard]] std::string_view pendingBytes() const noexcept { return pending_; }

    /** Discards partial input state, for example when a terminal session is torn down. */
    void reset() noexcept {
        pending_.clear();
        suppress_next_lf_ = false;
    }

private:
    [[nodiscard]] std::vector<Event> decode(bool flush);

    std::string pending_;

    // CR is emitted immediately as Enter; a following LF in the next device chunk is suppressed.
    bool suppress_next_lf_{false};
};

} // namespace sasd::ui::terminal
