#include "test_framework.hpp"

#include <sasd/ui/geometry.hpp>

#include <limits>

using namespace sasd::ui;

TEST_CASE("Rect contains points inside its half-open bounds") {
    const Rect rect{10, 20, 30, 40};

    CHECK(rect.contains({10, 20}));
    CHECK(rect.contains({39, 59}));
    CHECK(!rect.contains({40, 59}));
    CHECK(!rect.contains({39, 60}));
}

TEST_CASE("Rect treats zero and negative extents as empty") {
    // Invalid/empty extents must never accidentally become hittable. This matters later for
    // layout transitions where a widget can temporarily have no arranged area.
    const Rect zero_width{10, 20, 0, 40};
    const Rect negative_width{10, 20, -1, 40};
    const Rect negative_height{10, 20, 30, -1};

    CHECK(zero_width.isEmpty());
    CHECK(negative_width.isEmpty());
    CHECK(negative_height.isEmpty());
    CHECK(!zero_width.contains({10, 20}));
    CHECK(!negative_width.contains({10, 20}));
    CHECK(!negative_height.contains({10, 20}));
}

TEST_CASE("Rect hit testing remains safe near Coordinate limits") {
    // Regression test: the original implementation calculated x + width in int32_t. A rectangle
    // close to INT32_MAX could therefore overflow before the comparison was made. contains() now
    // widens the edge calculation while keeping the public Coordinate type compact.
    constexpr Coordinate maximum = std::numeric_limits<Coordinate>::max();
    const Rect near_maximum{static_cast<Coordinate>(maximum - 5), 0, 10, 1};

    CHECK(near_maximum.contains({maximum, 0}));
    CHECK(!near_maximum.contains({static_cast<Coordinate>(maximum - 6), 0}));
}

TEST_CASE("SizeConstraints clamps to minimum and maximum") {
    const SizeConstraints constraints{{10, 5}, {20, 10}, {100, 50}};

    CHECK(constraints.hasValidRange());
    CHECK(constraints.clamp({5, 100}) == Size{10, 50});
    CHECK(constraints.clamp({25, 20}) == Size{25, 20});
}

TEST_CASE("SizeConstraints handles an invalid range deterministically") {
    // std::clamp requires minimum <= maximum. Keeping this regression test ensures malformed input
    // cannot turn into undefined behavior before higher-level layout validation reports the error.
    const SizeConstraints invalid{{100, 10}, {75, 7}, {50, 5}};

    CHECK(!invalid.hasValidRange());
    CHECK(invalid.clamp({75, 7}) == Size{100, 10});
}
