#include <sasd/ui/terminal/terminal_event_pump.hpp>

#include <stdexcept>
#include <utility>

namespace sasd::ui::terminal {

TerminalEventPump::TerminalEventPump(TerminalSession& session,
                                     TerminalEventPumpOptions options)
    : session_{session}, options_{options}, last_size_{session.size()} {
    if (options_.incomplete_sequence_timeout.count() < 0) {
        throw std::invalid_argument(
            "TerminalEventPump incomplete_sequence_timeout must be non-negative");
    }
}

void TerminalEventPump::append(std::vector<Event>& destination,
                               std::vector<Event> source) {
    for (Event& event : source) {
        destination.push_back(std::move(event));
    }
}

std::vector<Event> TerminalEventPump::poll(TimePoint now) {
    std::vector<Event> events;

    /*
     * Resize is observed before input so downstream application code can update geometry before
     * handling a key/text event collected in the same tick. Focus identity is unaffected, but this
     * ordering gives pointer/hit-test capable backends a sensible precedent for later.
     */
    const Size current_size = session_.size();
    if (current_size != last_size_) {
        last_size_ = current_size;
        events.emplace_back(ResizeEvent{current_size});
    }

    const std::string bytes = session_.pollInputBytes();

    if (!bytes.empty()) {
        append(events, decoder_.feed(bytes));

        /*
         * A newly extended but still incomplete sequence receives a fresh timeout window. This is
         * important when ESC, '[', and a final CSI byte arrive in separate device reads.
         */
        if (decoder_.hasPendingInput()) {
            pending_since_ = now;
        } else {
            pending_since_.reset();
        }
    } else if (decoder_.hasPendingInput()) {
        if (!pending_since_) {
            // Defensive recovery: normal feed() paths establish this timestamp.
            pending_since_ = now;
        }

        if (now - *pending_since_ >= options_.incomplete_sequence_timeout) {
            append(events, decoder_.flushPending());

            if (decoder_.hasPendingInput()) {
                /*
                 * flushPending() is expected to make complete progress with today's decoder. Retain a
                 * fresh timestamp instead of spinning if a future protocol extension deliberately
                 * keeps partial state after a flush.
                 */
                pending_since_ = now;
            } else {
                pending_since_.reset();
            }
        }
    } else {
        pending_since_.reset();
    }

    return events;
}

void TerminalEventPump::resetInput() noexcept {
    decoder_.reset();
    pending_since_.reset();
}

} // namespace sasd::ui::terminal
