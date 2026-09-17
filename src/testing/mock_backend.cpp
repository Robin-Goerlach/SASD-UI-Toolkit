#include <sasd/ui/testing/mock_backend.hpp>

#include <stdexcept>
#include <utility>

namespace sasd::ui::testing {

MockBackend::MockBackend(BackendCapabilities capabilities) : capabilities_{capabilities} {}

void MockBackend::initialize() {
    if (initialized_) {
        throw std::logic_error("MockBackend is already initialized");
    }

    initialized_ = true;
    ++initialize_count_;
}

void MockBackend::shutdown() noexcept {
    if (!initialized_) {
        return;
    }

    initialized_ = false;
    ++shutdown_count_;
}

std::optional<Event> MockBackend::pollEvent() {
    return events_.tryPop();
}

void MockBackend::postEvent(Event event) {
    events_.push(std::move(event));
}

} // namespace sasd::ui::testing
