#include "test_framework.hpp"

#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/widget.hpp>

using namespace sasd::ui;

namespace {

/**
 * Test-only widget exposing the protected invalidation operation.
 *
 * Production widgets use invalidateVisual() from state-changing setters such as future Button
 * pressed/hovered state or Label content changes. The public API exposes observation/acknowledgement
 * to presentation coordinators, not arbitrary dirty-state mutation.
 */
class VisualProbe final : public Widget {
public:
    void changeVisualOnlyState() noexcept {
        invalidateVisual();
    }
};

} // namespace

TEST_CASE("New widgets require an initial visual synchronization") {
    Widget widget;

    CHECK(widget.isVisualUpdatePending());

    widget.acknowledgeVisualUpdate();
    CHECK(!widget.isVisualUpdatePending());
}

TEST_CASE("Visual-only state changes do not invalidate measurement") {
    VisualProbe widget;
    const MeasureConstraints constraints{{0, 0}, {100, 100}};

    (void)widget.measure(constraints);
    widget.acknowledgeVisualUpdate();

    CHECK(widget.isMeasureValid());
    CHECK(!widget.isVisualUpdatePending());

    widget.changeVisualOnlyState();

    CHECK(widget.isMeasureValid());
    CHECK(widget.isVisualUpdatePending());
}

TEST_CASE("Visual invalidation propagates through visual parents") {
    Container root;
    auto& middle = root.emplace<Container>();
    auto& child = middle.emplace<VisualProbe>();

    // Adoption itself is visually relevant, so establish a clean baseline first.
    root.acknowledgeVisualUpdate();
    middle.acknowledgeVisualUpdate();
    child.acknowledgeVisualUpdate();

    child.changeVisualOnlyState();

    CHECK(child.isVisualUpdatePending());
    CHECK(middle.isVisualUpdatePending());
    CHECK(root.isVisualUpdatePending());
}

TEST_CASE("Acknowledging an ancestor does not silently acknowledge dirty descendants") {
    Container root;
    auto& child = root.emplace<VisualProbe>();

    root.acknowledgeVisualUpdate();
    child.acknowledgeVisualUpdate();

    child.changeVisualOnlyState();
    CHECK(root.isVisualUpdatePending());
    CHECK(child.isVisualUpdatePending());

    // A renderer may update/acknowledge an ancestor separately. The child remains pending until its
    // own presentation state has actually been synchronized.
    root.acknowledgeVisualUpdate();
    CHECK(!root.isVisualUpdatePending());
    CHECK(child.isVisualUpdatePending());

    // invalidateVisual() always re-propagates, even if the child was already dirty.
    child.changeVisualOnlyState();
    CHECK(root.isVisualUpdatePending());
    CHECK(child.isVisualUpdatePending());
}

TEST_CASE("Enabled state invalidates visuals without forcing re-measurement") {
    Widget widget;
    (void)widget.measure();
    widget.acknowledgeVisualUpdate();

    widget.setEnabled(false);

    CHECK(widget.isMeasureValid());
    CHECK(widget.isVisualUpdatePending());
}

TEST_CASE("Visibility invalidates both layout measurement and visuals") {
    Widget widget;
    (void)widget.measure();
    widget.acknowledgeVisualUpdate();

    widget.setVisible(false);

    CHECK(!widget.isMeasureValid());
    CHECK(widget.isVisualUpdatePending());
}

TEST_CASE("Focus transitions invalidate visuals but preserve measurement") {
    FocusManager focus;
    Widget first;
    Widget second;

    first.setFocusable(true);
    second.setFocusable(true);

    (void)first.measure();
    (void)second.measure();
    first.acknowledgeVisualUpdate();
    second.acknowledgeVisualUpdate();

    CHECK(focus.requestFocus(first));

    CHECK(first.hasFocus());
    CHECK(first.isVisualUpdatePending());
    CHECK(first.isMeasureValid());
    CHECK(second.isMeasureValid());

    first.acknowledgeVisualUpdate();
    CHECK(focus.requestFocus(second));

    CHECK(!first.hasFocus());
    CHECK(second.hasFocus());
    CHECK(first.isVisualUpdatePending());
    CHECK(second.isVisualUpdatePending());
    CHECK(first.isMeasureValid());
    CHECK(second.isMeasureValid());
}

TEST_CASE("Geometry changes invalidate visuals without invalidating measured size") {
    Widget widget;
    (void)widget.measure();
    widget.arrange({0, 0, 20, 10});
    widget.acknowledgeVisualUpdate();

    CHECK(widget.isMeasureValid());

    widget.arrange({5, 7, 20, 10});

    CHECK(widget.isVisualUpdatePending());
    CHECK(widget.isMeasureValid());

    widget.acknowledgeVisualUpdate();

    // Re-arranging to exactly the same rectangle is not a new visual state change by itself.
    widget.arrange({5, 7, 20, 10});
    CHECK(!widget.isVisualUpdatePending());
}

TEST_CASE("Visual child insertion and removal invalidate the container presentation") {
    Container root;
    root.acknowledgeVisualUpdate();

    auto& child = root.emplace<Widget>();
    CHECK(root.isVisualUpdatePending());

    root.acknowledgeVisualUpdate();
    auto released = root.release(child);

    CHECK(released != nullptr);
    CHECK(root.isVisualUpdatePending());
}
