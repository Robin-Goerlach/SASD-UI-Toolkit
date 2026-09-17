#include "backend_contract.hpp"
#include "test_framework.hpp"

#include <sasd/ui/testing/mock_backend.hpp>

#include <stdexcept>
#include <utility>
#include <variant>

using namespace sasd::ui;
using sasd::ui::testing::MockBackend;

namespace {

/**
 * Adapts MockBackend to the reusable backend contract without adding test hooks to Backend itself.
 *
 * Production backends can later provide equivalent fixtures backed by synthetic platform messages,
 * pseudo-terminal input or renderer test hooks while the common contract remains unchanged.
 */
class MockBackendContractFixture {
public:
    [[nodiscard]] Backend& backend() noexcept { return backend_; }
    [[nodiscard]] bool isInitialized() const noexcept { return backend_.isInitialized(); }

    void postEvent(Event event) {
        backend_.postEvent(std::move(event));
    }

private:
    MockBackend backend_;
};

} // namespace

TEST_CASE("MockBackend satisfies the reusable M1 backend core contract") {
    MockBackendContractFixture fixture;
    sasd::ui::tests::verifyBackendCoreContract(fixture);
}

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
