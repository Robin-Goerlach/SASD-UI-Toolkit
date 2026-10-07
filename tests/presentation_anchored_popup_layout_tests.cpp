#include "test_framework.hpp"

#include <sasd/ui/presentation/anchored_popup_layout.hpp>

#include <limits>
#include <optional>

using namespace sasd::ui;
using namespace sasd::ui::presentation;

TEST_CASE("Anchored popup placement preserves preferred below attachment when it fits") {
    const auto placement =
        placeAnchoredPopup({10, 5, 20, 4}, {24, 12}, {0, 0, 80, 40});

    CHECK(placement.has_value());
    CHECK(placement->side == PopupVerticalSide::below);
    CHECK(placement->bounds == Rect{10, 9, 24, 12});
}

TEST_CASE("Anchored popup placement flips above when preferred below side has no room") {
    const auto placement =
        placeAnchoredPopup({10, 30, 20, 4}, {24, 12}, {0, 0, 80, 40});

    CHECK(placement.has_value());
    CHECK(placement->side == PopupVerticalSide::above);
    CHECK(placement->bounds == Rect{10, 18, 24, 12});
}

TEST_CASE("Anchored popup placement can prefer above and fall back below") {
    const auto above =
        placeAnchoredPopup(
            {15, 20, 10, 3},
            {18, 8},
            {0, 0, 60, 40},
            PopupVerticalSide::above);
    CHECK(above.has_value());
    CHECK(above->side == PopupVerticalSide::above);
    CHECK(above->bounds == Rect{15, 12, 18, 8});

    const auto fallback_below =
        placeAnchoredPopup(
            {15, 2, 10, 3},
            {18, 8},
            {0, 0, 60, 40},
            PopupVerticalSide::above);
    CHECK(fallback_below.has_value());
    CHECK(fallback_below->side == PopupVerticalSide::below);
    CHECK(fallback_below->bounds == Rect{15, 5, 18, 8});
}

TEST_CASE("Anchored popup placement clamps only horizontal overflow") {
    const auto right_overflow =
        placeAnchoredPopup({47, 5, 8, 3}, {20, 6}, {0, 0, 60, 30});
    CHECK(right_overflow.has_value());
    CHECK(right_overflow->bounds == Rect{40, 8, 20, 6});

    const auto left_overflow =
        placeAnchoredPopup({-7, 5, 8, 3}, {20, 6}, {0, 0, 60, 30});
    CHECK(left_overflow.has_value());
    CHECK(left_overflow->bounds == Rect{0, 8, 20, 6});
}

TEST_CASE("Anchored popup placement respects a non-zero viewport origin") {
    const auto placement =
        placeAnchoredPopup({102, 55, 8, 4}, {30, 10}, {100, 50, 50, 30});

    CHECK(placement.has_value());
    CHECK(placement->side == PopupVerticalSide::below);
    CHECK(placement->bounds == Rect{102, 59, 30, 10});
}

TEST_CASE("Anchored popup placement fails closed when a complete attached popup cannot fit") {
    CHECK(!placeAnchoredPopup({10, 10, 10, 3}, {81, 5}, {0, 0, 80, 40}).has_value());
    CHECK(!placeAnchoredPopup({10, 10, 10, 3}, {20, 41}, {0, 0, 80, 40}).has_value());

    /*
     * The popup itself fits the viewport, but the anchor splits the remaining vertical space so
     * neither a complete below nor complete above attachment is possible. This slice intentionally
     * does not invent clipping or scrolling.
     */
    CHECK(!placeAnchoredPopup({10, 17, 10, 6}, {20, 20}, {0, 0, 80, 40}).has_value());
}

TEST_CASE("Anchored popup placement rejects empty or malformed geometry") {
    CHECK(!placeAnchoredPopup({0, 0, 0, 4}, {10, 5}, {0, 0, 40, 20}).has_value());
    CHECK(!placeAnchoredPopup({0, 0, 4, 4}, {0, 5}, {0, 0, 40, 20}).has_value());
    CHECK(!placeAnchoredPopup({0, 0, 4, 4}, {10, 5}, {0, 0, 0, 20}).has_value());
    CHECK(!placeAnchoredPopup({0, 0, 4, 4}, {-1, 5}, {0, 0, 40, 20}).has_value());
}

TEST_CASE("Anchored popup placement remains safe near Coordinate limits") {
    constexpr Coordinate maximum = std::numeric_limits<Coordinate>::max();

    const Rect viewport{static_cast<Coordinate>(maximum - 99), 0, 100, 50};
    const Rect anchor{static_cast<Coordinate>(maximum - 20), 10, 10, 3};

    const auto placement = placeAnchoredPopup(anchor, {25, 8}, viewport);
    CHECK(placement.has_value());

    /*
     * The anchor-aligned x would make the popup extend past the viewport. Horizontal fitting clamps
     * to the final complete origin without overflowing int32 arithmetic.
     */
    CHECK(placement->bounds.x == static_cast<Coordinate>(maximum - 24));
    CHECK(placement->bounds.y == 13);
}

TEST_CASE("Fixed popup row geometry maps indices to deterministic backend-neutral rectangles") {
    const Rect content{4, 10, 30, 21};

    const auto first = fixedPopupRowBounds(content, 3U, 0U, 7);
    const auto middle = fixedPopupRowBounds(content, 3U, 1U, 7);
    const auto last = fixedPopupRowBounds(content, 3U, 2U, 7);

    CHECK(first == std::optional<Rect>{Rect{4, 10, 30, 7}});
    CHECK(middle == std::optional<Rect>{Rect{4, 17, 30, 7}});
    CHECK(last == std::optional<Rect>{Rect{4, 24, 30, 7}});
}

TEST_CASE("Fixed popup row geometry rejects invalid indices and implicit clipping") {
    const Rect content{0, 0, 20, 10};

    CHECK(!fixedPopupRowBounds(content, 2U, 2U, 5).has_value());
    CHECK(!fixedPopupRowBounds(content, 3U, 0U, 5).has_value());
    CHECK(!fixedPopupRowBounds(content, 2U, 0U, 0).has_value());
    CHECK(!fixedPopupRowBounds({0, 0, 20, 0}, 1U, 0U, 1).has_value());
    CHECK(!fixedPopupRowBounds(content, 0U, 0U, 5).has_value());
}

TEST_CASE("Fixed popup row geometry fails closed when a row origin leaves Coordinate range") {
    constexpr Coordinate maximum = std::numeric_limits<Coordinate>::max();

    /*
     * Rect permits an edge beyond Coordinate::max() because edge arithmetic is widened. The first row
     * origin is representable, while the second would require max+1 and must therefore fail closed.
     */
    const Rect content{0, maximum, 10, 2};
    CHECK(fixedPopupRowBounds(content, 2U, 0U, 1).has_value());
    CHECK(!fixedPopupRowBounds(content, 2U, 1U, 1).has_value());
}
