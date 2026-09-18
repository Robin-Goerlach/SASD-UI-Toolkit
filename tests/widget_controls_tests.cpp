#include "test_framework.hpp"

#include <sasd/ui/label.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/window.hpp>

#include <cstdint>
#include <string_view>

using namespace sasd::ui;

namespace {

class CountingMeasurementContext : public MeasurementContext {
public:
    explicit CountingMeasurementContext(Size measured) : measured_{measured} {}

    void setMeasured(Size measured) noexcept {
        measured_ = measured;
        ++revision_;
    }

    Size measureText(std::string_view) const override {
        ++calls_;
        return measured_;
    }

    std::uint64_t revision() const noexcept override {
        return revision_;
    }

    [[nodiscard]] int calls() const noexcept {
        return calls_;
    }

private:
    Size measured_{};
    std::uint64_t revision_{0};
    mutable int calls_{0};
};

class AlternateMeasurementContext final : public MeasurementContext {
public:
    Size measureText(std::string_view) const override {
        ++calls_;
        return {7, 2};
    }

    std::uint64_t revision() const noexcept override { return 0; }
    [[nodiscard]] int calls() const noexcept { return calls_; }

private:
    mutable int calls_{0};
};

} // namespace

TEST_CASE("Label text changes invalidate measurement and presentation") {
    Label label{"first"};

    (void)label.measure();
    label.acknowledgeVisualUpdate();

    CHECK(label.isMeasureValid());
    CHECK(!label.isVisualUpdatePending());
    CHECK(label.text() == "first");

    label.setText("second");

    CHECK(label.text() == "second");
    CHECK(!label.isMeasureValid());
    CHECK(label.isVisualUpdatePending());
    CHECK(!label.isFocusable());
}

TEST_CASE("Assigning identical Label text preserves cached state") {
    Label label{"same"};

    (void)label.measure();
    label.acknowledgeVisualUpdate();

    label.setText("same");

    CHECK(label.isMeasureValid());
    CHECK(!label.isVisualUpdatePending());
}

TEST_CASE("Label uses MeasurementContext without embedding backend metrics") {
    Label label{"context measured"};
    CountingMeasurementContext context{{14, 3}};

    CHECK(label.measure(context) == Size{14, 3});
    CHECK(context.calls() == 1);

    // Identical dynamic context type + revision + constraints reuses the Widget measurement cache.
    CHECK(label.measure(context) == Size{14, 3});
    CHECK(context.calls() == 1);
}

TEST_CASE("MeasurementContext revision invalidates Widget measurement cache") {
    Label label{"revision"};
    CountingMeasurementContext context{{8, 1}};

    CHECK(label.measure(context) == Size{8, 1});
    CHECK(context.calls() == 1);

    context.setMeasured({12, 2});

    CHECK(label.measure(context) == Size{12, 2});
    CHECK(context.calls() == 2);
}

TEST_CASE("Different MeasurementContext types occupy different cache domains") {
    Label label{"type"};
    CountingMeasurementContext first{{5, 1}};
    AlternateMeasurementContext second;

    CHECK(label.measure(first) == Size{5, 1});
    CHECK(first.calls() == 1);

    CHECK(label.measure(second) == Size{7, 2});
    CHECK(second.calls() == 1);
}

TEST_CASE("Context-free and context-aware measurement never share cached results") {
    Label label{"text"};
    label.setSizeConstraints({{0, 0}, {2, 1}, {100, 100}});
    CountingMeasurementContext context{{9, 1}};

    CHECK(label.measure() == Size{2, 1});
    CHECK(label.measure(context) == Size{9, 1});
    CHECK(context.calls() == 1);

    // Returning to context-free measurement must restore that cache domain rather than reuse 9x1.
    CHECK(label.measure() == Size{2, 1});
}

TEST_CASE("MeasurementContext result still obeys Widget and parent constraints") {
    Label label{"clamped"};
    label.setSizeConstraints({{4, 2}, {0, 0}, {20, 10}});
    CountingMeasurementContext context{{30, 1}};

    CHECK(label.measure(context, {{0, 0}, {12, 8}}) == Size{12, 2});
}

TEST_CASE("Window owns visual children through the existing Container contract") {
    Window window;
    auto& label = window.emplace<Label>("Hello");

    CHECK(window.childCount() == 1);
    CHECK(&window.childAt(0) == &label);
    CHECK(label.parent() == &window);
    CHECK(label.owner() == &window);
    CHECK(!window.isFocusable());
}
