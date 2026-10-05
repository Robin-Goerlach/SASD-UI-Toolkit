#include <sasd/ui/terminal/terminal_event_pump.hpp>

#include <stdexcept>
#include <utility>
#include <variant>

namespace sasd::ui::terminal {

TerminalEventPump::TerminalEventPump(TerminalSession& session,
                                     TerminalEventPumpOptions options)
    : session_{session}, options_{options}, last_size_{session.size()} {
    if (options_.incomplete_sequence_timeout.count() < 0) {
        throw std::invalid_argument(
            "TerminalEventPump incomplete_sequence_timeout must be non-negative");
    }

    if (options_.multi_click_interval.count() < 0) {
        throw std::invalid_argument(
            "TerminalEventPump multi_click_interval must be non-negative");
    }
}

void TerminalEventPump::append(std::vector<Event>& destination,
                               std::vector<Event> source) {
    for (Event& event : source) {
        destination.push_back(std::move(event));
    }
}

void TerminalEventPump::synthesizePointerClickCounts(std::vector<Event>& events,
                                                      TimePoint now) noexcept {
    /*
     * SGR 1006 reports button transitions and coordinates but no native multi-click count. Enrich the
     * decoded events here rather than in AnsiInputDecoder: the pump already owns a monotonic observation
     * time, while the byte decoder intentionally remains a clock-free incremental protocol parser.
     *
     * A click chain continues only after a completed, non-dragged click of the same button in the exact
     * same terminal cell and within the configured interval. The exact-cell rule is deliberately strict:
     * terminal input has no honest sub-cell geometry from which to derive a desktop-style click rectangle.
     */
    for (Event& event : events) {
        auto* pointer = std::get_if<PointerEvent>(&event);
        if (pointer == nullptr) {
            continue;
        }

        if (pointer->action == PointerAction::move) {
            pointer->click_count = 0;

            if (active_pointer_press_.has_value() &&
                pointer->position != active_pointer_press_->position) {
                /*
                 * Once a pressed gesture has moved to another terminal cell it is a drag for click-chain
                 * purposes, even if the pointer later returns to the original cell before release. This
                 * prevents a selection drag from becoming the first half of an accidental double click.
                 */
                active_pointer_press_->moved = true;
            }
            continue;
        }

        if (pointer->action == PointerAction::press) {
            if (pointer->button == PointerButton::none) {
                /*
                 * A press without button identity is outside the current PointerEvent contract. Keep the
                 * event conservative and discard remembered click state instead of associating it with a
                 * previous real button transition.
                 */
                pointer->click_count = 1;
                active_pointer_press_.reset();
                last_completed_pointer_click_.reset();
                continue;
            }

            if (active_pointer_press_.has_value()) {
                /*
                 * SGR button-event tracking is expected to provide a matching release before another press
                 * in this simplified pointer model. If the stream violates that assumption, do not carry
                 * stale timing state across the ambiguous boundary.
                 */
                active_pointer_press_.reset();
                last_completed_pointer_click_.reset();
            }

            std::uint8_t click_count = 1;
            if (last_completed_pointer_click_.has_value()) {
                const CompletedPointerClick& previous = *last_completed_pointer_click_;
                const bool time_is_monotonic = now >= previous.released_at;
                const bool within_interval =
                    time_is_monotonic &&
                    now - previous.released_at <= options_.multi_click_interval;

                if (within_interval &&
                    previous.button == pointer->button &&
                    previous.position == pointer->position) {
                    /*
                     * The current toolkit has explicit useful semantics for single, double and triple
                     * clicks. Saturate at three instead of allowing uint8_t wraparound or making a rapid
                     * fourth click unexpectedly look like an unrelated value to downstream interaction
                     * helpers. A later richer gesture model can revise this policy deliberately.
                     */
                    click_count =
                        previous.click_count < 3
                            ? static_cast<std::uint8_t>(previous.click_count + 1U)
                            : static_cast<std::uint8_t>(3);
                }
            }

            pointer->click_count = click_count;
            active_pointer_press_ = ActivePointerPress{
                pointer->button,
                pointer->position,
                click_count,
                false};
            continue;
        }

        if (pointer->action == PointerAction::release) {
            if (active_pointer_press_.has_value() &&
                active_pointer_press_->button == pointer->button) {
                const ActivePointerPress active = *active_pointer_press_;
                pointer->click_count = active.click_count;

                if (!active.moved && pointer->position == active.position) {
                    /*
                     * Only a completed press/release in one cell can seed the next click in a chain. The
                     * release time is the least ambiguous terminal-side reference because the previous
                     * click is fully complete before a later press may extend it.
                     */
                    last_completed_pointer_click_ = CompletedPointerClick{
                        active.button,
                        active.position,
                        active.click_count,
                        now};
                } else {
                    last_completed_pointer_click_.reset();
                }

                active_pointer_press_.reset();
            } else {
                /*
                 * An unmatched release cannot safely complete a remembered click. Preserve a valid
                 * single-transition count for consumers but terminate all synthesis state.
                 */
                pointer->click_count = 1;
                active_pointer_press_.reset();
                last_completed_pointer_click_.reset();
            }
        }
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

    /*
     * Keep protocol decoding and click classification as two explicit stages. This also means a future
     * terminal protocol can emit PointerEvent through the decoder without reimplementing click timing,
     * while non-pointer events and resize ordering pass through unchanged.
     */
    synthesizePointerClickCounts(events, now);
    return events;
}

void TerminalEventPump::resetInput() noexcept {
    decoder_.reset();
    pending_since_.reset();
    active_pointer_press_.reset();
    last_completed_pointer_click_.reset();
}

} // namespace sasd::ui::terminal
