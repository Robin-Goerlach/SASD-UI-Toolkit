#include "test_framework.hpp"

#include <sasd/ui/testing/mock_backend.hpp>

#include <stdexcept>
#include <variant>

using namespace sasd::ui;
using sasd::ui::testing::MockBackend;

TEST_CASE("MockBackend exposes configured capabilities") {
    BackendCapabilities capabilities{};
    capabilities.pointer_input = true;
    capabilities.clipboard = true;

    MockBackend backend{capabilities};

    CHECK(backend.name() == "mock");
    CHECK(backend.capabilities().pointer_input);
    CHECK(backend.capabilities().clipboard);
    CHECK(!backend.capabilities().printing);
}

TEST_CASE("MockBackend lifecycle is deterministic") {
    MockBackend backend;

    backend.initialize();
    CHECK(backend.isInitialized());
    CHECK(backend.initializeCount() == 1);

    bool threw = false;
    try {
        backend.initialize();
    } catch (const std::logic_error&) {
        threw = true;
    }
    CHECK(threw);

    backend.shutdown();
    backend.shutdown();
    CHECK(!backend.isInitialized());
    CHECK(backend.shutdownCount() == 1);
}

TEST_CASE("MockBackend delivers posted events in order") {
    MockBackend backend;
    backend.postEvent(FocusEvent{true});
    backend.postEvent(QuitEvent{});

    CHECK(backend.pendingEventCount() == 2);
    auto first = backend.pollEvent();
    auto second = backend.pollEvent();

    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(std::holds_alternative<FocusEvent>(*first));
    CHECK(std::holds_alternative<QuitEvent>(*second));
}
