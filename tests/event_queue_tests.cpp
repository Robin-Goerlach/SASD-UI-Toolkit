#include "test_framework.hpp"

#include <sasd/ui/events/event_queue.hpp>

#include <variant>

using namespace sasd::ui;

TEST_CASE("EventQueue preserves FIFO order") {
    EventQueue queue;
    queue.push(KeyEvent{Key::enter, true, KeyModifier::none});
    queue.push(TextInputEvent{"hello"});

    CHECK(queue.size() == 2);

    auto first = queue.tryPop();
    auto second = queue.tryPop();

    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(std::holds_alternative<KeyEvent>(*first));
    CHECK(std::holds_alternative<TextInputEvent>(*second));
    CHECK(queue.empty());
}

TEST_CASE("Key modifiers can be combined without backend-specific constants") {
    const auto modifiers = KeyModifier::control | KeyModifier::shift;

    CHECK(hasModifier(modifiers, KeyModifier::control));
    CHECK(hasModifier(modifiers, KeyModifier::shift));
    CHECK(!hasModifier(modifiers, KeyModifier::alt));
}
