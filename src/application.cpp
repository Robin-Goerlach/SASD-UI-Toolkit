#include <sasd/ui/application.hpp>

#include <sasd/ui/events/event_dispatcher.hpp>

#include <variant>

namespace sasd::ui {

Application::Application(Backend& backend) : backend_{backend} {
    backend_.initialize();
}

Application::~Application() {
    backend_.shutdown();
}

std::size_t Application::processEvents(const EventHandler& handler) {
    /*
     * Keep one event-pump implementation. The raw form is simply routed processing without a target
     * resolver, which means every event falls through to the supplied handler. This avoids subtle
     * differences in lifecycle handling between two otherwise equivalent pump loops.
     */
    return processRoutedEvents({}, handler);
}

std::size_t Application::processRoutedEvents(const EventTargetResolver& target_resolver,
                                             const EventHandler& unhandled_handler) {
    std::size_t processed = 0;

    while (auto event = backend_.pollEvent()) {
        ++processed;

        /*
         * Quit is a runtime/application concern rather than a focusable-widget concern. Handling it
         * before target resolution guarantees that a focused control cannot consume the event and
         * prevent the application from observing the shutdown request.
         */
        if (std::holds_alternative<QuitEvent>(*event)) {
            exit_requested_ = true;

            if (unhandled_handler) {
                unhandled_handler(*event);
            }
            continue;
        }

        bool handled = false;

        /*
         * Target selection is injected as policy. Today a test can select an explicit widget;
         * later a focus manager, window manager or pointer hit-test can provide the same answer
         * without teaching Application any of those subsystems.
         */
        if (target_resolver) {
            if (Widget* target = target_resolver(*event)) {
                handled = EventDispatcher::dispatch(*target, *event).handled();
            }
        }

        if (!handled && unhandled_handler) {
            unhandled_handler(*event);
        }
    }

    return processed;
}

} // namespace sasd::ui
