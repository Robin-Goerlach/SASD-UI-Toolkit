#include "test_framework.hpp"

#include <sasd/ui/geometry.hpp>

using namespace sasd::ui;

TEST_CASE("Rect contains points inside its half-open bounds") {
    const Rect rect{10, 20, 30, 40};

    CHECK(rect.contains({10, 20}));
    CHECK(rect.contains({39, 59}));
    CHECK(!rect.contains({40, 59}));
    CHECK(!rect.contains({39, 60}));
}

TEST_CASE("SizeConstraints clamps to minimum and maximum") {
    const SizeConstraints constraints{{10, 5}, {20, 10}, {100, 50}};

    CHECK(constraints.clamp({5, 100}) == Size{10, 50});
    CHECK(constraints.clamp({25, 20}) == Size{25, 20});
}
