#include "test_framework.hpp"

#include <sasd/ui/hbox.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/vbox.hpp>

#include <cstdint>
#include <stdexcept>
#include <string_view>

using namespace sasd::ui;

namespace {

class FixedTextContext final : public MeasurementContext {
public:
    explicit FixedTextContext(Size size) : size_{size} {}

    Size measureText(std::string_view) const override {
        ++calls_;
        return size_;
    }

    std::uint64_t revision() const noexcept override { return 0; }
    [[nodiscard]] int calls() const noexcept { return calls_; }

private:
    Size size_{};
    mutable int calls_{0};
};

} // namespace

TEST_CASE("VBox measures widest child and stacked visible heights") {
    VBox box;
    box.setSpacing(1);

    auto& first = box.emplace<Widget>();
    first.setSizeConstraints({{0, 0}, {3, 2}, {100, 100}});

    auto& second = box.emplace<Widget>();
    second.setSizeConstraints({{0, 0}, {5, 1}, {100, 100}});

    CHECK(box.measure() == Size{5, 4});
}

TEST_CASE("HBox measures summed widths and tallest child") {
    HBox box;
    box.setSpacing(2);

    auto& first = box.emplace<Widget>();
    first.setSizeConstraints({{0, 0}, {3, 2}, {100, 100}});

    auto& second = box.emplace<Widget>();
    second.setSizeConstraints({{0, 0}, {4, 5}, {100, 100}});

    CHECK(box.measure() == Size{9, 5});
}

TEST_CASE("Box layouts exclude invisible children and adjacent spacing") {
    VBox box;
    box.setSpacing(3);

    auto& first = box.emplace<Widget>();
    first.setSizeConstraints({{0, 0}, {2, 1}, {100, 100}});

    auto& hidden = box.emplace<Widget>();
    hidden.setSizeConstraints({{0, 0}, {20, 20}, {100, 100}});
    hidden.setVisible(false);

    auto& last = box.emplace<Widget>();
    last.setSizeConstraints({{0, 0}, {4, 2}, {100, 100}});

    CHECK(box.measure() == Size{4, 6});
}

TEST_CASE("VBox stretches cross axis and arranges children in adoption order") {
    VBox box;
    box.setSpacing(1);

    auto& first = box.emplace<Widget>();
    first.setSizeConstraints({{0, 0}, {3, 2}, {100, 100}});

    auto& second = box.emplace<Widget>();
    second.setSizeConstraints({{0, 0}, {4, 3}, {100, 100}});

    (void)box.measure();
    box.arrange({7, 9, 10, 8});

    CHECK(first.bounds() == Rect{0, 0, 10, 2});
    CHECK(second.bounds() == Rect{0, 3, 10, 3});
}

TEST_CASE("HBox stretches cross axis and clips later children when space is exhausted") {
    HBox box;
    box.setSpacing(1);

    auto& first = box.emplace<Widget>();
    first.setSizeConstraints({{0, 0}, {3, 1}, {100, 100}});

    auto& second = box.emplace<Widget>();
    second.setSizeConstraints({{0, 0}, {4, 1}, {100, 100}});

    (void)box.measure();
    box.arrange({0, 0, 5, 3});

    CHECK(first.bounds() == Rect{0, 0, 3, 3});
    CHECK(second.bounds() == Rect{4, 0, 1, 3});
}

TEST_CASE("Box layout forwards MeasurementContext to content children") {
    VBox box;
    box.setSpacing(1);
    box.emplace<Label>("first");
    box.emplace<Label>("second");

    FixedTextContext context{{6, 2}};

    CHECK(box.measure(context) == Size{6, 5});
    CHECK(context.calls() == 2);
}

TEST_CASE("Changing box spacing invalidates cached measurement") {
    VBox box;
    box.emplace<Widget>().setSizeConstraints({{0, 0}, {2, 1}, {100, 100}});
    box.emplace<Widget>().setSizeConstraints({{0, 0}, {2, 1}, {100, 100}});

    CHECK(box.measure() == Size{2, 2});
    CHECK(box.isMeasureValid());

    box.setSpacing(2);

    CHECK(!box.isMeasureValid());
    CHECK(box.measure() == Size{2, 4});
}

TEST_CASE("Box layouts reject negative spacing without changing state") {
    HBox box;
    box.setSpacing(2);

    bool threw = false;
    try {
        box.setSpacing(-1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(box.spacing() == 2);
}
