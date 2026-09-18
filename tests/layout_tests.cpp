#include "test_framework.hpp"

#include <sasd/ui/container.hpp>
#include <sasd/ui/widget.hpp>

#include <stdexcept>

using namespace sasd::ui;

namespace {

class IntrinsicWidget final : public Widget {
public:
    explicit IntrinsicWidget(Size intrinsic) : intrinsic_{intrinsic} {}

    void setIntrinsic(Size intrinsic) {
        if (intrinsic_ == intrinsic) {
            return;
        }

        intrinsic_ = intrinsic;
        invalidateMeasure();
    }

    [[nodiscard]] int measureCalls() const noexcept {
        return measure_calls_;
    }

protected:
    Size onMeasure(const MeasureConstraints&) override {
        ++measure_calls_;
        return intrinsic_;
    }

private:
    Size intrinsic_{};
    int measure_calls_{0};
};

class ChildMeasuringContainer final : public Container {
public:
    void setMeasuredChild(Widget& child) noexcept {
        child_ = &child;
    }

    [[nodiscard]] int measureCalls() const noexcept {
        return measure_calls_;
    }

protected:
    Size onMeasure(const MeasureConstraints& constraints) override {
        ++measure_calls_;
        if (child_ == nullptr) {
            return {};
        }

        return child_->measure(constraints);
    }

private:
    Widget* child_{nullptr};
    int measure_calls_{0};
};

class ArrangeProbe final : public Widget {
public:
    [[nodiscard]] int arrangeCalls() const noexcept {
        return arrange_calls_;
    }

    [[nodiscard]] Rect observedBounds() const noexcept {
        return observed_bounds_;
    }

protected:
    void onArrange(Rect) override {
        ++arrange_calls_;

        // The public contract promises that bounds() already contains the final rectangle while the
        // hook runs. A container can therefore use its own final geometry to lay out children.
        observed_bounds_ = bounds();
    }

private:
    int arrange_calls_{0};
    Rect observed_bounds_{};
};

} // namespace

TEST_CASE("MeasureConstraints validate and clamp parent-provided layout space") {
    const MeasureConstraints constraints{{10, 5}, {100, 50}};

    CHECK(constraints.hasValidRange());
    CHECK(constraints.clamp({5, 100}) == Size{10, 50});
    CHECK(constraints.clamp({25, 20}) == Size{25, 20});

    const MeasureConstraints negative{{-1, 0}, {100, 50}};
    const MeasureConstraints reversed{{100, 10}, {50, 5}};

    CHECK(!negative.hasValidRange());
    CHECK(!reversed.hasValidRange());

    // clamp() remains deterministic even for malformed values, while Widget::measure() rejects them.
    CHECK(reversed.clamp({75, 7}) == Size{100, 10});
}

TEST_CASE("Widget measure combines intrinsic hints with parent constraints") {
    Widget widget;
    widget.setSizeConstraints({{10, 5}, {40, 20}, {100, 50}});

    CHECK(widget.measure({{0, 0}, {30, 100}}) == Size{30, 20});
    CHECK(widget.desiredSize() == Size{30, 20});
    CHECK(widget.isMeasureValid());

    /*
     * The widget's own minimum is 10x5, but the parent is authoritative about available space. This
     * matters in narrow terminals/windows where a child cannot force the parent to create space that
     * does not exist.
     */
    CHECK(widget.measure({{0, 0}, {8, 4}}) == Size{8, 4});
}

TEST_CASE("Widget rejects contradictory measurement constraints") {
    Widget widget;
    bool threw = false;

    try {
        (void)widget.measure({{20, 10}, {10, 5}});
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(!widget.isMeasureValid());
}

TEST_CASE("Widget caches measurement until intrinsic state invalidates it") {
    IntrinsicWidget widget{{25, 12}};
    const MeasureConstraints constraints{{0, 0}, {100, 100}};

    CHECK(widget.measure(constraints) == Size{25, 12});
    CHECK(widget.measureCalls() == 1);

    // Same state + same parent constraints must use the cached desired size.
    CHECK(widget.measure(constraints) == Size{25, 12});
    CHECK(widget.measureCalls() == 1);

    widget.setIntrinsic({40, 18});
    CHECK(!widget.isMeasureValid());

    CHECK(widget.measure(constraints) == Size{40, 18});
    CHECK(widget.measureCalls() == 2);

    // A different parent constraint interval is a different cache key and requires re-measurement.
    CHECK(widget.measure({{0, 0}, {30, 100}}) == Size{30, 18});
    CHECK(widget.measureCalls() == 3);
}

TEST_CASE("Child measurement invalidation propagates through the visual parent tree") {
    ChildMeasuringContainer root;
    auto& middle = root.emplace<ChildMeasuringContainer>();
    auto& child = middle.emplace<IntrinsicWidget>(Size{20, 10});

    root.setMeasuredChild(middle);
    middle.setMeasuredChild(child);

    const MeasureConstraints constraints{{0, 0}, {100, 100}};
    CHECK(root.measure(constraints) == Size{20, 10});
    CHECK(root.isMeasureValid());
    CHECK(middle.isMeasureValid());
    CHECK(child.isMeasureValid());

    child.setIntrinsic({35, 15});

    CHECK(!child.isMeasureValid());
    CHECK(!middle.isMeasureValid());
    CHECK(!root.isMeasureValid());

    CHECK(root.measure(constraints) == Size{35, 15});
    CHECK(root.measureCalls() == 2);
    CHECK(middle.measureCalls() == 2);
    CHECK(child.measureCalls() == 2);
}

TEST_CASE("Container visual structure changes invalidate cached measurement") {
    Container root;
    const MeasureConstraints constraints{};

    CHECK(root.measure(constraints) == Size{0, 0});
    CHECK(root.isMeasureValid());

    auto& child = root.emplace<Widget>();
    CHECK(!root.isMeasureValid());

    CHECK(root.measure(constraints) == Size{0, 0});
    CHECK(root.isMeasureValid());

    auto released = root.release(child);
    CHECK(released != nullptr);
    CHECK(!root.isMeasureValid());
}

TEST_CASE("Arrange stores final bounds before invoking the widget hook") {
    ArrangeProbe widget;

    widget.arrange({10, 20, 80, 25});

    CHECK(widget.bounds() == Rect{10, 20, 80, 25});
    CHECK(widget.observedBounds() == Rect{10, 20, 80, 25});
    CHECK(widget.arrangeCalls() == 1);

    // setBounds intentionally shares the same arrangement path instead of bypassing onArrange().
    widget.setBounds({1, 2, 30, 40});
    CHECK(widget.bounds() == Rect{1, 2, 30, 40});
    CHECK(widget.observedBounds() == Rect{1, 2, 30, 40});
    CHECK(widget.arrangeCalls() == 2);
}

TEST_CASE("Arrange rejects negative extents without changing existing bounds") {
    ArrangeProbe widget;
    widget.arrange({1, 2, 30, 40});

    bool threw = false;
    try {
        widget.arrange({5, 6, -1, 10});
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(widget.bounds() == Rect{1, 2, 30, 40});
    CHECK(widget.arrangeCalls() == 1);
}

TEST_CASE("Widget rejects invalid intrinsic size ranges") {
    Widget widget;
    bool threw = false;

    try {
        widget.setSizeConstraints({{100, 10}, {75, 7}, {50, 5}});
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(widget.sizeConstraints() == SizeConstraints{});
}
