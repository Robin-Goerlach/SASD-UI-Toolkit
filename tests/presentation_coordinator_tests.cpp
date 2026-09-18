#include "test_framework.hpp"

#include <sasd/ui/component.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/presentation/presentation_sink.hpp>
#include <sasd/ui/widget.hpp>

#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace sasd::ui;

namespace {

class VisualProbe final : public Widget {
public:
    void changeVisualState() noexcept {
        invalidateVisual();
    }
};

class NonVisualComponent final : public Component {};

class RecordingPresentationSink final : public PresentationSink {
public:
    using Policy = std::function<PresentationUpdateResult(const Widget&)>;

    explicit RecordingPresentationSink(Policy policy = {})
        : policy_{std::move(policy)} {}

    PresentationUpdateResult synchronize(const Widget& widget) override {
        synchronized_widgets_.push_back(&widget);

        if (policy_) {
            return policy_(widget);
        }
        return PresentationUpdateResult::synchronized;
    }

    [[nodiscard]] const std::vector<const Widget*>& synchronizedWidgets() const noexcept {
        return synchronized_widgets_;
    }

private:
    Policy policy_;
    std::vector<const Widget*> synchronized_widgets_;
};

} // namespace

TEST_CASE("PresentationCoordinator synchronizes a new tree in deterministic preorder") {
    Container root;
    auto& first = root.emplace<VisualProbe>();
    auto& middle = root.emplace<Container>();
    auto& nested = middle.emplace<VisualProbe>();
    auto& last = root.emplace<VisualProbe>();

    RecordingPresentationSink sink;
    const auto result = PresentationCoordinator::synchronize(root, sink);

    CHECK(result.visited == 5);
    CHECK(result.requested == 5);
    CHECK(result.synchronized == 5);
    CHECK(result.deferred == 0);
    CHECK(result.complete());

    CHECK(sink.synchronizedWidgets() ==
          std::vector<const Widget*>({&root, &first, &middle, &nested, &last}));

    CHECK(!root.isVisualUpdatePending());
    CHECK(!first.isVisualUpdatePending());
    CHECK(!middle.isVisualUpdatePending());
    CHECK(!nested.isVisualUpdatePending());
    CHECK(!last.isVisualUpdatePending());
}

TEST_CASE("PresentationCoordinator skips clean widgets but still finds a dirty descendant") {
    Container root;
    auto& child = root.emplace<VisualProbe>();

    RecordingPresentationSink initial_sink;
    (void)PresentationCoordinator::synchronize(root, initial_sink);

    child.changeVisualState();

    // Visual invalidation normally dirties the parent as well. A presentation backend is allowed to
    // acknowledge the parent independently, leaving exactly the descendant pending.
    root.acknowledgeVisualUpdate();
    CHECK(!root.isVisualUpdatePending());
    CHECK(child.isVisualUpdatePending());

    RecordingPresentationSink sink;
    const auto result = PresentationCoordinator::synchronize(root, sink);

    CHECK(result.visited == 2);
    CHECK(result.requested == 1);
    CHECK(result.synchronized == 1);
    CHECK(result.deferred == 0);
    CHECK(sink.synchronizedWidgets() == std::vector<const Widget*>({&child}));
    CHECK(!child.isVisualUpdatePending());
}

TEST_CASE("Deferred updates remain pending and are retried on the next pass") {
    Container root;
    auto& first = root.emplace<VisualProbe>();
    auto& second = root.emplace<VisualProbe>();

    RecordingPresentationSink first_pass_sink{[&](const Widget& widget) {
        if (&widget == &first) {
            return PresentationUpdateResult::deferred;
        }
        return PresentationUpdateResult::synchronized;
    }};

    const auto first_pass = PresentationCoordinator::synchronize(root, first_pass_sink);

    CHECK(first_pass.visited == 3);
    CHECK(first_pass.requested == 3);
    CHECK(first_pass.synchronized == 2);
    CHECK(first_pass.deferred == 1);
    CHECK(!first_pass.complete());

    CHECK(!root.isVisualUpdatePending());
    CHECK(first.isVisualUpdatePending());
    CHECK(!second.isVisualUpdatePending());

    RecordingPresentationSink second_pass_sink;
    const auto second_pass = PresentationCoordinator::synchronize(root, second_pass_sink);

    CHECK(second_pass.visited == 3);
    CHECK(second_pass.requested == 1);
    CHECK(second_pass.synchronized == 1);
    CHECK(second_pass.deferred == 0);
    CHECK(second_pass.complete());
    CHECK(second_pass_sink.synchronizedWidgets() == std::vector<const Widget*>({&first}));
    CHECK(!first.isVisualUpdatePending());
}

TEST_CASE("PresentationCoordinator traverses only visual children") {
    Container root;
    root.emplace<NonVisualComponent>();
    auto& visual = root.emplace<VisualProbe>();

    RecordingPresentationSink sink;
    const auto result = PresentationCoordinator::synchronize(root, sink);

    CHECK(root.componentCount() == 2);
    CHECK(root.childCount() == 1);
    CHECK(result.visited == 2);
    CHECK(result.requested == 2);
    CHECK(sink.synchronizedWidgets() == std::vector<const Widget*>({&root, &visual}));
}

TEST_CASE("Invisible dirty widgets are still presented so their old representation can be removed") {
    VisualProbe widget;

    RecordingPresentationSink initial_sink;
    (void)PresentationCoordinator::synchronize(widget, initial_sink);

    widget.setVisible(false);
    CHECK(widget.isVisualUpdatePending());

    RecordingPresentationSink sink;
    const auto result = PresentationCoordinator::synchronize(widget, sink);

    CHECK(result.visited == 1);
    CHECK(result.requested == 1);
    CHECK(result.synchronized == 1);
    CHECK(!widget.isVisualUpdatePending());
}

TEST_CASE("Presentation sink exceptions acknowledge only earlier successful widgets") {
    Container root;
    auto& first = root.emplace<VisualProbe>();
    auto& second = root.emplace<VisualProbe>();

    RecordingPresentationSink sink{[&](const Widget& widget) -> PresentationUpdateResult {
        if (&widget == &first) {
            throw std::runtime_error("synthetic presentation failure");
        }
        return PresentationUpdateResult::synchronized;
    }};

    bool threw = false;
    try {
        (void)PresentationCoordinator::synchronize(root, sink);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);

    // Root is visited first and was acknowledged. The throwing child remains pending because
    // acknowledgement happens only after a successful sink result. Later siblings were not visited.
    CHECK(!root.isVisualUpdatePending());
    CHECK(first.isVisualUpdatePending());
    CHECK(second.isVisualUpdatePending());
    CHECK(sink.synchronizedWidgets() == std::vector<const Widget*>({&root, &first}));
}

TEST_CASE("A clean tree produces an empty presentation request set") {
    Container root;
    root.emplace<VisualProbe>();

    RecordingPresentationSink initial_sink;
    (void)PresentationCoordinator::synchronize(root, initial_sink);

    RecordingPresentationSink sink;
    const auto result = PresentationCoordinator::synchronize(root, sink);

    CHECK(result.visited == 2);
    CHECK(result.requested == 0);
    CHECK(result.synchronized == 0);
    CHECK(result.deferred == 0);
    CHECK(result.complete());
    CHECK(sink.synchronizedWidgets().empty());
}


TEST_CASE("Subtree refresh replays otherwise clean descendants") {
    Container root;
    auto& child = root.emplace<VisualProbe>();

    RecordingPresentationSink initial_sink;
    (void)PresentationCoordinator::synchronize(root, initial_sink);

    // Geometry change requests a subtree rebuild at the root while child remains otherwise clean.
    root.arrange({0, 0, 20, 5});
    child.acknowledgeVisualUpdate();

    RecordingPresentationSink sink;
    const auto result = PresentationCoordinator::synchronize(root, sink);

    CHECK(result.requested == 2);
    CHECK(result.forced == 1);
    CHECK(result.synchronized == 2);
    CHECK(result.deferred == 0);
    CHECK(sink.synchronizedWidgets() == std::vector<const Widget*>({&root, &child}));
}

TEST_CASE("Deferred forced descendants become normally pending for retry") {
    Container root;
    auto& child = root.emplace<VisualProbe>();

    RecordingPresentationSink initial_sink;
    (void)PresentationCoordinator::synchronize(root, initial_sink);

    root.arrange({0, 0, 20, 5});
    child.acknowledgeVisualUpdate();

    RecordingPresentationSink first_sink{[&](const Widget& widget) {
        if (&widget == &child) {
            return PresentationUpdateResult::deferred;
        }
        return PresentationUpdateResult::synchronized;
    }};

    const auto first = PresentationCoordinator::synchronize(root, first_sink);
    CHECK(first.forced == 1);
    CHECK(first.deferred == 1);
    CHECK(child.isVisualUpdatePending());

    RecordingPresentationSink retry_sink;
    const auto retry = PresentationCoordinator::synchronize(root, retry_sink);
    CHECK(retry.synchronized >= 1);
    CHECK(!child.isVisualUpdatePending());
}

TEST_CASE("Deferred subtree refresh root blocks descendant replay") {
    Container root;
    auto& child = root.emplace<VisualProbe>();

    RecordingPresentationSink initial_sink;
    (void)PresentationCoordinator::synchronize(root, initial_sink);

    root.arrange({0, 0, 10, 3});

    RecordingPresentationSink sink{[&](const Widget& widget) {
        if (&widget == &root) {
            return PresentationUpdateResult::deferred;
        }
        return PresentationUpdateResult::synchronized;
    }};

    const auto result = PresentationCoordinator::synchronize(root, sink);

    CHECK(result.requested == 1);
    CHECK(result.deferred == 1);
    CHECK(sink.synchronizedWidgets() == std::vector<const Widget*>({&root}));
    CHECK(child.isVisualUpdatePending() == false);
    CHECK(root.isSubtreeRefreshPending());
}
