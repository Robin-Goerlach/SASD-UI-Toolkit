#include <sasd/ui/native/gtk_backend.hpp>

#include "gtk_input_translation.hpp"
#include "gtk_file_dialog_service.hpp"
#include "gtk_menu_binding.hpp"
#include "gtk_render_context.hpp"

#include <sasd/ui/events/event_queue.hpp>
#include <sasd/ui/rendered/display_list_executor.hpp>

#include <gtk/gtk.h>

#include <cairo.h>

#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace sasd::ui::native::gtk {
namespace {

void requireFiniteSize(Size value) {
    if (value.width <= 0 || value.height <= 0) {
        throw std::invalid_argument{"GTK window initial size must be positive"};
    }
}

} // namespace

struct GtkWindowBackend::Impl {
    explicit Impl(GtkWindowBackendConfig value) : config{std::move(value)} {
        requireFiniteSize(config.initial_size);
        if (!std::isfinite(config.font_point_size) || config.font_point_size <= 0.0) {
            throw std::invalid_argument{"GTK font point size must be finite and positive"};
        }
        metric_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        metric_cairo = cairo_create(metric_surface);
        metric_context = std::make_unique<detail::GtkRenderContext>(*metric_cairo, fontDescription());
    }

    ~Impl() {
        shutdown();
        metric_context.reset();
        if (metric_cairo != nullptr) cairo_destroy(metric_cairo);
        if (metric_surface != nullptr) cairo_surface_destroy(metric_surface);
    }

    [[nodiscard]] std::string fontDescription() const {
        return "Sans " + std::to_string(config.font_point_size);
    }

    void requireInitialized(const char* operation) const {
        if (window == nullptr || area == nullptr) {
            throw std::logic_error{std::string{"GtkWindowBackend::"} + operation +
                                   " requires initialize()"};
        }
        if (owner_thread != std::this_thread::get_id()) {
            throw std::runtime_error{"GTK window backend must be used from its UI thread"};
        }
    }

    static gboolean closeRequest(GtkWindow*, gpointer data) noexcept {
        auto& self = *static_cast<Impl*>(data);
        self.events.push(Event{QuitEvent{}});
        return TRUE;
    }

    static gboolean keyPressed(GtkEventControllerKey*, guint keyval, guint,
                                GdkModifierType state, gpointer data) noexcept {
        auto& self = *static_cast<Impl*>(data);
        if (const auto key = detail::translateKey(keyval)) {
            self.events.push(Event{KeyEvent{*key, true, detail::translateModifiers(state)}});
        }
        if (const auto text = detail::committedText(keyval);
            text.has_value() && (state & (GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_META_MASK)) == 0) {
            self.events.push(Event{TextInputEvent{*text}});
        }
        return GDK_EVENT_PROPAGATE;
    }

    static gboolean keyReleased(GtkEventControllerKey*, guint keyval, guint,
                                 GdkModifierType state, gpointer data) noexcept {
        auto& self = *static_cast<Impl*>(data);
        if (const auto key = detail::translateKey(keyval)) {
            self.events.push(Event{KeyEvent{*key, false, detail::translateModifiers(state)}});
        }
        return GDK_EVENT_PROPAGATE;
    }

    static void buttonPressed(GtkGestureClick* gesture, int, double x, double y,
                              gpointer data) noexcept {
        auto& self = *static_cast<Impl*>(data);
        const guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
        GdkModifierType state = gtk_event_controller_get_current_event_state(
            GTK_EVENT_CONTROLLER(gesture));
        if (const auto event = detail::translatePointerButton(button, true, x, y, state)) {
            self.events.push(Event{*event});
        }
    }

    static void buttonReleased(GtkGestureClick* gesture, int, double x, double y,
                               gpointer data) noexcept {
        auto& self = *static_cast<Impl*>(data);
        const guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
        GdkModifierType state = gtk_event_controller_get_current_event_state(
            GTK_EVENT_CONTROLLER(gesture));
        if (const auto event = detail::translatePointerButton(button, false, x, y, state)) {
            self.events.push(Event{*event});
        }
    }

    static void motion(GtkEventControllerMotion*, double x, double y, gpointer data) noexcept {
        auto& self = *static_cast<Impl*>(data);
        if (const auto point = detail::translatePoint(x, y)) {
            self.events.push(Event{PointerEvent{*point, PointerAction::move, PointerButton::none,
                                                0, KeyModifier::none}});
        }
    }

    static void leave(GtkEventControllerMotion*, gpointer data) noexcept {
        static_cast<Impl*>(data)->events.push(Event{PointerSurfaceEvent{PointerSurfaceAction::left}});
    }

    static void draw(GtkDrawingArea*, cairo_t* cairo, int, int, gpointer data) noexcept {
        auto& self = *static_cast<Impl*>(data);
        if (!self.last_frame.has_value()) return;
        try {
            detail::GtkRenderContext context{*cairo, self.fontDescription()};
            context.clear(self.config.background_color);
            self.active_context = &context;
            (void)rendered::DisplayListExecutor::execute(*self.last_frame, context);
            self.active_context = nullptr;
            self.presentation_requested = false;
        } catch (...) {
            self.active_context = nullptr;
            self.presentation_requested = true;
        }
    }

    void initialize() {
        if (window != nullptr) throw std::logic_error{"GtkWindowBackend is already initialized"};
        if (gtk_init_check() == FALSE) throw std::runtime_error{"GTK initialization failed"};
        owner_thread = std::this_thread::get_id();

        GtkWindow* new_window = GTK_WINDOW(gtk_window_new());
        GtkWidget* new_area = gtk_drawing_area_new();
        GtkWidget* new_root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        if (new_window == nullptr || new_area == nullptr || new_root == nullptr) {
            if (new_window != nullptr) gtk_window_destroy(new_window);
            if (new_root != nullptr) g_object_unref(new_root);
            throw std::runtime_error{"GTK window creation failed"};
        }
        gtk_window_set_title(new_window, config.title.c_str());
        gtk_window_set_default_size(new_window, config.initial_size.width, config.initial_size.height);
        gtk_window_set_resizable(new_window, config.resizable);
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(new_area), &Impl::draw, this, nullptr);
        gtk_widget_set_hexpand(new_area, TRUE);
        gtk_widget_set_vexpand(new_area, TRUE);

        auto* keys = gtk_event_controller_key_new();
        g_signal_connect(keys, "key-pressed", G_CALLBACK(&Impl::keyPressed), this);
        g_signal_connect(keys, "key-released", G_CALLBACK(&Impl::keyReleased), this);
        gtk_widget_add_controller(new_area, keys);

        auto* clicks = gtk_gesture_click_new();
        gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(clicks), 0);
        g_signal_connect(clicks, "pressed", G_CALLBACK(&Impl::buttonPressed), this);
        g_signal_connect(clicks, "released", G_CALLBACK(&Impl::buttonReleased), this);
        gtk_widget_add_controller(new_area, GTK_EVENT_CONTROLLER(clicks));

        auto* motion_controller = gtk_event_controller_motion_new();
        g_signal_connect(motion_controller, "motion", G_CALLBACK(&Impl::motion), this);
        g_signal_connect(motion_controller, "leave", G_CALLBACK(&Impl::leave), this);
        gtk_widget_add_controller(new_area, motion_controller);

        g_signal_connect(new_window, "close-request", G_CALLBACK(&Impl::closeRequest), this);
        gtk_widget_set_vexpand(new_area, TRUE);
        gtk_box_append(GTK_BOX(new_root), new_area);
        gtk_window_set_child(new_window, new_root);
        window = new_window;
        root_box = new_root;
        area = new_area;
        menu_binding.setHost(root_box, &events);
        file_dialogs = std::make_unique<detail::GtkFileDialogService>(
            window, events, owner_thread);
        last_size = config.initial_size;
        if (!config.hidden) gtk_window_present(window);
    }

    void shutdown() noexcept {
        if (window == nullptr) return;
        // Invalidate the dialog completion state before the GtkWindow and EventQueue leave scope.
        if (file_dialogs != nullptr) {
            file_dialogs->shutdown();
            file_dialogs.reset();
        }
        owner_thread = std::this_thread::get_id();
        active_context = nullptr;
        menu_binding.clear();
        gtk_window_destroy(window);
        g_object_unref(window);
        window = nullptr;
        root_box = nullptr;
        area = nullptr;
        last_frame.reset();
        presentation_requested = false;
    }

    GtkWindowBackendConfig config;
    GtkWindow* window{nullptr};
    GtkWidget* root_box{nullptr};
    GtkWidget* area{nullptr};
    Size last_size{};
    std::thread::id owner_thread{};
    EventQueue events;
    std::optional<rendered::DisplayList> last_frame;
    bool presentation_requested{false};
    detail::GtkRenderContext* active_context{nullptr};
    cairo_surface_t* metric_surface{nullptr};
    cairo_t* metric_cairo{nullptr};
    std::unique_ptr<detail::GtkRenderContext> metric_context;
    detail::GtkMenuBinding menu_binding;
    std::unique_ptr<detail::GtkFileDialogService> file_dialogs;
};

GtkWindowBackend::GtkWindowBackend(GtkWindowBackendConfig config)
    : impl_{std::make_unique<Impl>(std::move(config))} {}

GtkWindowBackend::~GtkWindowBackend() = default;

BackendCapabilities GtkWindowBackend::capabilities() const noexcept {
    BackendCapabilities result;
    result.pointer_input = true;
    result.native_menus = true;
    result.file_dialogs = true;
    return result;
}

FileDialogService* GtkWindowBackend::fileDialogs() noexcept {
    return impl_->window != nullptr ? impl_->file_dialogs.get() : nullptr;
}

const FileDialogService* GtkWindowBackend::fileDialogs() const noexcept {
    return impl_->window != nullptr ? impl_->file_dialogs.get() : nullptr;
}

void GtkWindowBackend::initialize() { impl_->initialize(); }
void GtkWindowBackend::shutdown() noexcept { impl_->shutdown(); }

std::optional<Event> GtkWindowBackend::pollEvent() {
    impl_->requireInitialized("pollEvent");
    while (g_main_context_pending(nullptr)) (void)g_main_context_iteration(nullptr, FALSE);
    impl_->menu_binding.refresh();
    const Size current{static_cast<Coordinate>(gtk_widget_get_width(impl_->area)),
                       static_cast<Coordinate>(gtk_widget_get_height(impl_->area))};
    if (current.width > 0 && current.height > 0 && current != impl_->last_size) {
        impl_->last_size = current;
        impl_->presentation_requested = true;
        return Event{ResizeEvent{current}};
    }
    return impl_->events.tryPop();
}

void GtkWindowBackend::syncMenuBar(const MenuBarModel& menu_bar) {
    impl_->requireInitialized("syncMenuBar");
    if (menu_bar.menuCount() == 0) {
        clearMenuBar();
        return;
    }

    detail::GtkMenuBinding replacement;
    replacement.setHost(impl_->root_box, &impl_->events);
    replacement.rebuild(menu_bar);
    replacement.install();
    // The old projection remains installed until the new tree is fully built and appended. Swapping
    // first lets the temporary binding's destructor detach exactly the old widget afterwards.
    impl_->menu_binding.swap(replacement);
}

void GtkWindowBackend::clearMenuBar() noexcept {
    if (impl_->window == nullptr) return;
    impl_->menu_binding.clear();
}

bool GtkWindowBackend::isInitialized() const noexcept { return impl_->window != nullptr; }

Size GtkWindowBackend::windowSize() const {
    impl_->requireInitialized("windowSize");
    return impl_->last_size;
}

bool GtkWindowBackend::presentationRequested() const noexcept { return impl_->presentation_requested; }

std::size_t GtkWindowBackend::presentFrame(const rendered::DisplayList& list) {
    impl_->requireInitialized("presentFrame");
    impl_->last_frame = list;
    impl_->presentation_requested = true;
    gtk_widget_queue_draw(impl_->area);
    return list.size();
}

void GtkWindowBackend::fillRect(const rendered::FillRectCommand& command) {
    if (impl_->active_context == nullptr) throw std::logic_error{"GTK fillRect outside draw callback"};
    impl_->active_context->fillRect(command);
}
void GtkWindowBackend::strokeRect(const rendered::StrokeRectCommand& command) {
    if (impl_->active_context == nullptr) throw std::logic_error{"GTK strokeRect outside draw callback"};
    impl_->active_context->strokeRect(command);
}
void GtkWindowBackend::drawText(const rendered::DrawTextCommand& command) {
    if (impl_->active_context == nullptr) throw std::logic_error{"GTK drawText outside draw callback"};
    impl_->active_context->drawText(command);
}

Size GtkWindowBackend::measureText(std::string_view text) const { return impl_->metric_context->measureText(text); }
Coordinate GtkWindowBackend::lineHeight() const noexcept { return impl_->metric_context->lineHeight(); }
std::optional<Coordinate> GtkWindowBackend::textAdvanceToScalar(std::string_view text,
                                                                std::size_t scalar_index) const {
    return impl_->metric_context->textAdvanceToScalar(text, scalar_index);
}
std::uint64_t GtkWindowBackend::revision() const noexcept { return 1; }

} // namespace sasd::ui::native::gtk
