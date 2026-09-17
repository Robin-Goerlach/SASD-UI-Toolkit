#include "test_framework.hpp"

#include <sasd/ui/application.hpp>
#include <sasd/ui/testing/mock_backend.hpp>

#include <cstddef>
#include <variant>

using namespace sasd::ui;
using sasd::ui::testing::MockBackend;

TEST_CASE("Application owns backend runtime lifecycle without owning the backend object") {
    MockBackend backend;

    {
        Application application{backend};
        CHECK(backend.isInitialized());
        CHECK(backend.initializeCount() == 1);
        CHECK(&application.backend() == &backend);
    }

    CHECK(!backend.isInitialized());
    CHECK(backend.shutdownCount() == 1);
}

TEST_CASE("Application drains backend events and records quit requests") {
    MockBackend backend;
    Application application{backend};

    backend.postEvent(KeyEvent{Key::enter, true, KeyModifier::none});
    backend.postEvent(TextInputEvent{"x"});
    backend.postEvent(QuitEvent{});

    std::size_t handled = 0;
    bool saw_text = false;

    const auto processed = application.processEvents([&](const Event& event) {
        ++handled;
        if (std::holds_alternative<TextInputEvent>(event)) {
            saw_text = true;
        }
    });

    CHECK(processed == 3);
    CHECK(handled == 3);
    CHECK(saw_text);
    CHECK(application.exitRequested());
    CHECK(backend.pendingEventCount() == 0);
}
