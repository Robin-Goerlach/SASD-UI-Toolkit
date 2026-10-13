#include "test_framework.hpp"

#include "appkit_input_translation.hpp"
#include "appkit_render_context.hpp"

#include <sasd/ui/native/appkit_backend.hpp>

#import <AppKit/AppKit.h>

using namespace sasd::ui;
using namespace sasd::ui::native::appkit;

TEST_CASE("AppKit value translation preserves Core key and logical point contracts") {
    CHECK(detail::translateKey(36) == Key::enter);
    CHECK(detail::translateKey(53) == Key::escape);
    CHECK(detail::translateKey(123) == Key::left);
    CHECK(detail::translateKey(122) == Key::f1);
    CHECK(detail::translateLogicalLetter('A') == Key::a);
    CHECK(!detail::translateLogicalLetter(0x00E4).has_value());
    CHECK(detail::translateModifiers((1ULL << 17U) | (1ULL << 20U)) ==
          (KeyModifier::shift | KeyModifier::meta));
    CHECK(detail::translatePoint(12.5, 18.5) == Point{12, 18});
    CHECK(!detail::translatePoint(-1.0, 2.0).has_value());
    CHECK(detail::translatePointerButton(0, true, 4.0, 5.0, 0)->button == PointerButton::primary);
    CHECK(detail::translatePointerButton(2, false, 4.0, 5.0, 0)->action == PointerAction::release);
}

TEST_CASE("AppKit CoreText context renders and measures UTF-8 without a window") {
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef bitmap = CGBitmapContextCreate(nullptr, 160, 80, 8, 160 * 4, space,
                                                kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(space);
    CHECK(bitmap != nullptr);
    {
        detail::AppKitRenderContext context{bitmap, 12.0};
        context.clear(Color::black);
        context.fillRect(rendered::FillRectCommand{{2, 2, 20, 10}, Color::white});
        context.strokeRect(rendered::StrokeRectCommand{{2, 2, 20, 10}, Color::bright_white, 1});
        context.drawText(rendered::DrawTextCommand{{4, 4}, "Grüße 😀", TextStyle{Color::bright_white},
                                                    std::nullopt});
        CHECK(context.lineHeight() > 0);
        CHECK(context.measureText("Grüße 😀").width > 0);
        CHECK(context.textAdvanceToScalar("Grüße 😀", 6).has_value());
        CHECK(context.measureText("").height == context.lineHeight());
    }
    CGContextRelease(bitmap);
}

TEST_CASE("AppKit host retains a complete frame and shuts down idempotently") {
    AppKitWindowBackend backend{{"SASD AppKit test", {320, 200}, true, true, 12.0, Color::black}};
    CHECK(!backend.isInitialized());
    backend.initialize();
    CHECK(backend.isInitialized());
    CHECK(backend.windowSize() == Size{320, 200});
    rendered::DisplayList frame;
    frame.fillRect({0, 0, 320, 200}, Color::black);
    frame.strokeRect({10, 10, 100, 30}, Color::bright_white);
    frame.drawText({20, 20}, "AppKit UTF-8: Grüße 😀", TextStyle{Color::bright_white});
    CHECK(backend.presentFrame(frame) == frame.size());
    CHECK(backend.presentationRequested());
    for (int i = 0; i != 3; ++i) (void)backend.pollEvent();
    backend.shutdown();
    backend.shutdown();
    CHECK(!backend.isInitialized());
}
