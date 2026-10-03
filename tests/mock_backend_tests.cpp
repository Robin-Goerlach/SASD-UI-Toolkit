#include "backend_contract.hpp"
#include "test_framework.hpp"

#include <sasd/ui/clipboard.hpp>
#include <sasd/ui/testing/memory_clipboard.hpp>
#include <sasd/ui/testing/mock_backend.hpp>

#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

using namespace sasd::ui;
using sasd::ui::testing::MemoryClipboard;
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

TEST_CASE("MemoryClipboard distinguishes absent text from an empty payload") {
    MemoryClipboard clipboard;

    CHECK(!clipboard.readText().has_value());

    clipboard.writeText("");
    const auto empty = clipboard.readText();
    CHECK(empty.has_value());
    CHECK(empty->empty());

    clipboard.clear();
    CHECK(!clipboard.readText().has_value());
}

TEST_CASE("MemoryClipboard owns written and returned text independently") {
    MemoryClipboard clipboard;
    std::string source{"alpha"};

    clipboard.writeText(source);
    source.assign("changed outside clipboard");

    auto first_read = clipboard.readText();
    CHECK(first_read.has_value());
    CHECK(*first_read == "alpha");

    /*
     * readText() returns an owned value rather than a view into backend storage. Mutating the caller's
     * copy must therefore have no effect on the retained clipboard payload. This is the same lifetime
     * rule a native backend must preserve when its OS API exposes temporary or locked memory.
     */
    first_read->assign("caller mutation");

    const auto second_read = clipboard.readText();
    CHECK(second_read.has_value());
    CHECK(*second_read == "alpha");
}

TEST_CASE("Backend clipboard discovery defaults to unsupported") {
    MockBackend backend;

    CHECK(!backend.capabilities().clipboard);
    CHECK(backend.clipboard() == nullptr);

    const MockBackend& const_backend = backend;
    CHECK(const_backend.clipboard() == nullptr);
}

TEST_CASE("MockBackend exposes a lifetime-owned clipboard when capability is enabled") {
    BackendCapabilities capabilities{};
    capabilities.clipboard = true;
    MockBackend backend{capabilities};

    Clipboard* clipboard = backend.clipboard();
    CHECK(clipboard != nullptr);
    CHECK(backend.capabilities().clipboard);

    clipboard->writeText("shared service");

    /*
     * Backend owns the service and repeated discovery returns the same semantic clipboard state. The
     * application borrows the pointer; it does not own or replace the backend's platform service.
     */
    const MockBackend& const_backend = backend;
    const Clipboard* const_clipboard = const_backend.clipboard();
    CHECK(const_clipboard != nullptr);

    const auto text = const_clipboard->readText();
    CHECK(text.has_value());
    CHECK(*text == "shared service");
}
