#include "test_framework.hpp"

#include "appkit_input_translation.hpp"
#include "appkit_file_dialog_service.hpp"
#include "appkit_render_context.hpp"

#include <sasd/ui/native/appkit_backend.hpp>
#include <sasd/ui/application.hpp>
#include <sasd/ui/menu_model.hpp>

#import <AppKit/AppKit.h>

#include <memory>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::native::appkit;

namespace {
struct FakeOpenFileState {
    std::vector<detail::NativeOpenFileCompletion> completions;
    std::optional<std::filesystem::path> initial_directory;
    int begins{0};
};

class FakeOpenFileInvoker final : public detail::NativeOpenFileInvoker {
public:
    explicit FakeOpenFileInvoker(std::shared_ptr<FakeOpenFileState> state) : state_{std::move(state)} {}
    void begin(NSWindow*, NSString* title, std::vector<std::string> extensions,
               const std::optional<std::filesystem::path>& initial_directory,
               detail::NativeOpenFileCompletion completion) override {
        title_ = title == nil ? std::string{} : std::string{title.UTF8String};
        extensions_ = std::move(extensions);
        initial_directory_ = initial_directory;
        state_->initial_directory = initial_directory;
        ++state_->begins;
        state_->completions.push_back(std::move(completion));
    }
    void cancel() noexcept override { ++cancels; }
    std::shared_ptr<FakeOpenFileState> state_;
    std::string title_;
    std::vector<std::string> extensions_;
    std::optional<std::filesystem::path> initial_directory_;
    int cancels{0};
};

struct FakeOpenFilesState {
    std::vector<detail::NativeOpenFilesCompletion> completions;
    std::optional<std::filesystem::path> initial_directory;
    int begins{0};
};

class FakeOpenFilesInvoker final : public detail::NativeOpenFilesInvoker {
public:
    explicit FakeOpenFilesInvoker(std::shared_ptr<FakeOpenFilesState> state)
        : state_{std::move(state)} {}
    void begin(NSWindow*, NSString* title, std::vector<std::string> extensions,
               const std::optional<std::filesystem::path>& initial_directory,
               detail::NativeOpenFilesCompletion completion) override {
        title_ = title == nil ? std::string{} : std::string{title.UTF8String};
        extensions_ = std::move(extensions);
        initial_directory_ = initial_directory;
        state_->initial_directory = initial_directory;
        ++state_->begins;
        state_->completions.push_back(std::move(completion));
    }
    void cancel() noexcept override { ++cancels; }
    std::shared_ptr<FakeOpenFilesState> state_;
    std::string title_;
    std::vector<std::string> extensions_;
    std::optional<std::filesystem::path> initial_directory_;
    int cancels{0};
};

struct FakeSaveFileState {
    std::vector<detail::NativeSaveFileCompletion> completions;
    std::vector<std::string> extensions;
    std::string suggested_filename;
    std::optional<std::filesystem::path> initial_directory;
    int begins{0};
};

class FakeSaveFileInvoker final : public detail::NativeSaveFileInvoker {
public:
    explicit FakeSaveFileInvoker(std::shared_ptr<FakeSaveFileState> state) : state_{std::move(state)} {}
    void begin(NSWindow*, NSString* title, std::vector<std::string> extensions,
               NSString* suggested_filename,
               const std::optional<std::filesystem::path>& initial_directory,
               detail::NativeSaveFileCompletion completion) override {
        title_ = title == nil ? std::string{} : std::string{title.UTF8String};
        state_->extensions = std::move(extensions);
        state_->suggested_filename = suggested_filename == nil ? std::string{} :
            std::string{suggested_filename.UTF8String};
        state_->initial_directory = initial_directory;
        ++state_->begins;
        state_->completions.push_back(std::move(completion));
    }
    void cancel() noexcept override { ++cancels; }
    std::shared_ptr<FakeSaveFileState> state_;
    std::string title_;
    int cancels{0};
};

struct FakeSelectFolderState {
    std::vector<detail::NativeSelectFolderCompletion> completions;
    std::optional<std::filesystem::path> initial_directory;
    int begins{0};
};

class FakeSelectFolderInvoker final : public detail::NativeSelectFolderInvoker {
public:
    explicit FakeSelectFolderInvoker(std::shared_ptr<FakeSelectFolderState> state)
        : state_{std::move(state)} {}
    void begin(NSWindow*, NSString* title,
               const std::optional<std::filesystem::path>& initial_directory,
               detail::NativeSelectFolderCompletion completion) override {
        title_ = title == nil ? std::string{} : std::string{title.UTF8String};
        initial_directory_ = initial_directory;
        state_->initial_directory = initial_directory;
        ++state_->begins;
        state_->completions.push_back(std::move(completion));
    }
    void cancel() noexcept override { ++cancels; }
    std::shared_ptr<FakeSelectFolderState> state_;
    std::string title_;
    std::optional<std::filesystem::path> initial_directory_;
    int cancels{0};
};

NSWindow* testOwner() {
    return [[NSWindow alloc] initWithContentRect:NSZeroRect styleMask:0
                                          backing:NSBackingStoreBuffered defer:YES];
}
} // namespace

TEST_CASE("AppKit value translation preserves Core key and logical point contracts") {
    CHECK(detail::translateKey(36) == Key::enter);
    CHECK(detail::translateKey(53) == Key::escape);
    CHECK(detail::translateKey(123) == Key::left);
    CHECK(detail::translateKey(122) == Key::f1);
    CHECK(detail::translateLogicalLetter('A') == Key::a);
    CHECK(!detail::translateLogicalLetter(0x00E4).has_value());
    CHECK(detail::translateModifiers((1ULL << 17U) | (1ULL << 20U)) ==
          (KeyModifier::shift | KeyModifier::meta));
    CHECK(detail::translatePoint(12.5, 18.5) == sasd::ui::Point{12, 18});
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
    CHECK(backend.capabilities().native_menus);
    CHECK(!backend.isInitialized());
    backend.initialize();
    CHECK(backend.isInitialized());
    CHECK(backend.windowSize() == sasd::ui::Size{320, 200});
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

TEST_CASE("AppKit file dialog advertises lifecycle and native capabilities") {
    AppKitWindowBackend backend{{"SASD AppKit dialog capability test", {320, 200}, true, false}};
    CHECK(backend.capabilities().pointer_input);
    CHECK(backend.capabilities().native_menus);
    CHECK(backend.capabilities().file_dialogs);
    CHECK(backend.fileDialogs() == nullptr);
    backend.initialize();
    CHECK(backend.fileDialogs() != nullptr);
    backend.shutdown();
    CHECK(backend.fileDialogs() == nullptr);
}

TEST_CASE("AppKit file dialog is asynchronous, exact-once and reusable after queued completion") {
    EventQueue events;
    auto fake_state = std::make_shared<FakeOpenFileState>();
    detail::AppKitFileDialogService service{testOwner(), events,
        std::make_unique<FakeOpenFileInvoker>(fake_state)};
    const auto first = service.requestOpenFile({"Open 😀"});
    CHECK(first != 0);
    CHECK(service.pending());
    CHECK(fake_state->begins == 1);
    CHECK(!events.tryPop().has_value());
    bool second_rejected = false;
    try { (void)service.requestOpenFile({}); } catch (const std::logic_error&) { second_rejected = true; }
    CHECK(second_rejected);

    NSURL* selected = [NSURL fileURLWithPath:@"/tmp/Grüße long file name.txt"];
    fake_state->completions[0](NSModalResponseOK, selected);
    fake_state->completions[0](NSModalResponseOK, selected);
    CHECK(!service.pending());
    auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<FileDialogResultEvent>(*event).request_id == first);
    CHECK(std::get<FileDialogResultEvent>(*event).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*event).path ==
          std::filesystem::path{selected.fileSystemRepresentation});
    CHECK(!events.tryPop().has_value());

    const auto second = service.requestOpenFile({});
    CHECK(second != first);
    fake_state->completions[1](NSModalResponseCancel, nil);
    const auto cancelled = events.tryPop();
    CHECK(cancelled.has_value());
    CHECK(std::get<FileDialogResultEvent>(*cancelled).request_id == second);
    CHECK(std::get<FileDialogResultEvent>(*cancelled).status == FileDialogResultStatus::cancelled);

    const auto third = service.requestOpenFile({});
    fake_state->completions[2](static_cast<NSModalResponse>(-1), nil);
    const auto failed = events.tryPop();
    CHECK(failed.has_value());
    CHECK(std::get<FileDialogResultEvent>(*failed).request_id == third);
    CHECK(std::get<FileDialogResultEvent>(*failed).status == FileDialogResultStatus::failed);
    CHECK(!std::get<FileDialogResultEvent>(*failed).path.has_value());
}

TEST_CASE("AppKit open extension filters validate before ID and preserve order") {
    EventQueue events;
    auto fake = std::make_unique<FakeOpenFileInvoker>(std::make_shared<FakeOpenFileState>());
    FakeOpenFileInvoker* fake_ptr = fake.get();
    detail::AppKitFileDialogService service{testOwner(), events, std::move(fake)};
    bool rejected = false;
    try { (void)service.requestOpenFile({"Open", {"*.txt"}}); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    CHECK(fake_ptr->state_->begins == 0);
    const auto id = service.requestOpenFile({"Open", {"txt", "md", "cpp"}});
    CHECK(id == 1);
    CHECK(fake_ptr->state_->begins == 1);
    CHECK(fake_ptr->extensions_ == std::vector<std::string>{"txt", "md", "cpp"});
}

TEST_CASE("AppKit multiple open preserves native URL order and the plural result contract") {
    EventQueue events;
    auto open_state = std::make_shared<FakeOpenFileState>();
    auto open_files_state = std::make_shared<FakeOpenFilesState>();
    auto open_files_fake = std::make_unique<FakeOpenFilesInvoker>(open_files_state);
    FakeOpenFilesInvoker* open_files_ptr = open_files_fake.get();
    detail::AppKitFileDialogService service{
        testOwner(), events,
        std::make_unique<FakeOpenFileInvoker>(open_state), nullptr, nullptr,
        std::move(open_files_fake)};

    const auto id = service.requestOpenFiles({"Open Multiple", {"txt", "md"}});
    CHECK(id == 1);
    CHECK(open_files_state->begins == 1);
    CHECK(open_files_state->completions.size() == 1);
    CHECK(open_files_ptr->extensions_ == std::vector<std::string>{"txt", "md"});

    NSArray<NSURL*>* selected = @[
        [NSURL fileURLWithPath:@"/tmp/one.txt"],
        [NSURL fileURLWithPath:@"/tmp/two.md"]
    ];
    open_files_state->completions[0](NSModalResponseOK, selected);

    const auto event = events.tryPop();
    CHECK(event.has_value());
    const auto& result = std::get<OpenFilesDialogResultEvent>(*event);
    CHECK(result.request_id == id);
    CHECK(result.status == FileDialogResultStatus::accepted);
    CHECK(result.paths == std::vector<std::filesystem::path>{
        std::filesystem::path{"/tmp/one.txt"}, std::filesystem::path{"/tmp/two.md"}});
    CHECK(!service.pending());
    CHECK(!events.tryPop().has_value());
}

TEST_CASE("AppKit file dialog rejects malformed title before native invocation") {
    EventQueue events;
    auto state = std::make_shared<FakeOpenFileState>();
    detail::AppKitFileDialogService service{testOwner(), events,
        std::make_unique<FakeOpenFileInvoker>(state)};
    bool rejected = false;
    try { (void)service.requestOpenFile({"\xFF"}); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    CHECK(state->begins == 0);
}

TEST_CASE("AppKit save file reuses the async result contract and shared pending state") {
    EventQueue events;
    auto open_state = std::make_shared<FakeOpenFileState>();
    auto save_state = std::make_shared<FakeSaveFileState>();
    detail::AppKitFileDialogService service{
        testOwner(), events,
        std::make_unique<FakeOpenFileInvoker>(open_state),
        std::make_unique<FakeSaveFileInvoker>(save_state)};

    const auto save_id = service.requestSaveFile({"Save 😀", {"txt", "md"}, "report.md"});
    CHECK(save_id != 0);
    CHECK(service.pending());
    CHECK(save_state->begins == 1);
    CHECK(save_state->completions.size() == 1);
    CHECK(save_state->completions[0]);
    CHECK(save_state->completions[0] != nullptr);
    CHECK(save_state->extensions == std::vector<std::string>{"txt", "md"});
    CHECK(save_state->suggested_filename == "report.md");
    CHECK(!events.tryPop().has_value());

    bool open_rejected = false;
    try { (void)service.requestOpenFile({}); } catch (const std::logic_error&) { open_rejected = true; }
    CHECK(open_rejected);

    NSURL* new_target = [NSURL fileURLWithPath:@"/tmp/SASD UI Toolkit new save file.txt"];
    save_state->completions[0](NSModalResponseOK, new_target);
    CHECK(!service.pending());
    auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<FileDialogResultEvent>(*event).request_id == save_id);
    CHECK(std::get<FileDialogResultEvent>(*event).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*event).path ==
          std::filesystem::path{new_target.fileSystemRepresentation});
    CHECK(!events.tryPop().has_value());

    const auto open_id = service.requestOpenFile({});
    CHECK(open_id > save_id);
    open_state->completions[0](NSModalResponseCancel, nil);
    CHECK(events.tryPop().has_value());
}

TEST_CASE("AppKit initial directories propagate independently to all panels") {
    EventQueue events;
    auto open_state = std::make_shared<FakeOpenFileState>();
    auto save_state = std::make_shared<FakeSaveFileState>();
    auto folder_state = std::make_shared<FakeSelectFolderState>();
    detail::AppKitFileDialogService service{
        testOwner(), events,
        std::make_unique<FakeOpenFileInvoker>(open_state),
        std::make_unique<FakeSaveFileInvoker>(save_state),
        std::make_unique<FakeSelectFolderInvoker>(folder_state)};
    const std::filesystem::path directory{"/Users/Robin/Documents/Grüße"};

    const auto open_id = service.requestOpenFile({"Open", {}, directory});
    open_state->completions[0](NSModalResponseOK, [NSURL fileURLWithPath:@"/Users/Other/selected.txt"]);
    const auto save_id = service.requestSaveFile({"Save", {}, "notes.md", directory});
    save_state->completions[0](NSModalResponseCancel, nil);
    const auto folder_id = service.requestSelectFolder({"Folder", directory});
    folder_state->completions[0](NSModalResponseCancel, nil);

    CHECK(open_state->initial_directory == directory);
    CHECK(save_state->initial_directory == directory);
    CHECK(folder_state->initial_directory == directory);
    const auto open_event = events.tryPop();
    CHECK(std::get<FileDialogResultEvent>(*open_event).request_id == open_id);
    CHECK(std::get<FileDialogResultEvent>(*open_event).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*open_event).path ==
          std::filesystem::path{"/Users/Other/selected.txt"});
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == save_id);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == folder_id);
}

TEST_CASE("AppKit initial-directory validation rejects relative and empty paths before native begin") {
    EventQueue events;
    auto state = std::make_shared<FakeOpenFileState>();
    detail::AppKitFileDialogService service{
        testOwner(), events, std::make_unique<FakeOpenFileInvoker>(state)};
    for (const auto& directory : {std::filesystem::path{}, std::filesystem::path{"relative"}}) {
        bool rejected = false;
        try { (void)service.requestOpenFile({"Open", {}, directory}); }
        catch (const std::invalid_argument&) { rejected = true; }
        CHECK(rejected);
    }
    CHECK(state->begins == 0);
}

TEST_CASE("AppKit save filters validate before native begin and preserve order") {
    EventQueue events;
    auto state = std::make_shared<FakeSaveFileState>();
    auto fake = std::make_unique<FakeSaveFileInvoker>(state);
    detail::AppKitFileDialogService service{
        testOwner(), events, std::make_unique<FakeOpenFileInvoker>(std::make_shared<FakeOpenFileState>()),
        std::move(fake)};
    bool suggested_rejected = false;
    try { (void)service.requestSaveFile({"Save", {}, "foo/bar"}); }
    catch (const std::invalid_argument&) { suggested_rejected = true; }
    CHECK(suggested_rejected);
    CHECK(state->begins == 0);
    bool rejected = false;
    try { (void)service.requestSaveFile({"Save", {"*.txt"}}); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
    CHECK(state->begins == 0);
    const auto id = service.requestSaveFile({"Save", {"txt", "md", "cpp"}});
    CHECK(id == 1);
    CHECK(state->extensions == std::vector<std::string>{"txt", "md", "cpp"});
    state->completions[0](NSModalResponseOK, [NSURL fileURLWithPath:@"/tmp/report.bin"]);
    const auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<FileDialogResultEvent>(*event).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*event).path ==
          std::filesystem::path{"/tmp/report.bin"});
}

TEST_CASE("AppKit save file maps cancellation, failure and stale completion safely") {
    EventQueue events;
    auto state = std::make_shared<FakeSaveFileState>();
    detail::AppKitFileDialogService service{
        testOwner(), events, std::make_unique<FakeOpenFileInvoker>(std::make_shared<FakeOpenFileState>()),
        std::make_unique<FakeSaveFileInvoker>(state)};
    const auto cancelled_id = service.requestSaveFile({});
    state->completions[0](NSModalResponseCancel, nil);
    auto cancelled = events.tryPop();
    CHECK(cancelled.has_value());
    CHECK(std::get<FileDialogResultEvent>(*cancelled).request_id == cancelled_id);
    CHECK(std::get<FileDialogResultEvent>(*cancelled).status == FileDialogResultStatus::cancelled);

    const auto failed_id = service.requestSaveFile({});
    state->completions[1](static_cast<NSModalResponse>(-1), nil);
    auto failed = events.tryPop();
    CHECK(failed.has_value());
    CHECK(std::get<FileDialogResultEvent>(*failed).request_id == failed_id);
    CHECK(std::get<FileDialogResultEvent>(*failed).status == FileDialogResultStatus::failed);

    const auto stale = state->completions[1];
    const auto next_id = service.requestSaveFile({});
    stale(NSModalResponseOK, [NSURL fileURLWithPath:@"/tmp/stale.txt"]);
    CHECK(!events.tryPop().has_value());
    service.shutdown();
    state->completions[2](NSModalResponseOK, [NSURL fileURLWithPath:@"/tmp/late.txt"]);
    CHECK(!events.tryPop().has_value());
    CHECK(next_id != 0);
}

TEST_CASE("AppKit folder selection is directories-only, shared and shutdown-safe") {
    EventQueue events;
    auto open_state = std::make_shared<FakeOpenFileState>();
    auto save_state = std::make_shared<FakeSaveFileState>();
    auto folder_state = std::make_shared<FakeSelectFolderState>();
    detail::AppKitFileDialogService service{
        testOwner(), events,
        std::make_unique<FakeOpenFileInvoker>(open_state),
        std::make_unique<FakeSaveFileInvoker>(save_state),
        std::make_unique<FakeSelectFolderInvoker>(folder_state)};

    const auto folder_id = service.requestSelectFolder({"Select Folder 😀"});
    CHECK(folder_id == 1);
    CHECK(service.pending());
    CHECK(folder_state->begins == 1);
    bool open_rejected = false;
    bool save_rejected = false;
    bool folder_rejected = false;
    try { (void)service.requestOpenFile({}); } catch (const std::logic_error&) { open_rejected = true; }
    try { (void)service.requestSaveFile({}); } catch (const std::logic_error&) { save_rejected = true; }
    try { (void)service.requestSelectFolder({}); } catch (const std::logic_error&) { folder_rejected = true; }
    CHECK(open_rejected);
    CHECK(save_rejected);
    CHECK(folder_rejected);

    NSURL* selected = [NSURL fileURLWithPath:@"/tmp/Existing Folder/Grüße 📁"];
    folder_state->completions[0](NSModalResponseOK, selected);
    folder_state->completions[0](NSModalResponseOK, selected);
    CHECK(!service.pending());
    auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<FileDialogResultEvent>(*event).request_id == folder_id);
    CHECK(std::get<FileDialogResultEvent>(*event).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*event).path ==
          std::filesystem::path{selected.fileSystemRepresentation});
    CHECK(!events.tryPop().has_value());

    const auto open_id = service.requestOpenFile({});
    CHECK(open_id > folder_id);
    service.shutdown();
    open_state->completions[0](NSModalResponseOK, [NSURL fileURLWithPath:@"/tmp/late"]);
    CHECK(!events.tryPop().has_value());
}

TEST_CASE("AppKit file dialog ignores completion after shutdown and destruction") {
    EventQueue events;
    auto state = std::make_shared<FakeOpenFileState>();
    {
        detail::AppKitFileDialogService service{testOwner(), events,
            std::make_unique<FakeOpenFileInvoker>(state)};
        (void)service.requestOpenFile({});
        service.shutdown();
        CHECK(!service.pending());
    }
    state->completions[0](NSModalResponseOK, [NSURL fileURLWithPath:@"/tmp/late.txt"]);
    CHECK(!events.tryPop().has_value());
}

TEST_CASE("AppKit native menu projects the Core tree and queues activation") {
    NSMenu* saved_menu = [NSApp mainMenu];
    [NSApp setMainMenu:nil];

    AppKitWindowBackend backend{{"SASD AppKit menu test", {320, 200}, true, false}};
    Application application{backend};
    Command open{"Öffnen 😀"};
    bool executed = false;
    open.setOnExecuted([&] { executed = true; });
    Command disabled{"Disabled"};
    disabled.setEnabled(false);
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("Fíle 界");
    file.appendCommand(open, Shortcut{Key::o, KeyModifier::control});
    file.appendSeparator();
    MenuModel& nested = file.appendSubmenu("More & More");
    MenuModel& deeply_nested = nested.appendSubmenu("研究");
    deeply_nested.appendCommand(disabled);
    Command literal_ampersand{"Research & Development"};
    file.appendCommand(literal_ampersand);

    backend.syncMenuBar(bar);
    NSMenu* root = [NSApp mainMenu];
    CHECK(root != nil);
    CHECK(root.numberOfItems == 1);
    NSMenu* file_menu = [[root itemAtIndex:0] submenu];
    CHECK(file_menu != nil);
    CHECK(file_menu.numberOfItems == 4);
    CHECK([[file_menu itemAtIndex:0].title UTF8String] != nullptr);
    CHECK(std::string{[[file_menu itemAtIndex:0].title UTF8String]} == "Öffnen 😀");
    CHECK([file_menu itemAtIndex:0].keyEquivalent.length == 0);
    CHECK(std::string{[[file_menu itemAtIndex:2].title UTF8String]} == "More & More");
    CHECK(![[[file_menu itemAtIndex:2].submenu itemAtIndex:0].submenu itemAtIndex:0].isEnabled);
    CHECK(std::string{[[file_menu itemAtIndex:3].title UTF8String]} == "Research & Development");

    [NSApp sendAction:[file_menu itemAtIndex:0].action
                   to:[file_menu itemAtIndex:0].target
                 from:[file_menu itemAtIndex:0]];
    CHECK(!executed);
    bool saw_queued_activation = false;
    application.processEvents([&](const Event& event) {
        const auto& activation = std::get<CommandActivationEvent>(event);
        saw_queued_activation = true;
        CHECK(!executed);
        CHECK(activation.command.get() == &open);
        CHECK(activation.command.get()->execute());
    });
    CHECK(saw_queued_activation);
    CHECK(executed);

    backend.clearMenuBar();
    CHECK([NSApp mainMenu] == nil);
    [NSApp setMainMenu:saved_menu];
}

TEST_CASE("AppKit native menu rejects a second global owner and restores prior menu") {
    NSMenu* saved_menu = [NSApp mainMenu];
    NSMenu* existing = [[NSMenu alloc] initWithTitle:@"Existing"];
    [NSApp setMainMenu:existing];
    AppKitWindowBackend backend{{"SASD AppKit ownership test", {320, 200}, true, false}};
    backend.initialize();
    MenuBarModel bar;
    bar.appendMenu("File");
    backend.syncMenuBar(bar);
    CHECK([NSApp mainMenu] != existing);
    backend.clearMenuBar();
    CHECK([NSApp mainMenu] == existing);
    backend.shutdown();

    [NSApp setMainMenu:nil];
    AppKitWindowBackend first{{"SASD AppKit first owner", {320, 200}, true, false}};
    AppKitWindowBackend second{{"SASD AppKit second owner", {320, 200}, true, false}};
    first.initialize();
    second.initialize();
    first.syncMenuBar(bar);
    NSMenu* installed = [NSApp mainMenu];
    bool second_rejected = false;
    try {
        second.syncMenuBar(bar);
    } catch (const std::logic_error&) {
        second_rejected = true;
    }
    CHECK(second_rejected);
    CHECK([NSApp mainMenu] == installed);
    first.clearMenuBar();
    CHECK([NSApp mainMenu] == nil);
    second.shutdown();
    [NSApp setMainMenu:saved_menu];
}

TEST_CASE("AppKit expired command is disabled and cannot queue activation") {
    NSMenu* saved_menu = [NSApp mainMenu];
    [NSApp setMainMenu:nil];
    AppKitWindowBackend backend{{"SASD AppKit expired menu test", {320, 200}, true, false}};
    backend.initialize();
    MenuBarModel bar;
    MenuModel& menu = bar.appendMenu("Actions");
    auto command = std::make_unique<Command>("Temporary");
    menu.appendCommand(*command);
    backend.syncMenuBar(bar);
    NSMenuItem* item = [[[NSApp mainMenu] itemAtIndex:0].submenu itemAtIndex:0];
    command.reset();
    id<NSMenuDelegate> delegate = [[NSApp mainMenu] delegate];
    [delegate menuNeedsUpdate:[NSApp mainMenu]];
    [NSApp sendAction:item.action to:item.target from:item];
    CHECK(!item.isEnabled);
    CHECK(!backend.pollEvent().has_value());
    backend.shutdown();
    [NSApp setMainMenu:saved_menu];
}

TEST_CASE("AppKit native menu refreshes live command state without structural resync") {
    NSMenu* saved_menu = [NSApp mainMenu];
    [NSApp setMainMenu:nil];
    AppKitWindowBackend backend{{"SASD AppKit live menu test", {320, 200}, true, false}};
    backend.initialize();
    Command command{"Open"};
    MenuBarModel bar;
    bar.appendMenu("File").appendCommand(command);
    backend.syncMenuBar(bar);
    NSMenuItem* item = [[[NSApp mainMenu] itemAtIndex:0].submenu itemAtIndex:0];
    CHECK(std::string{item.title.UTF8String} == "Open");
    CHECK(item.isEnabled);

    command.setText("Open Recent");
    command.setEnabled(false);
    id<NSMenuDelegate> delegate = [[NSApp mainMenu] delegate];
    [delegate menuNeedsUpdate:[NSApp mainMenu]];
    CHECK(std::string{item.title.UTF8String} == "Open Recent");
    CHECK(!item.isEnabled);
    command.setEnabled(true);
    [delegate menuNeedsUpdate:[NSApp mainMenu]];
    CHECK(item.isEnabled);

    backend.shutdown();
    [NSApp setMainMenu:saved_menu];
}

TEST_CASE("AppKit native menu is an explicit structural snapshot and resync is transactional") {
    NSMenu* saved_menu = [NSApp mainMenu];
    [NSApp setMainMenu:nil];
    AppKitWindowBackend backend{{"SASD AppKit structural menu test", {320, 200}, true, false}};
    backend.initialize();
    Command open{"Open"};
    Command save{"Save"};
    Command exit{"Exit"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);
    backend.syncMenuBar(bar);
    NSMenu* old_root = [NSApp mainMenu];
    NSMenu* old_file = [old_root itemAtIndex:0].submenu;

    bar.clear();
    MenuModel& changed = bar.appendMenu("Changed");
    changed.appendCommand(save);
    changed.appendCommand(exit);
    CHECK([NSApp mainMenu] == old_root);
    CHECK(std::string{[[old_file itemAtIndex:0].title UTF8String]} == "Open");
    backend.syncMenuBar(bar);
    CHECK([NSApp mainMenu] != old_root);
    CHECK(std::string{[[[NSApp mainMenu] itemAtIndex:0].title UTF8String]} == "Changed");
    CHECK([[[NSApp mainMenu] itemAtIndex:0].submenu numberOfItems] == 2);

    std::string invalid{"bad\xFF", 4};
    changed.setTitle(invalid);
    NSMenu* stable_root = [NSApp mainMenu];
    bool rejected = false;
    try {
        backend.syncMenuBar(bar);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    CHECK(rejected);
    CHECK([NSApp mainMenu] == stable_root);
    CHECK(std::string{[[[NSApp mainMenu] itemAtIndex:0].title UTF8String]} == "Changed");

    backend.shutdown();
    [NSApp setMainMenu:saved_menu];
}

TEST_CASE("AppKit native menu restores prior menu but never clobbers external replacement") {
    NSMenu* saved_menu = [NSApp mainMenu];
    NSMenu* previous = [[NSMenu alloc] initWithTitle:@"Previous"];
    NSMenu* external = [[NSMenu alloc] initWithTitle:@"External"];
    [NSApp setMainMenu:previous];
    AppKitWindowBackend backend{{"SASD AppKit restoration test", {320, 200}, true, false}};
    backend.initialize();
    MenuBarModel bar;
    bar.appendMenu("File");
    backend.syncMenuBar(bar);
    CHECK([NSApp mainMenu] != previous);
    backend.shutdown();
    CHECK([NSApp mainMenu] == previous);

    backend.initialize();
    backend.syncMenuBar(bar);
    [NSApp setMainMenu:external];
    backend.clearMenuBar();
    CHECK([NSApp mainMenu] == external);
    backend.shutdown();
    CHECK([NSApp mainMenu] == external);
    [NSApp setMainMenu:saved_menu];
}
