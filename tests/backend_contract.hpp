#pragma once

#include "test_framework.hpp"

#include <sasd/ui/backend/backend.hpp>
#include <sasd/ui/events/event.hpp>

#include <variant>

namespace sasd::ui::tests {

/**
 * Runs the backend-neutral M1 contract against a backend-specific test fixture.
 *
 * The production Backend interface intentionally does not expose test-only controls such as event
 * injection or lifecycle inspection. A fixture bridges that gap without polluting the public API.
 * To participate, a fixture provides:
 *
 *   Backend& backend();
 *   bool isInitialized() const;
 *   void postEvent(Event event);
 *
 * A future terminal, rendered or native backend can therefore use its own injection mechanism while
 * still executing the same semantic contract. Keeping the contract here prevents individual backend
 * tests from slowly developing incompatible assumptions about lifecycle and event ordering.
 */
template <typename Fixture>
void verifyBackendCoreContract(Fixture& fixture) {
    Backend& backend = fixture.backend();

    // Every backend needs a stable human-readable identity for diagnostics and future capability
    // reporting. An empty name would make cross-backend test failures unnecessarily opaque.
    CHECK(!backend.name().empty());

    backend.initialize();
    CHECK(fixture.isInitialized());

    // Capability discovery is part of the backend contract. Repeated reads during one initialized
    // lifetime must not mutate the reported capability set as a side effect of the query itself.
    const auto capabilities = backend.capabilities();
    CHECK(backend.capabilities() == capabilities);

    // With no platform/input events pending, polling is non-blocking and reports an empty result.
    CHECK(!backend.pollEvent().has_value());

    // Fixtures inject already-normalized semantic events. This deliberately tests the common
    // Backend -> Application boundary, not platform-specific key decoding or terminal parsing.
    fixture.postEvent(KeyEvent{Key::enter, true, KeyModifier::control});
    fixture.postEvent(TextInputEvent{"contract"});

    auto first = backend.pollEvent();
    auto second = backend.pollEvent();

    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(std::holds_alternative<KeyEvent>(*first));
    CHECK(std::holds_alternative<TextInputEvent>(*second));
    CHECK(std::get<KeyEvent>(*first).key == Key::enter);
    CHECK(hasModifier(std::get<KeyEvent>(*first).modifiers, KeyModifier::control));
    CHECK(std::get<TextInputEvent>(*second).text == "contract");
    CHECK(!backend.pollEvent().has_value());

    backend.shutdown();
    CHECK(!fixture.isInitialized());
}

} // namespace sasd::ui::tests
