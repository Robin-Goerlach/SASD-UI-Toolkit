#include "test_framework.hpp"

#include "rendered/sdl3/sdl3_software_device.hpp"

#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/display_list_executor.hpp>
#include <sasd/ui/text/utf8.hpp>

#include <cstddef>
#include <optional>
#include <string>

using namespace sasd::ui;
using namespace sasd::ui::rendered;
using namespace sasd::ui::rendered::sdl3;

#ifndef SASD_UI_SDL3_TEST_FONT_PATH
#error "SASD_UI_SDL3_TEST_FONT_PATH must be defined for SDL3 adapter tests"
#endif

namespace {

[[nodiscard]] Sdl3SoftwareDevice makeDevice() {
    return Sdl3SoftwareDevice{
        Sdl3SoftwareDeviceConfig{
            {160, 80},
            SASD_UI_SDL3_TEST_FONT_PATH,
            16.0F,
        }};
}

} // namespace

TEST_CASE("SDL3 software device distinguishes default background and foreground fills") {
    auto device = makeDevice();
    device.clear();

    CHECK(device.pixelAt({0, 0}) == Rgba8{0, 0, 0, 255});

    DisplayList list;
    list.fillRect({2, 2, 12, 12}); // default background: remains black
    list.fillRect({20, 2, 4, 12}, Color::default_color, FillRole::foreground);

    CHECK(DisplayListExecutor::execute(list, device) == 2);

    CHECK(device.pixelAt({3, 3}) == Rgba8{0, 0, 0, 255});
    CHECK(device.pixelAt({21, 3}) == Rgba8{224, 224, 224, 255});
}

TEST_CASE("SDL3 software device executes named fill and stroke commands") {
    auto device = makeDevice();
    device.clear();

    DisplayList list;
    list.fillRect({5, 5, 20, 20}, Color::blue);
    list.strokeRect({35, 5, 20, 20}, Color::bright_red, 2);

    CHECK(DisplayListExecutor::execute(list, device) == 2);

    CHECK(device.pixelAt({10, 10}) == Rgba8{0, 0, 170, 255});
    CHECK(device.pixelAt({35, 5}) == Rgba8{255, 85, 85, 255});
    CHECK(device.pixelAt({45, 15}) == Rgba8{0, 0, 0, 255});
}

TEST_CASE("SDL3 ttf metrics provide positive UTF-8 sizes and scalar advances") {
    auto device = makeDevice();

    const std::string text{"A\xCE\xA9\xE7\x95\x8C"};
    const Size measured = device.measureText(text);

    CHECK(measured.width > 0);
    CHECK(measured.height == device.lineHeight());
    CHECK(device.lineHeight() > 0);

    const auto zero = device.textAdvanceToScalar(text, 0);
    const auto one = device.textAdvanceToScalar(text, 1);
    const auto end = device.textAdvanceToScalar(text, utf8::scalarCount(text));

    CHECK(zero == std::optional<Coordinate>{0});
    CHECK(one.has_value());
    CHECK(end.has_value());
    CHECK(*one >= 0);
    CHECK(*end == measured.width);
    CHECK(*end >= *one);
}

TEST_CASE("SDL3 software device renders UTF-8 text into the off-screen surface") {
    auto device = makeDevice();
    device.clear();

    DisplayList list;

    TextStyle style;
    style.foreground = Color::bright_green;
    style.underline = true;

    list.drawText({4, 4}, std::string{"A\xCE\xA9"}, style, Rect{4, 4, 80, 30});
    CHECK(DisplayListExecutor::execute(list, device) == 1);

    bool found_non_background_pixel = false;
    for (Coordinate y = 4; y < 34 && !found_non_background_pixel; ++y) {
        for (Coordinate x = 4; x < 84; ++x) {
            if (device.pixelAt({x, y}) != Rgba8{0, 0, 0, 255}) {
                found_non_background_pixel = true;
                break;
            }
        }
    }

    CHECK(found_non_background_pixel);
}

TEST_CASE("SDL3 software device honors text clipping") {
    auto device = makeDevice();
    device.clear();

    DisplayList list;
    TextStyle style;
    style.foreground = Color::bright_white;

    list.drawText({0, 0}, "Clipped text", style, Rect{20, 0, 20, 20});
    CHECK(DisplayListExecutor::execute(list, device) == 1);

    // The glyph run starts at x=0, but clipping must keep pixels left of x=20 untouched.
    CHECK(device.pixelAt({2, 8}) == Rgba8{0, 0, 0, 255});
}
