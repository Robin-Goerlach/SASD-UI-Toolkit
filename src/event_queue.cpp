#include <sasd/ui/events/event_queue.hpp>

#include <utility>

namespace sasd::ui {

void EventQueue::push(Event event) {
    std::scoped_lock lock{mutex_};
    events_.push_back(std::move(event));
}

std::optional<Event> EventQueue::tryPop() {
    std::scoped_lock lock{mutex_};
    if (events_.empty()) {
        return std::nullopt;
    }

    Event event = std::move(events_.front());
    events_.pop_front();
    return event;
}

bool EventQueue::empty() const {
    std::scoped_lock lock{mutex_};
    return events_.empty();
}

std::size_t EventQueue::size() const {
    std::scoped_lock lock{mutex_};
    return events_.size();
}

void EventQueue::clear() {
    std::scoped_lock lock{mutex_};
    events_.clear();
}

} // namespace sasd::ui
