#include "test_framework.hpp"

#include <sasd/ui/application.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/testing/mock_backend.hpp>

#include <cstddef>
#include <variant>

using namespace sasd::ui;
using sasd::ui::testing::MockBackend;

namespace {

/**
 * Small test widget that consumes key events and deliberately ignores other semantic events.
 *
 * It models the behavior a future Button/TextField-style control can provide through onEvent()
 * without coupling Application tests to any concrete user-facing widget that does not exist yet.
 */
class KeyHandlingWidget final : public Widget {
public:
    [[nodiscard]] std::size_t deliveryCount() const noexcept {
        return delivery_count_;
    }

    [[nodiscard]] std::size_t handledKeyCount() const noexcept {
        return handled_key_count_;
    }

protected:
    EventResult onEvent(const Event& event) override {
        ++delivery_count_;

        if (std::holds_alternative<KeyEvent>(event)) {
            ++handled_key_count_;
            return EventResult::handled;
        }

        return EventResult::ignored;
    }

private:
    std::size_t delivery_count_{0};
    std::size_t handled_key_count_{0};
};

} // namespace

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

TEST_CASE("Application routes non-quit events through resolved widget targets") {
    MockBackend backend;
    Application application{backend};
    Container root;
    auto& target = root.emplace<KeyHandlingWidget>();

    backend.postEvent(KeyEvent{Key::enter, true, KeyModifier::control});
    backend.postEvent(TextInputEvent{"not handled by the test widget"});
    backend.postEvent(QuitEvent{});

    std::size_t resolver_calls = 0;
    std::size_t unhandled_calls = 0;
    bool saw_unhandled_text = false;
    bool saw_quit = false;

    const auto processed = application.processRoutedEvents(
        [&](const Event&) -> Widget* {
            ++resolver_calls;
            return &target;
        },
        [&](const Event& event) {
            ++unhandled_calls;
            saw_unhandled_text = saw_unhandled_text || std::holds_alternative<TextInputEvent>(event);
            saw_quit = saw_quit || std::holds_alternative<QuitEvent>(event);
        });

    CHECK(processed == 3);
    CHECK(resolver_calls == 2);
    CHECK(target.deliveryCount() == 2);
    CHECK(target.handledKeyCount() == 1);

    // KeyEvent was consumed by the target. TextInputEvent reached the root unhandled, and QuitEvent
    // deliberately bypassed target resolution because it belongs to Application lifecycle semantics.
    CHECK(unhandled_calls == 2);
    CHECK(saw_unhandled_text);
    CHECK(saw_quit);
    CHECK(application.exitRequested());
    CHECK(backend.pendingEventCount() == 0);
}

TEST_CASE("Application reports routed events as unhandled when no target is resolved") {
    MockBackend backend;
    Application application{backend};

    backend.postEvent(FocusEvent{true});

    std::size_t resolver_calls = 0;
    std::size_t unhandled_calls = 0;

    const auto processed = application.processRoutedEvents(
        [&](const Event&) -> Widget* {
            ++resolver_calls;
            return nullptr;
        },
        [&](const Event&) {
            ++unhandled_calls;
        });

    CHECK(processed == 1);
    CHECK(resolver_calls == 1);
    CHECK(unhandled_calls == 1);
}
