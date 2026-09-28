#include "test_framework.hpp"

#include <sasd/ui/container.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/widget.hpp>

#include <memory>
#include <vector>

using namespace sasd::ui;

namespace {

class PointerProbe final : public Widget {
public:
    explicit PointerProbe(bool handle = true) : handle_{handle} {}

    [[nodiscard]] const std::vector<PointerEvent>& events() const noexcept {
        return events_;
    }

    [[nodiscard]] bool captureLost() const noexcept { return capture_lost_; }

protected:
    EventResult onEvent(const Event& event) override {
        if (const auto* pointer = std::get_if<PointerEvent>(&event)) {
            events_.push_back(*pointer);
            return handle_ ? EventResult::handled : EventResult::ignored;
        }
        return EventResult::ignored;
    }

    void onPointerCaptureLost() noexcept override {
        capture_lost_ = true;
    }

private:
    bool handle_;
    bool capture_lost_{false};
    std::vector<PointerEvent> events_;
};

class PointerHandlingContainer final : public Container {
protected:
    EventResult onEvent(const Event& event) override {
        return std::holds_alternative<PointerEvent>(event)
                   ? EventResult::handled
                   : EventResult::ignored;
    }
};

} // namespace

TEST_CASE("PointerRouter hit-tests the initial target and captures a handled press") {
    Container root;
    root.arrange({0, 0, 200, 100});

    auto& target = root.emplace<PointerProbe>();
    target.arrange({20, 10, 50, 30});

    PointerRouter router;

    const auto press = router.route(
        root,
        PointerEvent{{25, 15}, PointerAction::press, PointerButton::primary, 1});

    CHECK(press.targeted);
    CHECK(press.handled);
    CHECK(press.visited == 1);
    CHECK(press.capture_active);
    CHECK(router.capturedWidget() == &target);
}

TEST_CASE("PointerRouter captures the Widget that handled a bubbled press") {
    PointerHandlingContainer root;
    root.arrange({0, 0, 100, 100});

    auto& child = root.emplace<PointerProbe>(false);
    child.arrange({10, 10, 40, 40});

    PointerRouter router;
    const auto press = router.route(
        root,
        PointerEvent{{20, 20}, PointerAction::press, PointerButton::primary, 1});

    CHECK(press.targeted);
    CHECK(press.handled);
    CHECK(press.visited == 2);
    CHECK(router.capturedWidget() == &root);
}

TEST_CASE("PointerRouter keeps motion and matching release on the captured press target") {
    Container root;
    root.arrange({0, 0, 200, 100});

    auto& target = root.emplace<PointerProbe>();
    target.arrange({20, 10, 50, 30});

    PointerRouter router;

    CHECK(router.route(
        root,
        PointerEvent{{25, 15}, PointerAction::press, PointerButton::primary, 1}).handled);

    const auto move = router.route(
        root,
        PointerEvent{{180, 90}, PointerAction::move, PointerButton::none, 0});
    CHECK(move.handled);
    CHECK(move.capture_active);

    const auto release = router.route(
        root,
        PointerEvent{{180, 90}, PointerAction::release, PointerButton::primary, 1});
    CHECK(release.handled);
    CHECK(!release.capture_active);
    CHECK(!router.hasCapture());

    CHECK(target.events().size() == 3);
    CHECK(target.events()[1].position == Point{180, 90});
    CHECK(target.events()[2].action == PointerAction::release);
}

TEST_CASE("PointerRouter drops speculative capture when the press route is ignored") {
    Container root;
    root.arrange({0, 0, 100, 100});

    auto& target = root.emplace<PointerProbe>(false);
    target.arrange({0, 0, 50, 50});

    PointerRouter router;
    const auto result = router.route(
        root,
        PointerEvent{{10, 10}, PointerAction::press, PointerButton::primary, 1});

    CHECK(result.targeted);
    CHECK(!result.handled);
    CHECK(!result.capture_active);
    CHECK(!router.hasCapture());
}

TEST_CASE("PointerRouter clears capture when a captured Widget is destroyed") {
    auto root = std::make_unique<Container>();
    root->arrange({0, 0, 100, 100});

    auto& target = root->emplace<PointerProbe>();
    target.arrange({0, 0, 50, 50});

    PointerRouter router;
    CHECK(router.route(
        *root,
        PointerEvent{{10, 10}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(router.hasCapture());

    auto released = root->release(target);
    CHECK(released != nullptr);

    // Destruction handshake must invalidate the router's non-owning observation.
    released.reset();
    CHECK(!router.hasCapture());
    CHECK(router.capturedWidget() == nullptr);
}

TEST_CASE("PointerRouter never keeps routing to a Widget detached from the supplied root") {
    Container root;
    root.arrange({0, 0, 100, 100});

    auto& target = root.emplace<PointerProbe>();
    target.arrange({0, 0, 50, 50});

    PointerRouter router;
    CHECK(router.route(
        root,
        PointerEvent{{10, 10}, PointerAction::press, PointerButton::primary, 1}).handled);

    auto released = root.release(target);
    CHECK(released != nullptr);

    const auto move = router.route(
        root,
        PointerEvent{{90, 90}, PointerAction::move, PointerButton::none, 0});

    CHECK(!router.hasCapture());
    CHECK(target.captureLost());
    CHECK(move.targeted); // the root itself is now the geometric target
    CHECK(!move.handled);
}

TEST_CASE("PointerRouter routes uncaptured motion by current hit-test result") {
    Container root;
    root.arrange({0, 0, 100, 100});

    auto& first = root.emplace<PointerProbe>();
    first.arrange({0, 0, 40, 40});

    auto& second = root.emplace<PointerProbe>();
    second.arrange({50, 0, 40, 40});

    PointerRouter router;

    CHECK(router.route(
        root,
        PointerEvent{{10, 10}, PointerAction::move, PointerButton::none, 0}).handled);
    CHECK(first.events().size() == 1);
    CHECK(second.events().empty());

    CHECK(router.route(
        root,
        PointerEvent{{60, 10}, PointerAction::move, PointerButton::none, 0}).handled);
    CHECK(second.events().size() == 1);
}
