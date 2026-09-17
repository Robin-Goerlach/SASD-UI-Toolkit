#pragma once

#include <sasd/ui/events/event.hpp>

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>

namespace sasd::ui {

/** Thread-safe FIFO queue used at backend/application boundaries. */
class EventQueue {
public:
    EventQueue() = default;

    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

    void push(Event event);
    [[nodiscard]] std::optional<Event> tryPop();
    [[nodiscard]] bool empty() const;
    [[nodiscard]] std::size_t size() const;
    void clear();

private:
    mutable std::mutex mutex_;
    std::deque<Event> events_;
};

} // namespace sasd::ui
