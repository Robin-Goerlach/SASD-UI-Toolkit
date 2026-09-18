#include "test_framework.hpp"

#include <sasd/ui/terminal/screen_buffer.hpp>

#include <limits>
#include <stdexcept>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal ScreenBuffer creates deterministic blank cells") {
    ScreenBuffer buffer{{4, 3}};

    CHECK(buffer.size() == Size{4, 3});
    CHECK(buffer.cellCount() == 12);
    CHECK(!buffer.empty());

    for (Coordinate y = 0; y < 3; ++y) {
        for (Coordinate x = 0; x < 4; ++x) {
            CHECK(buffer.at({x, y}) == Cell{U' '});
        }
    }
}

TEST_CASE("Terminal ScreenBuffer stores Unicode code points without byte assumptions") {
    ScreenBuffer buffer{{3, 1}};

    buffer.set({0, 0}, Cell{U'A'});
    buffer.set({1, 0}, Cell{U'Ω'});
    buffer.set({2, 0}, Cell{U'界'});

    CHECK(buffer.at({0, 0}).code_point == U'A');
    CHECK(buffer.at({1, 0}).code_point == U'Ω');
    CHECK(buffer.at({2, 0}).code_point == U'界');

    /*
     * This test intentionally validates code-point storage only. It does not claim that every code
     * point occupies one physical terminal column; display width/grapheme handling is a later M2
     * layer above the raw cell buffer.
     */
}

TEST_CASE("Terminal ScreenBuffer rejects out of range points") {
    ScreenBuffer buffer{{2, 2}};

    CHECK(buffer.contains({0, 0}));
    CHECK(buffer.contains({1, 1}));
    CHECK(!buffer.contains({-1, 0}));
    CHECK(!buffer.contains({0, -1}));
    CHECK(!buffer.contains({2, 0}));
    CHECK(!buffer.contains({0, 2}));

    bool negative_threw = false;
    try {
        (void)buffer.at({-1, 0});
    } catch (const std::out_of_range&) {
        negative_threw = true;
    }
    CHECK(negative_threw);

    bool edge_threw = false;
    try {
        buffer.set({2, 1}, Cell{U'X'});
    } catch (const std::out_of_range&) {
        edge_threw = true;
    }
    CHECK(edge_threw);
}

TEST_CASE("Terminal ScreenBuffer resize is transactional and clears previous contents") {
    ScreenBuffer buffer{{2, 2}};
    buffer.set({0, 0}, Cell{U'X'});

    buffer.resize({3, 1}, Cell{U'.'});

    CHECK(buffer.size() == Size{3, 1});
    CHECK(buffer.cellCount() == 3);
    CHECK(buffer.at({0, 0}) == Cell{U'.'});
    CHECK(buffer.at({1, 0}) == Cell{U'.'});
    CHECK(buffer.at({2, 0}) == Cell{U'.'});

    bool threw = false;
    try {
        buffer.resize({-1, 5});
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);

    // A failed resize must not publish partial dimensions or discard the old frame.
    CHECK(buffer.size() == Size{3, 1});
    CHECK(buffer.at({1, 0}) == Cell{U'.'});
}

TEST_CASE("Terminal ScreenBuffer clear replaces every cell") {
    ScreenBuffer buffer{{3, 2}};
    buffer.set({1, 0}, Cell{U'X'});
    buffer.set({2, 1}, Cell{U'Y'});

    buffer.clear(Cell{U'-'});

    for (Coordinate y = 0; y < 2; ++y) {
        for (Coordinate x = 0; x < 3; ++x) {
            CHECK(buffer.at({x, y}) == Cell{U'-'});
        }
    }
}

TEST_CASE("Terminal ScreenBuffer row exposes only one contiguous row") {
    ScreenBuffer buffer{{4, 2}};

    auto second = buffer.row(1);
    CHECK(second.size() == 4);

    second[0] = Cell{U'A'};
    second[3] = Cell{U'Z'};

    CHECK(buffer.at({0, 1}) == Cell{U'A'});
    CHECK(buffer.at({3, 1}) == Cell{U'Z'});
    CHECK(buffer.at({0, 0}) == Cell{U' '});

    bool threw = false;
    try {
        (void)buffer.row(2);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

TEST_CASE("Terminal ScreenBuffer fill clips rectangles to screen bounds") {
    ScreenBuffer buffer{{5, 4}};

    buffer.fill({-2, 1, 5, 4}, Cell{U'#'});

    for (Coordinate y = 0; y < 4; ++y) {
        for (Coordinate x = 0; x < 5; ++x) {
            const bool expected_filled = x <= 2 && y >= 1;
            CHECK(buffer.at({x, y}) == Cell{expected_filled ? U'#' : U' '});
        }
    }
}

TEST_CASE("Terminal ScreenBuffer fill handles coordinate-limit rectangles safely") {
    ScreenBuffer buffer{{4, 2}};
    constexpr Coordinate maximum = std::numeric_limits<Coordinate>::max();

    // x + width would overflow int32_t in naive arithmetic. The fill implementation widens first.
    buffer.fill({maximum - 2, 0, 10, 1}, Cell{U'X'});

    for (Coordinate y = 0; y < 2; ++y) {
        for (Coordinate x = 0; x < 4; ++x) {
            CHECK(buffer.at({x, y}) == Cell{U' '});
        }
    }
}

TEST_CASE("Terminal ScreenBuffer rejects dimensions that cannot form a vector") {
    ScreenBuffer buffer;
    constexpr Coordinate maximum = std::numeric_limits<Coordinate>::max();

    bool threw = false;
    try {
        buffer.resize({maximum, maximum});
    } catch (const std::length_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(buffer.size() == Size{0, 0});
    CHECK(buffer.empty());
}
