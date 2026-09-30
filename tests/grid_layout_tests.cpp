#include "test_framework.hpp"

#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/measurement_context.hpp>

#include <cstdint>
#include <stdexcept>
#include <string_view>

using namespace sasd::ui;

namespace {

class FixedGridTextContext final : public MeasurementContext {
public:
    explicit FixedGridTextContext(Size size) : size_{size} {}

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

void setPreferred(Widget& widget, Size size) {
    widget.setSizeConstraints({{0, 0}, size, {1000, 1000}});
}

} // namespace

TEST_CASE("GridLayout measures per-column maxima and per-row maxima") {
    GridLayout grid{2};
    grid.setColumnSpacing(1);
    grid.setRowSpacing(2);

    auto& a = grid.emplace<Widget>();
    auto& b = grid.emplace<Widget>();
    auto& c = grid.emplace<Widget>();
    setPreferred(a, {3, 2});
    setPreferred(b, {5, 1});
    setPreferred(c, {4, 6});

    /*
     * column widths: max(3,4)=4 and 5 => 4 + 1 + 5 = 10
     * row heights: max(2,1)=2 and 6 => 2 + 2 + 6 = 10
     */
    CHECK(grid.measure() == Size{10, 10});
}

TEST_CASE("GridLayout assigns visible children row-major and collapses hidden slots") {
    GridLayout grid{2};

    auto& first = grid.emplace<Widget>();
    auto& hidden = grid.emplace<Widget>();
    auto& second = grid.emplace<Widget>();
    setPreferred(first, {2, 1});
    setPreferred(hidden, {50, 50});
    setPreferred(second, {4, 3});
    hidden.setVisible(false);

    CHECK(grid.measure() == Size{6, 3});
    grid.arrange({7, 9, 10, 5});

    CHECK(first.bounds() == Rect{0, 0, 2, 3});
    CHECK(second.bounds() == Rect{2, 0, 4, 3});
}

TEST_CASE("GridLayout stretches children to complete intrinsic cells") {
    GridLayout grid{2};
    grid.setColumnSpacing(1);
    grid.setRowSpacing(1);

    auto& a = grid.emplace<Widget>();
    auto& b = grid.emplace<Widget>();
    auto& c = grid.emplace<Widget>();
    auto& d = grid.emplace<Widget>();

    setPreferred(a, {2, 1});
    setPreferred(b, {5, 3});
    setPreferred(c, {4, 2});
    setPreferred(d, {1, 6});

    (void)grid.measure();
    grid.arrange({20, 30, 20, 20});

    // Tracks are [4,5] x [3,6]; children stretch inside their complete cell.
    CHECK(a.bounds() == Rect{0, 0, 4, 3});
    CHECK(b.bounds() == Rect{5, 0, 5, 3});
    CHECK(c.bounds() == Rect{0, 4, 4, 6});
    CHECK(d.bounds() == Rect{5, 4, 5, 6});
}

TEST_CASE("GridLayout clips later tracks deterministically when final space is small") {
    GridLayout grid{2};
    grid.setColumnSpacing(1);
    grid.setRowSpacing(1);

    auto& a = grid.emplace<Widget>();
    auto& b = grid.emplace<Widget>();
    auto& c = grid.emplace<Widget>();
    auto& d = grid.emplace<Widget>();

    setPreferred(a, {4, 3});
    setPreferred(b, {5, 3});
    setPreferred(c, {4, 6});
    setPreferred(d, {5, 6});

    (void)grid.measure();
    grid.arrange({0, 0, 7, 6});

    // Earlier tracks retain their intrinsic allocation first, like the initial Box layout contract.
    CHECK(a.bounds() == Rect{0, 0, 4, 3});
    CHECK(b.bounds() == Rect{5, 0, 2, 3});
    CHECK(c.bounds() == Rect{0, 4, 4, 2});
    CHECK(d.bounds() == Rect{5, 4, 2, 2});
}

TEST_CASE("GridLayout arranges fully exhausted later rows to zero extent") {
    GridLayout grid{1};
    grid.setRowSpacing(1);

    auto& first = grid.emplace<Widget>();
    auto& second = grid.emplace<Widget>();
    setPreferred(first, {3, 5});
    setPreferred(second, {3, 5});

    (void)grid.measure();
    grid.arrange({0, 0, 8, 4});

    CHECK(first.bounds() == Rect{0, 0, 3, 4});
    CHECK(second.bounds() == Rect{0, 4, 3, 0});
}

TEST_CASE("GridLayout forwards MeasurementContext to visible children") {
    GridLayout grid{2};
    grid.emplace<Label>("A");
    grid.emplace<Label>("B");
    grid.emplace<Label>("C");

    FixedGridTextContext context{{6, 2}};

    CHECK(grid.measure(context) == Size{12, 4});
    CHECK(context.calls() == 3);
}

TEST_CASE("GridLayout property changes invalidate measurement") {
    GridLayout grid{2};
    auto& a = grid.emplace<Widget>();
    auto& b = grid.emplace<Widget>();
    setPreferred(a, {2, 1});
    setPreferred(b, {2, 1});

    CHECK(grid.measure() == Size{4, 1});
    CHECK(grid.isMeasureValid());

    grid.setColumnCount(1);
    CHECK(!grid.isMeasureValid());
    CHECK(grid.measure() == Size{2, 2});

    grid.setColumnSpacing(3);
    CHECK(!grid.isMeasureValid());
    (void)grid.measure();

    grid.setRowSpacing(4);
    CHECK(!grid.isMeasureValid());
}

TEST_CASE("GridLayout rejects invalid column count and negative spacing") {
    bool constructor_threw = false;
    try {
        GridLayout invalid{0};
    } catch (const std::invalid_argument&) {
        constructor_threw = true;
    }
    CHECK(constructor_threw);

    GridLayout grid{2};
    grid.setColumnSpacing(2);
    grid.setRowSpacing(3);

    bool columns_threw = false;
    try {
        grid.setColumnCount(0);
    } catch (const std::invalid_argument&) {
        columns_threw = true;
    }

    bool column_spacing_threw = false;
    try {
        grid.setColumnSpacing(-1);
    } catch (const std::invalid_argument&) {
        column_spacing_threw = true;
    }

    bool row_spacing_threw = false;
    try {
        grid.setRowSpacing(-1);
    } catch (const std::invalid_argument&) {
        row_spacing_threw = true;
    }

    CHECK(columns_threw);
    CHECK(column_spacing_threw);
    CHECK(row_spacing_threw);
    CHECK(grid.columnCount() == 2);
    CHECK(grid.columnSpacing() == 2);
    CHECK(grid.rowSpacing() == 3);
}
