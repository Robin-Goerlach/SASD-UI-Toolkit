#include "test_framework.hpp"

#include "gtk_menu_binding.hpp"
#include "gtk_render_context.hpp"

#include <sasd/ui/native/gtk_backend.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/application.hpp>

#include <memory>
#include <string>

using namespace sasd::ui;
using namespace sasd::ui::native::gtk;

namespace {

[[nodiscard]] std::string menuLabel(GMenuModel* model, int index) {
    GVariant* value = g_menu_model_get_item_attribute_value(model, index, G_MENU_ATTRIBUTE_LABEL,
                                                              G_VARIANT_TYPE_STRING);
    if (value == nullptr) return {};
    const char* text = g_variant_get_string(value, nullptr);
    const std::string result = text == nullptr ? std::string{} : std::string{text};
    g_variant_unref(value);
    return result;
}

} // namespace

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
    CHECK(backend.capabilities().native_menus);
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

TEST_CASE("GTK menu binding projects recursive Core structure with literal labels") {
    EventQueue events;
    GtkWidget* host = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    CHECK(host != nullptr);
    g_object_ref_sink(host);
    detail::GtkMenuBinding binding;
    binding.setHost(host, &events);

    Command open{"Öffnen 😀"};
    Command nested_command{"Research & Development"};
    Command disabled{"Disabled"};
    disabled.setEnabled(false);
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("Fíle_界");
    file.appendCommand(open);
    file.appendSeparator();
    MenuModel& nested = file.appendSubmenu("More & More");
    nested.appendSubmenu("研究").appendCommand(disabled);
    file.appendCommand(nested_command);

    binding.rebuild(bar);
    binding.install();
    GMenuModel* root = binding.model();
    CHECK(root != nullptr);
    CHECK(g_menu_model_get_n_items(root) == 1);
    GMenuModel* file_model = g_menu_model_get_item_link(root, 0, G_MENU_LINK_SUBMENU);
    CHECK(file_model != nullptr);
    CHECK(g_menu_model_get_n_items(file_model) == 4);
    CHECK(menuLabel(file_model, 0) == "Öffnen 😀");
    CHECK(menuLabel(file_model, 2) == "More & More");
    CHECK(menuLabel(file_model, 3) == "Research & Development");
    CHECK(!g_action_group_get_action_enabled(binding.actionGroup(), "command-2"));

    binding.clear();
    g_object_unref(host);
}

TEST_CASE("GTK action activation queues a CommandActivationEvent and never executes inline") {
    EventQueue events;
    GtkWidget* host = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    g_object_ref_sink(host);
    detail::GtkMenuBinding binding;
    binding.setHost(host, &events);
    Command command{"Open"};
    bool executed = false;
    command.setOnExecuted([&] { executed = true; });
    MenuBarModel bar;
    bar.appendMenu("File").appendCommand(command);
    binding.rebuild(bar);
    binding.install();

    g_action_group_activate_action(binding.actionGroup(), "command-1", nullptr);
    CHECK(!executed);
    const auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::holds_alternative<CommandActivationEvent>(*event));
    CHECK(std::get<CommandActivationEvent>(*event).command.get() == &command);
    CHECK(command.execute());
    CHECK(executed);

    binding.clear();
    g_object_unref(host);
}

TEST_CASE("GTK menu refreshes live state and expires safely") {
    EventQueue events;
    GtkWidget* host = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    g_object_ref_sink(host);
    detail::GtkMenuBinding binding;
    binding.setHost(host, &events);
    Command command{"Open"};
    MenuBarModel bar;
    bar.appendMenu("File").appendCommand(command);
    binding.rebuild(bar);
    binding.install();
    GMenuModel* model = g_menu_model_get_item_link(binding.model(), 0, G_MENU_LINK_SUBMENU);
    CHECK(menuLabel(model, 0) == "Open");
    CHECK(g_action_group_get_action_enabled(binding.actionGroup(), "command-1"));

    command.setText("Open Recent");
    command.setEnabled(false);
    binding.refresh();
    CHECK(menuLabel(model, 0) == "Open Recent");
    CHECK(!g_action_group_get_action_enabled(binding.actionGroup(), "command-1"));
    command.setEnabled(true);
    binding.refresh();
    CHECK(g_action_group_get_action_enabled(binding.actionGroup(), "command-1"));

    command.setText(std::string{"bad\xFF", 4});
    binding.refresh();
    CHECK(menuLabel(model, 0) == "Open Recent");
    CHECK(!g_action_group_get_action_enabled(binding.actionGroup(), "command-1"));

    binding.clear();
    g_object_unref(host);
}

TEST_CASE("GTK stale action groups fail closed after structural resync") {
    EventQueue events;
    GtkWidget* host = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    g_object_ref_sink(host);
    detail::GtkMenuBinding binding;
    binding.setHost(host, &events);
    Command old_command{"Old"};
    Command new_command{"New"};
    MenuBarModel first;
    first.appendMenu("File").appendCommand(old_command);
    binding.rebuild(first);
    binding.install();
    GActionGroup* stale = G_ACTION_GROUP(g_object_ref(binding.actionGroup()));

    MenuBarModel second;
    second.appendMenu("File").appendCommand(new_command);
    binding.rebuild(second);
    binding.install();
    g_action_group_activate_action(stale, "command-1", nullptr);
    CHECK(!events.tryPop().has_value());
    g_action_group_activate_action(binding.actionGroup(), "command-1", nullptr);
    const auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<CommandActivationEvent>(*event).command.get() == &new_command);

    g_object_unref(stale);
    binding.clear();
    g_object_unref(host);
}
