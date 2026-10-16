#include "test_framework.hpp"

#include "appkit_input_translation.hpp"
#include "appkit_file_dialog_service.hpp"
#include "appkit_render_context.hpp"

#include <sasd/ui/native/appkit_backend.hpp>
#include <sasd/ui/application.hpp>
#include <sasd/ui/menu_model.hpp>

#import <AppKit/AppKit.h>

#include <memory>
#include <string>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::native::appkit;

namespace {
struct FakeOpenFileState {
    std::vector<detail::NativeOpenFileCompletion> completions;
    int begins{0};
};

class FakeOpenFileInvoker final : public detail::NativeOpenFileInvoker {
public:
    explicit FakeOpenFileInvoker(std::shared_ptr<FakeOpenFileState> state) : state_{std::move(state)} {}
    void begin(NSWindow*, NSString* title, detail::NativeOpenFileCompletion completion) override {
        title_ = title == nil ? std::string{} : std::string{title.UTF8String};
        ++state_->begins;
        state_->completions.push_back(std::move(completion));
    }
    void cancel() noexcept override { ++cancels; }
    std::shared_ptr<FakeOpenFileState> state_;
    std::string title_;
    int cancels{0};
};

struct FakeSaveFileState {
    std::vector<detail::NativeSaveFileCompletion> completions;
    int begins{0};
};

class FakeSaveFileInvoker final : public detail::NativeSaveFileInvoker {
public:
    explicit FakeSaveFileInvoker(std::shared_ptr<FakeSaveFileState> state) : state_{std::move(state)} {}
    void begin(NSWindow*, NSString* title, detail::NativeSaveFileCompletion completion) override {
        title_ = title == nil ? std::string{} : std::string{title.UTF8String};
        ++state_->begins;
        state_->completions.push_back(std::move(completion));
    }
    void cancel() noexcept override { ++cancels; }
    std::shared_ptr<FakeSaveFileState> state_;
    std::string title_;
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

    const auto save_id = service.requestSaveFile({"Save 😀"});
    CHECK(save_id != 0);
    CHECK(service.pending());
    CHECK(save_state->begins == 1);
    CHECK(save_state->completions.size() == 1);
    CHECK(save_state->completions[0]);
    CHECK(save_state->completions[0] != nullptr);
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
