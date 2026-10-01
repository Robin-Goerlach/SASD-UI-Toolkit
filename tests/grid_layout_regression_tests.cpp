#include "test_framework.hpp"

#include <sasd/ui/grid_layout.hpp>

using namespace sasd::ui;

namespace {

void setPreferred(Widget& widget, Size size) {
    widget.setSizeConstraints({{0, 0}, size, {1000, 1000}});
}

} // namespace

TEST_CASE("GridLayout preserves an occupied trailing zero-width column") {
    GridLayout grid{2};
    grid.setColumnSpacing(2);

    auto& first = grid.emplace<Widget>();
    auto& second = grid.emplace<Widget>();
    setPreferred(first, {3, 1});
    setPreferred(second, {0, 1});

    /*
     * The second child still occupies column 1 even though its desired width is zero. Occupancy is
     * determined by row-major cell assignment, not by measured extent, so the inter-column spacing
     * remains part of the intrinsic grid width: 3 + 2 + 0 = 5.
     */
    CHECK(grid.measure() == Size{5, 1});

    grid.arrange({0, 0, 5, 1});

    CHECK(first.bounds() == Rect{0, 0, 3, 1});
    CHECK(second.bounds() == Rect{5, 0, 0, 1});
}
