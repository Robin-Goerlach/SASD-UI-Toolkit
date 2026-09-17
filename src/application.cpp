#include <sasd/ui/application.hpp>

#include <utility>
#include <variant>

namespace sasd::ui {

Application::Application(Backend& backend) : backend_{backend} {
    backend_.initialize();
}

Application::~Application() {
    backend_.shutdown();
}

std::size_t Application::processEvents(const EventHandler& handler) {
    std::size_t processed = 0;

    while (auto event = backend_.pollEvent()) {
        ++processed;

        if (std::holds_alternative<QuitEvent>(*event)) {
            exit_requested_ = true;
        }

        if (handler) {
            handler(*event);
        }
    }

    return processed;
}

} // namespace sasd::ui
