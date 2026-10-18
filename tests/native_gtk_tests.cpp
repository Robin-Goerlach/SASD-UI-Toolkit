#include "test_framework.hpp"

#include "gtk_menu_binding.hpp"
#include "gtk_file_dialog_service.hpp"
#include "gtk_render_context.hpp"

#include <sasd/ui/native/gtk_backend.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/application.hpp>

#include <memory>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

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

namespace {

struct FakeGtkInvokerState {
    struct Invocation {
        GtkWindow* owner{};
        std::string title{};
        std::string suggested_filename{};
        detail::NativeOpenFileCompletion completion{};
    };
    std::vector<Invocation> invocations{};
    bool cancelled{false};
    std::vector<std::string> extensions{};
};

class FakeGtkInvoker final : public detail::NativeOpenFileInvoker {
public:
    explicit FakeGtkInvoker(std::shared_ptr<FakeGtkInvokerState> state) : state_{std::move(state)} {}

    void begin(GtkWindow* owner, std::string title, std::vector<std::string> extensions,
               detail::NativeOpenFileCompletion completion) override {
        state_->invocations.push_back({owner, std::move(title), {}, std::move(completion)});
        state_->extensions = std::move(extensions);
    }

    void cancel() noexcept override { state_->cancelled = true; }

private:
    std::shared_ptr<FakeGtkInvokerState> state_;
};

class FakeGtkSaveInvoker final : public detail::NativeSaveFileInvoker {
public:
    explicit FakeGtkSaveInvoker(std::shared_ptr<FakeGtkInvokerState> state) : state_{std::move(state)} {}

    void begin(GtkWindow* owner, std::string title, std::vector<std::string> extensions,
               std::string suggested_filename,
               detail::NativeSaveFileCompletion completion) override {
        state_->invocations.push_back({owner, std::move(title), std::move(suggested_filename),
                                       std::move(completion)});
        state_->extensions = std::move(extensions);
    }

    void cancel() noexcept override { state_->cancelled = true; }

private:
    std::shared_ptr<FakeGtkInvokerState> state_;
};

class FakeGtkSelectFolderInvoker final : public detail::NativeSelectFolderInvoker {
public:
    explicit FakeGtkSelectFolderInvoker(std::shared_ptr<FakeGtkInvokerState> state)
        : state_{std::move(state)} {}

    void begin(GtkWindow* owner, std::string title,
               detail::NativeSelectFolderCompletion completion) override {
        state_->invocations.push_back({owner, std::move(title), {}, std::move(completion)});
    }

    void cancel() noexcept override { state_->cancelled = true; }

private:
    std::shared_ptr<FakeGtkInvokerState> state_;
};

void completeFake(const std::shared_ptr<FakeGtkInvokerState>& state, std::size_t index,
                  FileDialogResultStatus status, GFile* file = nullptr) {
    auto completion = state->invocations.at(index).completion;
    completion(status, file);
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

TEST_CASE("GTK file-dialog service is asynchronous and has the shared lifecycle contract") {
    GtkWindowBackend backend{{"SASD GTK file test", {320, 200}, true, true, 12.0, Color::black}};
    CHECK(backend.capabilities().pointer_input);
    CHECK(backend.capabilities().native_menus);
    CHECK(backend.capabilities().file_dialogs);
    CHECK(backend.fileDialogs() == nullptr);
    backend.initialize();
    CHECK(backend.fileDialogs() != nullptr);

    backend.shutdown();
    CHECK(backend.fileDialogs() == nullptr);
}

TEST_CASE("GTK file-dialog fake covers async completion, exactly-once, stale generation and paths") {
    EventQueue events;
    auto fake_state = std::make_shared<FakeGtkInvokerState>();
    auto fake = std::make_unique<FakeGtkInvoker>(fake_state);
    detail::GtkFileDialogService service{reinterpret_cast<GtkWindow*>(0x1), events,
                                         std::this_thread::get_id(), std::move(fake)};

    const auto first = service.requestOpenFile({"Open 😀"});
    CHECK(first != 0);
    CHECK(service.pending());
    CHECK(fake_state->invocations.size() == 1);
    CHECK(fake_state->invocations[0].title == "Open 😀");
    bool second_rejected = false;
    try {
        (void)service.requestOpenFile({});
    } catch (const std::logic_error&) {
        second_rejected = true;
    }
    CHECK(second_rejected);
    CHECK(!events.tryPop().has_value());

    GFile* selected = g_file_new_for_path("/tmp/GTK file Grüße.txt");
    completeFake(fake_state, 0, FileDialogResultStatus::accepted, selected);
    completeFake(fake_state, 0, FileDialogResultStatus::accepted, selected);
    g_object_unref(selected);
    CHECK(!service.pending());
    const auto accepted = events.tryPop();
    CHECK(accepted.has_value());
    CHECK(std::get<FileDialogResultEvent>(*accepted).request_id == first);
    CHECK(std::get<FileDialogResultEvent>(*accepted).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*accepted).path ==
          std::filesystem::path{"/tmp/GTK file Grüße.txt"});
    CHECK(!events.tryPop().has_value());

    const auto second = service.requestOpenFile({});
    CHECK(second != first);
    completeFake(fake_state, 0, FileDialogResultStatus::accepted, nullptr);
    CHECK(service.pending());
    CHECK(!events.tryPop().has_value());
    completeFake(fake_state, 1, FileDialogResultStatus::cancelled);
    CHECK(!service.pending());
    const auto cancelled = events.tryPop();
    CHECK(cancelled.has_value());
    CHECK(std::get<FileDialogResultEvent>(*cancelled).request_id == second);
    CHECK(std::get<FileDialogResultEvent>(*cancelled).status == FileDialogResultStatus::cancelled);
}

TEST_CASE("GTK file-dialog rejects malformed titles and non-local results") {
    EventQueue events;
    auto fake_state = std::make_shared<FakeGtkInvokerState>();
    auto fake = std::make_unique<FakeGtkInvoker>(fake_state);
    detail::GtkFileDialogService service{reinterpret_cast<GtkWindow*>(0x1), events,
                                         std::this_thread::get_id(), std::move(fake)};
    bool rejected = false;
    try {
        (void)service.requestOpenFile({std::string{"bad\xFF", 4}});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    CHECK(rejected);
    CHECK(fake_state->invocations.empty());

    const auto id = service.requestOpenFile({});
    GFile* remote = g_file_new_for_uri("https://example.invalid/file.txt");
    completeFake(fake_state, 0, FileDialogResultStatus::accepted, remote);
    g_object_unref(remote);
    const auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<FileDialogResultEvent>(*event).request_id == id);
    CHECK(std::get<FileDialogResultEvent>(*event).status == FileDialogResultStatus::failed);
    CHECK(!std::get<FileDialogResultEvent>(*event).path.has_value());
}

TEST_CASE("GTK open extension filters validate before ID and preserve literal order") {
    EventQueue events;
    auto state = std::make_shared<FakeGtkInvokerState>();
    auto fake = std::make_unique<FakeGtkInvoker>(state);
    detail::GtkFileDialogService service{reinterpret_cast<GtkWindow*>(0x1), events,
                                         std::this_thread::get_id(), std::move(fake)};
    bool rejected = false;
    try { (void)service.requestOpenFile({"Open", {"foo/bar"}}); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    CHECK(state->invocations.empty());
    const auto id = service.requestOpenFile({"Open", {"txt", "md", "cpp"}});
    CHECK(id == 1);
    CHECK(state->extensions == std::vector<std::string>{"txt", "md", "cpp"});
    GFile* mismatching = g_file_new_for_path("/tmp/report.bin");
    completeFake(state, 0, FileDialogResultStatus::accepted, mismatching);
    g_object_unref(mismatching);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).status == FileDialogResultStatus::accepted);
}

TEST_CASE("GTK save file uses one shared pending boundary and preserves new targets") {
    EventQueue events;
    auto open_state = std::make_shared<FakeGtkInvokerState>();
    auto save_state = std::make_shared<FakeGtkInvokerState>();
    detail::GtkFileDialogService service{
        reinterpret_cast<GtkWindow*>(0x1), events, std::this_thread::get_id(),
        std::make_unique<FakeGtkInvoker>(open_state),
        std::make_unique<FakeGtkSaveInvoker>(save_state)};

    const auto save_id = service.requestSaveFile({"Save 😀", {"txt", "md"}, "report.md"});
    CHECK(save_id != 0);
    CHECK(service.pending());
    CHECK(save_state->invocations.size() == 1);
    CHECK(save_state->extensions == std::vector<std::string>{"txt", "md"});
    CHECK(save_state->invocations[0].suggested_filename == "report.md");
    bool open_rejected = false;
    try { (void)service.requestOpenFile({}); } catch (const std::logic_error&) { open_rejected = true; }
    CHECK(open_rejected);

    GFile* new_target = g_file_new_for_path("/tmp/SASD UI Toolkit new GTK save file.txt");
    completeFake(save_state, 0, FileDialogResultStatus::accepted, new_target);
    g_object_unref(new_target);
    CHECK(!service.pending());
    auto accepted = events.tryPop();
    CHECK(accepted.has_value());
    CHECK(std::get<FileDialogResultEvent>(*accepted).request_id == save_id);
    CHECK(std::get<FileDialogResultEvent>(*accepted).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*accepted).path ==
          std::filesystem::path{"/tmp/SASD UI Toolkit new GTK save file.txt"});
    CHECK(!events.tryPop().has_value());

    const auto open_id = service.requestOpenFile({});
    CHECK(open_id > save_id);
    completeFake(open_state, 0, FileDialogResultStatus::cancelled);
    CHECK(events.tryPop().has_value());
}

TEST_CASE("GTK save filters validate before ID and preserve order without rewriting results") {
    EventQueue events;
    auto open_state = std::make_shared<FakeGtkInvokerState>();
    auto save_state = std::make_shared<FakeGtkInvokerState>();
    detail::GtkFileDialogService service{
        reinterpret_cast<GtkWindow*>(0x1), events, std::this_thread::get_id(),
        std::make_unique<FakeGtkInvoker>(open_state),
        std::make_unique<FakeGtkSaveInvoker>(save_state)};
    bool suggested_rejected = false;
    try { (void)service.requestSaveFile({"Save", {}, "foo/bar"}); }
    catch (const std::invalid_argument&) { suggested_rejected = true; }
    CHECK(suggested_rejected);
    CHECK(save_state->invocations.empty());
    bool rejected = false;
    try { (void)service.requestSaveFile({"Save", {"foo/bar"}}); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    CHECK(save_state->invocations.empty());
    const auto id = service.requestSaveFile({"Save", {"txt", "md", "cpp"}});
    CHECK(id == 1);
    CHECK(save_state->extensions == std::vector<std::string>{"txt", "md", "cpp"});
    GFile* mismatching = g_file_new_for_path("/tmp/report.bin");
    completeFake(save_state, 0, FileDialogResultStatus::accepted, mismatching);
    g_object_unref(mismatching);
    const auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<FileDialogResultEvent>(*event).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*event).path == std::filesystem::path{"/tmp/report.bin"});
}

TEST_CASE("GTK save file maps cancel and failure and ignores double completion") {
    EventQueue events;
    auto save_state = std::make_shared<FakeGtkInvokerState>();
    detail::GtkFileDialogService service{
        reinterpret_cast<GtkWindow*>(0x1), events, std::this_thread::get_id(),
        std::make_unique<FakeGtkInvoker>(std::make_shared<FakeGtkInvokerState>()),
        std::make_unique<FakeGtkSaveInvoker>(save_state)};
    const auto cancelled_id = service.requestSaveFile({});
    completeFake(save_state, 0, FileDialogResultStatus::cancelled);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == cancelled_id);

    const auto failed_id = service.requestSaveFile({});
    completeFake(save_state, 1, FileDialogResultStatus::failed);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == failed_id);
    CHECK(!service.pending());

    completeFake(save_state, 1, FileDialogResultStatus::accepted, nullptr);
    CHECK(!events.tryPop().has_value());
}

TEST_CASE("GTK folder selection is local, single, shared and exactly-once") {
    EventQueue events;
    auto open_state = std::make_shared<FakeGtkInvokerState>();
    auto save_state = std::make_shared<FakeGtkInvokerState>();
    auto folder_state = std::make_shared<FakeGtkInvokerState>();
    detail::GtkFileDialogService service{
        reinterpret_cast<GtkWindow*>(0x1), events, std::this_thread::get_id(),
        std::make_unique<FakeGtkInvoker>(open_state),
        std::make_unique<FakeGtkSaveInvoker>(save_state),
        std::make_unique<FakeGtkSelectFolderInvoker>(folder_state)};

    const auto folder_id = service.requestSelectFolder({"Select Folder 😀"});
    CHECK(folder_id == 1);
    CHECK(service.pending());
    bool open_rejected = false;
    bool save_rejected = false;
    bool folder_rejected = false;
    try { (void)service.requestOpenFile({}); } catch (const std::logic_error&) { open_rejected = true; }
    try { (void)service.requestSaveFile({}); } catch (const std::logic_error&) { save_rejected = true; }
    try { (void)service.requestSelectFolder({}); } catch (const std::logic_error&) { folder_rejected = true; }
    CHECK(open_rejected);
    CHECK(save_rejected);
    CHECK(folder_rejected);

    GFile* selected = g_file_new_for_path("/tmp/Existing Folder/Grüße 📁");
    completeFake(folder_state, 0, FileDialogResultStatus::accepted, selected);
    completeFake(folder_state, 0, FileDialogResultStatus::accepted, selected);
    g_object_unref(selected);
    CHECK(!service.pending());
    auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<FileDialogResultEvent>(*event).request_id == folder_id);
    CHECK(std::get<FileDialogResultEvent>(*event).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*event).path ==
          std::filesystem::path{"/tmp/Existing Folder/Grüße 📁"});
    CHECK(!events.tryPop().has_value());

    const auto save_id = service.requestSaveFile({});
    completeFake(save_state, 0, FileDialogResultStatus::cancelled);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == save_id);
}

TEST_CASE("GTK open save folder open keep one monotone request-ID sequence") {
    EventQueue events;
    auto open_state = std::make_shared<FakeGtkInvokerState>();
    auto save_state = std::make_shared<FakeGtkInvokerState>();
    auto folder_state = std::make_shared<FakeGtkInvokerState>();
    detail::GtkFileDialogService service{
        reinterpret_cast<GtkWindow*>(0x1), events, std::this_thread::get_id(),
        std::make_unique<FakeGtkInvoker>(open_state),
        std::make_unique<FakeGtkSaveInvoker>(save_state),
        std::make_unique<FakeGtkSelectFolderInvoker>(folder_state)};

    const auto open_one = service.requestOpenFile({});
    completeFake(open_state, 0, FileDialogResultStatus::cancelled);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == open_one);
    const auto save = service.requestSaveFile({});
    completeFake(save_state, 0, FileDialogResultStatus::cancelled);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == save);
    const auto folder = service.requestSelectFolder({});
    completeFake(folder_state, 0, FileDialogResultStatus::cancelled);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == folder);
    const auto open_two = service.requestOpenFile({});
    completeFake(open_state, 1, FileDialogResultStatus::cancelled);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == open_two);
    CHECK(open_one < save);
    CHECK(save < folder);
    CHECK(folder < open_two);
}

TEST_CASE("GTK file-dialog ignores completion after shutdown and destruction") {
    EventQueue events;
    auto fake_state = std::make_shared<FakeGtkInvokerState>();
    {
        auto fake = std::make_unique<FakeGtkInvoker>(fake_state);
        detail::GtkFileDialogService service{reinterpret_cast<GtkWindow*>(0x1), events,
                                             std::this_thread::get_id(), std::move(fake)};
        (void)service.requestOpenFile({});
        service.shutdown();
        CHECK(fake_state->cancelled);
        completeFake(fake_state, 0, FileDialogResultStatus::cancelled);
    }
    CHECK(!events.tryPop().has_value());

    auto destruction_state = std::make_shared<FakeGtkInvokerState>();
    {
        auto fake = std::make_unique<FakeGtkInvoker>(destruction_state);
        auto service = std::make_unique<detail::GtkFileDialogService>(
            reinterpret_cast<GtkWindow*>(0x1), events, std::this_thread::get_id(), std::move(fake));
        (void)service->requestOpenFile({});
    }
    completeFake(destruction_state, 0, FileDialogResultStatus::cancelled);
    CHECK(!events.tryPop().has_value());
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
