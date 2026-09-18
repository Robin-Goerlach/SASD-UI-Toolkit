#include "test_framework.hpp"

#include <sasd/ui/container.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>

#include <memory>
#include <variant>
#include <vector>

using namespace sasd::ui;

namespace {

class RecordingWidget final : public Widget {
public:
    RecordingWidget(std::vector<int>& route, int id, EventResult result)
        : route_{route}, id_{id}, result_{result} {}

    [[nodiscard]] bool sawExpectedKey() const noexcept {
        return saw_expected_key_;
    }

protected:
    EventResult onEvent(const Event& event) override {
        route_.push_back(id_);

        if (const auto* key = std::get_if<KeyEvent>(&event)) {
            saw_expected_key_ =
                key->key == Key::enter && hasModifier(key->modifiers, KeyModifier::control);
        }

        return result_;
    }

private:
    std::vector<int>& route_;
    int id_;
    EventResult result_;
    bool saw_expected_key_{false};
};

class RecordingContainer final : public Container {
public:
    RecordingContainer(std::vector<int>& route, int id, EventResult result)
        : route_{route}, id_{id}, result_{result} {}

protected:
    EventResult onEvent(const Event&) override {
        route_.push_back(id_);
        return result_;
    }

private:
    std::vector<int>& route_;
    int id_;
    EventResult result_;
};

} // namespace

TEST_CASE("EventDispatcher stops when the target handles the event") {
    std::vector<int> route;
    RecordingContainer root{route, 1, EventResult::ignored};
    auto& target = root.emplace<RecordingWidget>(route, 2, EventResult::handled);

    const Event event = KeyEvent{Key::enter, true, KeyModifier::control};
    const auto result = EventDispatcher::dispatch(target, event);

    CHECK(result.handled());
    CHECK(result.handler == &target);
    CHECK(result.visited == 1);
    CHECK(route == std::vector<int>{2});
    CHECK(target.sawExpectedKey());
}

TEST_CASE("EventDispatcher bubbles ignored events through visual parents") {
    std::vector<int> route;
    RecordingContainer root{route, 1, EventResult::ignored};
    auto& middle = root.emplace<RecordingContainer>(route, 2, EventResult::handled);
    auto& target = middle.emplace<RecordingWidget>(route, 3, EventResult::ignored);

    const Event event = TextInputEvent{"hello"};
    const auto result = EventDispatcher::dispatch(target, event);

    CHECK(result.handled());
    CHECK(result.handler == &middle);
    CHECK(result.visited == 2);
    CHECK(route == std::vector<int>{3, 2});
}

TEST_CASE("EventDispatcher reports an event that reaches the root unhandled") {
    std::vector<int> route;
    RecordingContainer root{route, 1, EventResult::ignored};
    auto& middle = root.emplace<RecordingContainer>(route, 2, EventResult::ignored);
    auto& target = middle.emplace<RecordingWidget>(route, 3, EventResult::ignored);

    const Event event = FocusEvent{true};
    const auto result = EventDispatcher::dispatch(target, event);

    CHECK(!result.handled());
    CHECK(result.handler == nullptr);
    CHECK(result.visited == 3);
    CHECK(route == std::vector<int>{3, 2, 1});
}

TEST_CASE("Released widgets no longer route through their former visual parent") {
    std::vector<int> route;
    RecordingContainer root{route, 1, EventResult::handled};
    auto& target = root.emplace<RecordingWidget>(route, 2, EventResult::ignored);

    std::unique_ptr<Component> released = root.release(target);
    CHECK(released.get() == &target);
    CHECK(target.parent() == nullptr);

    const Event event = ResizeEvent{{80, 25}};
    const auto result = EventDispatcher::dispatch(target, event);

    CHECK(!result.handled());
    CHECK(result.visited == 1);
    CHECK(route == std::vector<int>{2});
}

TEST_CASE("EventDispatcher preserves the semantic event object during bubbling") {
    std::vector<int> route;
    RecordingContainer root{route, 1, EventResult::ignored};
    auto& target = root.emplace<RecordingWidget>(route, 2, EventResult::ignored);

    const Event event = KeyEvent{Key::enter, true, KeyModifier::control};
    const auto result = EventDispatcher::dispatch(target, event);

    CHECK(!result.handled());
    CHECK(target.sawExpectedKey());
    CHECK(route == std::vector<int>{2, 1});
}
