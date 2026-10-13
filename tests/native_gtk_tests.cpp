#include "test_framework.hpp"

#include "gtk_render_context.hpp"

#include <sasd/ui/native/gtk_backend.hpp>
#include <sasd/ui/rendered/display_list.hpp>

using namespace sasd::ui;
using namespace sasd::ui::native::gtk;

TEST_CASE("GTK Cairo/Pango context is usable on an image surface") {
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 80, 40);
    cairo_t* cairo = cairo_create(surface);
    {
        detail::GtkRenderContext context{*cairo, "Sans 12"};
        context.clear(Color::black);
        context.fillRect(rendered::FillRectCommand{{2, 2, 20, 10}, Color::white});
        context.strokeRect(rendered::StrokeRectCommand{{2, 2, 20, 10}, Color::bright_white, 1});
        context.drawText(rendered::DrawTextCommand{{4, 4}, "Grüße", TextStyle{Color::bright_white},
                                                    std::nullopt});
        CHECK(context.lineHeight() > 0);
        CHECK(context.measureText("Grüße").width > 0);
        CHECK(context.textAdvanceToScalar("Grüße", 2).has_value());
    }
    cairo_destroy(cairo);
    cairo_surface_destroy(surface);
}

TEST_CASE("GTK native host owns a complete frame and can shut down repeatedly") {
    GtkWindowBackend backend{{"SASD GTK test", {320, 200}, true, true, 12.0, Color::black}};
    CHECK(!backend.isInitialized());
    backend.initialize();
    CHECK(backend.isInitialized());
    CHECK(backend.windowSize() == Size{320, 200});

    rendered::DisplayList frame;
    frame.fillRect({0, 0, 320, 200}, Color::black);
    frame.strokeRect({10, 10, 100, 30}, Color::bright_white);
    frame.drawText({20, 20}, "GTK UTF-8: Grüße", TextStyle{Color::bright_white});
    CHECK(backend.presentFrame(frame) == frame.size());
    CHECK(backend.presentationRequested());
    CHECK(backend.lineHeight() > 0);
    CHECK(backend.measureText("Grüße").width > 0);

    for (int i = 0; i < 4; ++i) (void)backend.pollEvent();
    backend.shutdown();
    backend.shutdown();
    CHECK(!backend.isInitialized());
}
