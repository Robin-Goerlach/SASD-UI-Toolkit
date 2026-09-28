#include "test_framework.hpp"

#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/display_list_executor.hpp>
#include <sasd/ui/rendered/render_device.hpp>

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

/**
 * Deterministic device double that records the exact commands dispatched through RenderDevice.
 *
 * Reconstructing DrawCommand values in each callback verifies that DisplayListExecutor does not
 * normalize geometry, restyle text or lose clipping data on the device boundary.
 */
class RecordingRenderDevice final : public RenderDevice {
public:
    void fillRect(const FillRectCommand& command) override {
        commands.push_back(command);
    }

    void strokeRect(const StrokeRectCommand& command) override {
        commands.push_back(command);
    }

    void drawText(const DrawTextCommand& command) override {
        commands.push_back(command);
    }

    std::vector<DrawCommand> commands;
};

class ThrowingRenderDevice final : public RenderDevice {
public:
    void fillRect(const FillRectCommand&) override {
        ++fill_calls;
    }

    void strokeRect(const StrokeRectCommand&) override {
        ++stroke_calls;
        throw std::runtime_error{"synthetic device failure"};
    }

    void drawText(const DrawTextCommand&) override {
        ++text_calls;
    }

    std::size_t fill_calls{0};
    std::size_t stroke_calls{0};
    std::size_t text_calls{0};
};

} // namespace

TEST_CASE("DisplayListExecutor preserves command order and complete payload") {
    DisplayList list;

    TextStyle style;
    style.foreground = Color::bright_cyan;
    style.bold = true;
    style.underline = true;

    const Rect clip{5, 6, 70, 20};

    list.fillRect({1, 2, 100, 40}, Color::black);
    list.strokeRect({3, 4, 80, 30}, Color::bright_green, 2);
    list.drawText({7, 8}, "Robin A\xCE\xA9", style, clip);

    RecordingRenderDevice device;
    const std::size_t executed = DisplayListExecutor::execute(list, device);

    CHECK(executed == 3);
    CHECK(device.commands.size() == 3);

    CHECK(std::get<FillRectCommand>(device.commands[0]) ==
          FillRectCommand{Rect{1, 2, 100, 40}, Color::black});
    CHECK(std::get<StrokeRectCommand>(device.commands[1]) ==
          StrokeRectCommand{Rect{3, 4, 80, 30}, Color::bright_green, 2});
    CHECK(std::get<DrawTextCommand>(device.commands[2]) ==
          DrawTextCommand{Point{7, 8}, std::string{"Robin A\xCE\xA9"}, style,
                          std::optional<Rect>{clip}});

    // Replay is observational: a device cannot consume or mutate the source frame.
    CHECK(list.size() == 3);
}

TEST_CASE("DisplayListExecutor performs no callbacks for an empty list") {
    DisplayList list;
    RecordingRenderDevice device;

    CHECK(DisplayListExecutor::execute(list, device) == 0);
    CHECK(device.commands.empty());
}

TEST_CASE("DisplayListExecutor stops on device exception without mutating source list") {
    DisplayList list;
    list.fillRect({0, 0, 10, 10}, Color::blue);
    list.strokeRect({1, 1, 8, 8}, Color::white, 1);
    list.drawText({2, 2}, "not reached");

    ThrowingRenderDevice device;

    bool threw = false;
    try {
        (void)DisplayListExecutor::execute(list, device);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(device.fill_calls == 1);
    CHECK(device.stroke_calls == 1);
    CHECK(device.text_calls == 0);

    /*
     * The executor intentionally does not roll back an immediate-mode device, but the immutable
     * DisplayList is intact and can be replayed again into a fresh/back-buffered target.
     */
    CHECK(list.size() == 3);
}
