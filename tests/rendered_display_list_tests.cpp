#include "test_framework.hpp"

#include <sasd/ui/rendered/display_list.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

TEST_CASE("Rendered DisplayList preserves command order and payload") {
    DisplayList list;

    TextStyle style;
    style.foreground = Color::bright_cyan;
    style.bold = true;

    list.fillRect({1, 2, 30, 20}, Color::blue);
    list.strokeRect({3, 4, 10, 8}, Color::bright_green, 2);
    list.drawText({5, 6}, "Hello", style);

    CHECK(list.size() == 3);

    const auto commands = list.commands();
    CHECK(std::get<FillRectCommand>(commands[0]) ==
          FillRectCommand{Rect{1, 2, 30, 20}, Color::blue});
    CHECK(std::get<StrokeRectCommand>(commands[1]) ==
          StrokeRectCommand{Rect{3, 4, 10, 8}, Color::bright_green, 2});
    CHECK(std::get<DrawTextCommand>(commands[2]) ==
          DrawTextCommand{Point{5, 6}, std::string{"Hello"}, style, std::nullopt});
}

TEST_CASE("Rendered DisplayList ignores zero-area drawing operations") {
    DisplayList list;

    list.fillRect({0, 0, 0, 20}, Color::red);
    list.strokeRect({0, 0, 20, 0}, Color::green);
    list.drawText({0, 0}, "");

    CHECK(list.empty());
}

TEST_CASE("Rendered DisplayList rejects malformed rectangles transactionally") {
    DisplayList list;
    list.fillRect({0, 0, 5, 5}, Color::blue);

    bool fill_threw = false;
    try {
        list.fillRect({0, 0, -1, 5}, Color::red);
    } catch (const std::invalid_argument&) {
        fill_threw = true;
    }

    bool stroke_threw = false;
    try {
        list.strokeRect({0, 0, 5, -1}, Color::red);
    } catch (const std::invalid_argument&) {
        stroke_threw = true;
    }

    CHECK(fill_threw);
    CHECK(stroke_threw);
    CHECK(list.size() == 1);

    // Rejected input must not append a partial command or disturb the previous frame snapshot.
    CHECK(std::get<FillRectCommand>(list.commands()[0]).bounds == Rect{0, 0, 5, 5});
}

TEST_CASE("Rendered DisplayList rejects non-positive stroke thickness") {
    DisplayList list;

    bool zero_threw = false;
    try {
        list.strokeRect({0, 0, 5, 5}, Color::white, 0);
    } catch (const std::invalid_argument&) {
        zero_threw = true;
    }

    bool negative_threw = false;
    try {
        list.strokeRect({0, 0, 5, 5}, Color::white, -1);
    } catch (const std::invalid_argument&) {
        negative_threw = true;
    }

    CHECK(zero_threw);
    CHECK(negative_threw);
    CHECK(list.empty());
}

TEST_CASE("Rendered DisplayList owns text independently from caller storage") {
    DisplayList list;
    std::string source{"Robin A\xCE\xA9\xE7\x95\x8C"};

    list.drawText({7, 9}, source);
    source.assign("changed");

    CHECK(list.size() == 1);
    const auto& command = std::get<DrawTextCommand>(list.commands()[0]);
    CHECK(command.origin == Point{7, 9});
    CHECK(command.text == std::string{"Robin A\xCE\xA9\xE7\x95\x8C"});
}

TEST_CASE("Rendered DisplayList clear starts a new deterministic frame") {
    DisplayList list;
    list.fillRect({0, 0, 4, 4}, Color::black);
    list.drawText({1, 1}, "frame one");

    CHECK(!list.empty());

    list.clear();

    CHECK(list.empty());
    CHECK(list.commands().size() == 0);

    list.drawText({2, 3}, "frame two");
    CHECK(list.size() == 1);
    CHECK(std::get<DrawTextCommand>(list.commands()[0]).text == "frame two");
}

TEST_CASE("Rendered DisplayList stores per-command text clipping") {
    DisplayList list;

    const Rect clip{10, 20, 30, 40};
    list.drawText({12, 22}, "clipped", {}, clip);

    CHECK(list.size() == 1);
    const auto& command = std::get<DrawTextCommand>(list.commands()[0]);
    CHECK(command.clip_bounds == std::optional<Rect>{clip});
}

TEST_CASE("Rendered DisplayList validates text clipping before appending") {
    DisplayList list;

    bool negative_threw = false;
    try {
        list.drawText({0, 0}, "invalid", {}, Rect{0, 0, -1, 5});
    } catch (const std::invalid_argument&) {
        negative_threw = true;
    }

    CHECK(negative_threw);
    CHECK(list.empty());

    // A valid but empty clip cannot expose any glyphs, so recording the command would be pointless.
    list.drawText({0, 0}, "invisible", {}, Rect{0, 0, 0, 5});
    CHECK(list.empty());
}
