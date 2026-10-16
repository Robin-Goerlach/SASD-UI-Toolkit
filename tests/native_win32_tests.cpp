#include "test_framework.hpp"

#include "win32_input_translation.hpp"
#include "win32_file_dialog_service.hpp"
#include "win32_menu_binding.hpp"

#include <sasd/ui/native/win32_backend.hpp>
#include <sasd/ui/rendered/display_list.hpp>

#include <memory>
#include <stdexcept>
#include <variant>
#include <utility>

using namespace sasd::ui;
using namespace sasd::ui::native::win32;
using namespace sasd::ui::native::win32::detail;

namespace {

class FakeOpenFileInvoker final : public NativeOpenFileInvoker {
public:
    explicit FakeOpenFileInvoker(NativeOpenFileOutcome outcome) : outcome_{std::move(outcome)} {}

    NativeOpenFileOutcome show(HWND, std::wstring_view title) override {
        title_ = title;
        ++invocations;
        if (service != nullptr) {
            try {
                (void)service->requestOpenFile(OpenFileDialogRequest{"nested"});
            } catch (const std::logic_error&) {
                nested_request_rejected = true;
            }
        }
        return outcome_;
    }

    NativeOpenFileOutcome outcome_;
    std::wstring title_;
    Win32FileDialogService* service{nullptr};
    int invocations{0};
    bool nested_request_rejected{false};
};

class FakeSaveFileInvoker final : public NativeSaveFileInvoker {
public:
    explicit FakeSaveFileInvoker(NativeSaveFileOutcome outcome) : outcome_{std::move(outcome)} {}

    NativeSaveFileOutcome show(HWND, std::wstring_view title) override {
        title_ = title;
        ++invocations;
        if (service != nullptr) {
            try {
                (void)service->requestOpenFile(OpenFileDialogRequest{"nested"});
            } catch (const std::logic_error&) {
                nested_request_rejected = true;
            }
        }
        return outcome_;
    }

    NativeSaveFileOutcome outcome_;
    std::wstring title_;
    Win32FileDialogService* service{nullptr};
    int invocations{0};
    bool nested_request_rejected{false};
};

class FakeSelectFolderInvoker final : public NativeSelectFolderInvoker {
public:
    explicit FakeSelectFolderInvoker(NativeSelectFolderOutcome outcome) : outcome_{std::move(outcome)} {}

    NativeSelectFolderOutcome show(HWND, std::wstring_view title) override {
        title_ = title;
        ++invocations;
        if (service != nullptr) {
            try { (void)service->requestOpenFile(OpenFileDialogRequest{"nested open"}); }
            catch (const std::logic_error&) { open_rejected = true; }
            try { (void)service->requestSaveFile(SaveFileDialogRequest{"nested save"}); }
            catch (const std::logic_error&) { save_rejected = true; }
            try { (void)service->requestSelectFolder(SelectFolderDialogRequest{"nested folder"}); }
            catch (const std::logic_error&) { folder_rejected = true; }
        }
        return outcome_;
    }

    NativeSelectFolderOutcome outcome_;
    std::wstring title_;
    Win32FileDialogService* service{nullptr};
    int invocations{0};
    bool open_rejected{false};
    bool save_rejected{false};
    bool folder_rejected{false};
};

std::wstring menuText(HMENU menu, UINT item, UINT flags) {
    wchar_t buffer[256]{};
    const int length = GetMenuStringW(menu, item, buffer, static_cast<int>(std::size(buffer)), flags);
    return length <= 0 ? std::wstring{} : std::wstring{buffer, static_cast<std::size_t>(length)};
}

} // namespace

TEST_CASE("Win32 key translation preserves semantic identity and modifiers") {
    const auto event = translateKeyMessage(WM_KEYDOWN, 'C', KeyModifier::control);
    CHECK(event.has_value());
    CHECK(event->key == Key::c);
    CHECK(event->pressed);
    CHECK(event->modifiers == KeyModifier::control);

    const auto release = translateKeyMessage(WM_KEYUP, VK_LEFT, KeyModifier::shift);
    CHECK(release.has_value());
    CHECK(release->key == Key::left);
    CHECK(!release->pressed);
    CHECK(release->modifiers == KeyModifier::shift);

    CHECK(!translateKeyMessage(WM_KEYDOWN, VK_F24, KeyModifier::none).has_value());
}

TEST_CASE("Win32 modifier and coordinate translation is backend-neutral") {
    const auto modifiers = translateModifiers(MK_SHIFT | MK_CONTROL, KeyModifier::alt);
    CHECK(hasModifier(modifiers, KeyModifier::shift));
    CHECK(hasModifier(modifiers, KeyModifier::control));
    CHECK(hasModifier(modifiers, KeyModifier::alt));

    const LPARAM native_point = static_cast<LPARAM>(MAKELONG(static_cast<WORD>(-7),
                                                             static_cast<WORD>(23)));
    const auto point = translateClientPoint(native_point);
    CHECK(point.has_value());
    CHECK(point->x == -7);
    CHECK(point->y == 23);
}

TEST_CASE("Win32 pointer translation maps buttons, action, coordinates and modifiers") {
    const LPARAM native_point = static_cast<LPARAM>(MAKELONG(31, 17));
    const auto press = translatePointerMessage(WM_LBUTTONDOWN, MK_SHIFT, native_point);
    CHECK(press.has_value());
    CHECK(press->action == PointerAction::press);
    CHECK(press->button == PointerButton::primary);
    CHECK(press->position == Point{31, 17});
    CHECK(hasModifier(press->modifiers, KeyModifier::shift));

    const auto motion = translatePointerMessage(WM_MOUSEMOVE, MK_CONTROL, native_point,
                                                 KeyModifier::alt);
    CHECK(motion.has_value());
    CHECK(motion->action == PointerAction::move);
    CHECK(motion->button == PointerButton::none);
    CHECK(hasModifier(motion->modifiers, KeyModifier::control));
    CHECK(hasModifier(motion->modifiers, KeyModifier::alt));

    CHECK(!translatePointerMessage(WM_MOUSEWHEEL, 0, native_point).has_value());

    const auto leave = translatePointerSurfaceMessage(WM_MOUSELEAVE);
    CHECK(leave.has_value());
    CHECK(leave->action == PointerSurfaceAction::left);
}

TEST_CASE("Win32 text and window lifecycle translation fails closed") {
    // Keep the source literal ASCII-only so this test is independent of the MSVC source code page.
    const auto text = translateTextInput(L"Gr\u00FC\u00DFe");
    CHECK(text.has_value());
    CHECK(text->text == std::string{"Gr\xC3\xBC\xC3\x9F" "e"});
    CHECK(!translateTextInput(std::wstring{static_cast<wchar_t>(0xD800)}).has_value());

    const auto close = translateWindowMessage(WM_CLOSE, 0, 0);
    CHECK(close.has_value());
    CHECK(std::holds_alternative<QuitEvent>(*close));

    const auto resize = translateWindowMessage(WM_SIZE, SIZE_RESTORED,
                                                static_cast<LPARAM>(MAKELONG(640, 480)));
    CHECK(resize.has_value());
    CHECK(std::get<ResizeEvent>(*resize).size == Size{640, 480});
    CHECK(!translateWindowMessage(WM_SIZE, SIZE_MINIMIZED, 0).has_value());
}

TEST_CASE("Win32 host owns a hidden top-level window through deterministic lifecycle") {
    Win32WindowBackend backend{{"SASD Win32 test", {320, 200}, true, false}};
    CHECK(!backend.isInitialized());
    CHECK(backend.capabilities().pointer_input);
    backend.initialize();
    CHECK(backend.isInitialized());
    CHECK(backend.windowSize() == Size{320, 200});
    CHECK(backend.capabilities().native_menus);
    CHECK(backend.capabilities().file_dialogs);
    CHECK(backend.fileDialogs() != nullptr);
    CHECK(!backend.pollEvent().has_value());
    backend.shutdown();
    CHECK(!backend.isInitialized());
    CHECK(backend.fileDialogs() == nullptr);
}

TEST_CASE("Win32 file dialog service queues accepted result with exact UTF-16 path") {
    EventQueue events;
    auto fake = std::make_unique<FakeOpenFileInvoker>(NativeOpenFileOutcome{
        FileDialogResultStatus::accepted, L"C:\\Unicode\\Gr\u00FC\u00DFe.txt"});
    FakeOpenFileInvoker* fake_ptr = fake.get();
    Win32FileDialogService service{nullptr, events, std::move(fake)};
    const auto id = service.requestOpenFile(OpenFileDialogRequest{"\xF0\x9F\x93\x82 Open"});
    CHECK(id != 0);
    CHECK(fake_ptr->title_ == L"\U0001F4C2 Open");
    CHECK(fake_ptr->invocations == 1);
    const auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<FileDialogResultEvent>(*event).request_id == id);
    CHECK(std::get<FileDialogResultEvent>(*event).status == FileDialogResultStatus::accepted);
    CHECK(std::get<FileDialogResultEvent>(*event).path == std::filesystem::path{L"C:\\Unicode\\Gr\u00FC\u00DFe.txt"});
    CHECK(!events.tryPop().has_value());
}

TEST_CASE("Win32 file dialog service maps cancel and failure to one pathless completion") {
    for (const auto status : {FileDialogResultStatus::cancelled, FileDialogResultStatus::failed}) {
        EventQueue events;
        auto fake = std::make_unique<FakeOpenFileInvoker>(NativeOpenFileOutcome{status, L""});
        Win32FileDialogService service{nullptr, events, std::move(fake)};
        const auto id = service.requestOpenFile({});
        const auto event = events.tryPop();
        CHECK(event.has_value());
        const auto& result = std::get<FileDialogResultEvent>(*event);
        CHECK(result.request_id == id);
        CHECK(result.status == status);
        CHECK(!result.path.has_value());
        CHECK(!events.tryPop().has_value());
    }
}

TEST_CASE("Win32 file dialog service allocates distinct sequential request IDs") {
    EventQueue events;
    auto fake = std::make_unique<FakeOpenFileInvoker>(NativeOpenFileOutcome{
        FileDialogResultStatus::cancelled, L""});
    Win32FileDialogService service{nullptr, events, std::move(fake)};
    const auto first = service.requestOpenFile({});
    const auto second = service.requestOpenFile({});
    CHECK(first != 0);
    CHECK(second != 0);
    CHECK(first != second);
    CHECK(events.tryPop().has_value());
    CHECK(events.tryPop().has_value());
}

TEST_CASE("Win32 file dialog service rejects a nested pending request and preserves long paths") {
    EventQueue events;
    std::wstring long_path = L"C:\\";
    long_path.append(4096, L'x');
    long_path += L".txt";
    auto fake = std::make_unique<FakeOpenFileInvoker>(NativeOpenFileOutcome{
        FileDialogResultStatus::accepted, long_path});
    FakeOpenFileInvoker* fake_ptr = fake.get();
    Win32FileDialogService service{nullptr, events, std::move(fake)};
    fake_ptr->service = &service;
    const auto first = service.requestOpenFile({});
    CHECK(fake_ptr->nested_request_rejected);
    bool malformed_title_rejected = false;
    try {
        (void)service.requestOpenFile({"\xFF"});
    } catch (const std::invalid_argument&) {
        malformed_title_rejected = true;
    }
    CHECK(malformed_title_rejected);
    const auto event = events.tryPop();
    CHECK(event.has_value());
    CHECK(std::get<FileDialogResultEvent>(*event).request_id == first);
    CHECK(std::get<FileDialogResultEvent>(*event).path == std::filesystem::path{long_path});
}

TEST_CASE("Win32 save file uses the shared contract without touching the filesystem") {
    EventQueue events;
    auto open_fake = std::make_unique<FakeOpenFileInvoker>(NativeOpenFileOutcome{
        FileDialogResultStatus::cancelled, L""});
    auto save_fake = std::make_unique<FakeSaveFileInvoker>(NativeSaveFileOutcome{
        FileDialogResultStatus::accepted, L"C:\\New Folder\\Gr\u00FC\u00DFe \U0001F4C4.txt"});
    FakeSaveFileInvoker* save_ptr = save_fake.get();
    Win32FileDialogService service{nullptr, events, std::move(open_fake), std::move(save_fake)};

    const auto id = service.requestSaveFile(SaveFileDialogRequest{"\xF0\x9F\x92\xBE Save"});
    CHECK(id != 0);
    CHECK(save_ptr->title_ == L"\U0001F4BE Save");
    const auto event = events.tryPop();
    CHECK(event.has_value());
    const auto& result = std::get<FileDialogResultEvent>(*event);
    CHECK(result.request_id == id);
    CHECK(result.status == FileDialogResultStatus::accepted);
    CHECK(result.path == std::filesystem::path{L"C:\\New Folder\\Gr\u00FC\u00DFe \U0001F4C4.txt"});
    CHECK(!events.tryPop().has_value());
}

TEST_CASE("Win32 open and save share IDs and the pending boundary") {
    EventQueue events;
    auto open_fake = std::make_unique<FakeOpenFileInvoker>(NativeOpenFileOutcome{
        FileDialogResultStatus::cancelled, L""});
    auto save_fake = std::make_unique<FakeSaveFileInvoker>(NativeSaveFileOutcome{
        FileDialogResultStatus::cancelled, L""});
    FakeOpenFileInvoker* open_ptr = open_fake.get();
    FakeSaveFileInvoker* save_ptr = save_fake.get();
    Win32FileDialogService service{nullptr, events, std::move(open_fake), std::move(save_fake)};
    open_ptr->service = &service;
    save_ptr->service = &service;

    const auto open_id = service.requestOpenFile({});
    CHECK(open_ptr->nested_request_rejected);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == open_id);
    const auto save_id = service.requestSaveFile({});
    CHECK(save_ptr->nested_request_rejected);
    CHECK(save_id > open_id);
    CHECK(std::get<FileDialogResultEvent>(*events.tryPop()).request_id == save_id);
    CHECK(!events.tryPop().has_value());
}

TEST_CASE("Win32 folder selection uses one local path and the shared operation contract") {
    EventQueue events;
    auto folder_fake = std::make_unique<FakeSelectFolderInvoker>(NativeSelectFolderOutcome{
        FileDialogResultStatus::accepted, L"C:\\Existing Folder\\Gr\u00FC\u00DFe \U0001F4C1"});
    FakeSelectFolderInvoker* folder_ptr = folder_fake.get();
    Win32FileDialogService service{nullptr, events, nullptr, nullptr, std::move(folder_fake)};
    folder_ptr->service = &service;

    const auto id = service.requestSelectFolder(SelectFolderDialogRequest{"Select \xF0\x9F\x93\x81"});
    CHECK(id == 1);
    CHECK(folder_ptr->open_rejected);
    CHECK(folder_ptr->save_rejected);
    CHECK(folder_ptr->folder_rejected);
    const auto event = events.tryPop();
    CHECK(event.has_value());
    const auto& result = std::get<FileDialogResultEvent>(*event);
    CHECK(result.request_id == id);
    CHECK(result.status == FileDialogResultStatus::accepted);
    CHECK(result.path == std::filesystem::path{L"C:\\Existing Folder\\Gr\u00FC\u00DFe \U0001F4C1"});
    CHECK(!service.pending());
}

TEST_CASE("Win32 native menu binding projects structure and resolves a live Core Command") {
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    auto command = std::make_unique<Command>("Open");
    bool executed = false;
    command->setOnExecuted([&] { executed = true; });
    file.appendCommand(*command, Shortcut{Key::o, KeyModifier::control});
    file.appendSeparator();
    MenuModel& recent = file.appendSubmenu("Recent");
    Command close{"Close"};
    recent.appendCommand(close);

    detail::Win32MenuBinding binding;
    binding.rebuild(bar);
    CHECK(binding.root() != nullptr);

    HMENU file_popup = GetSubMenu(binding.root(), 0);
    CHECK(file_popup != nullptr);
    CHECK(GetMenuItemCount(file_popup) == 3);
    CHECK((GetMenuState(file_popup, 1, MF_BYPOSITION) & MF_SEPARATOR) != 0U);
    const UINT open_id = GetMenuItemID(file_popup, 0);
    CHECK(open_id != static_cast<UINT>(-1));
    const Command::Reference resolved = binding.resolveCommand(open_id);
    CHECK(resolved.get() == command.get());
    CHECK(!executed);
    CHECK(!binding.resolveCommand(open_id + 1000));
}

TEST_CASE("Win32 native menu resolution remains safe when later execution destroys a Command") {
    MenuBarModel bar;
    MenuModel& menu = bar.appendMenu("Actions");
    auto command = std::make_unique<Command>("Destroy");
    command->setOnExecuted([&] { command.reset(); });
    menu.appendCommand(*command);

    detail::Win32MenuBinding binding;
    binding.rebuild(bar);
    const UINT command_id = GetMenuItemID(GetSubMenu(binding.root(), 0), 0);
    const Command::Reference resolved = binding.resolveCommand(command_id);
    CHECK(resolved.get() == command.get());
    CHECK(resolved.get()->execute());
    CHECK(command == nullptr);
    CHECK(!binding.resolveCommand(command_id));
}

TEST_CASE("Win32 WM_COMMAND queues activation and never executes inside the native callback") {
    Win32WindowBackend backend{{"SASD Win32 event test", {320, 200}, true, false}};
    backend.initialize();

    MenuBarModel bar;
    MenuModel& menu = bar.appendMenu("Actions");
    auto command = std::make_unique<Command>("Destroy");
    bool executed = false;
    command->setOnExecuted([&] {
        executed = true;
        command.reset();
    });
    menu.appendCommand(*command);
    backend.syncMenuBar(bar);

    HWND window = FindWindowW(nullptr, L"SASD Win32 event test");
    CHECK(window != nullptr);
    const HMENU root = GetMenu(window);
    const UINT command_id = GetMenuItemID(GetSubMenu(root, 0), 0);
    CHECK(command_id != static_cast<UINT>(-1));

    // Creation can leave unrelated native messages in the thread queue. Drain those first so the
    // assertion below identifies the event produced by this WM_COMMAND specifically.
    while (backend.pollEvent().has_value()) {
    }

    // SendMessage returns only after WndProc has returned. The callback must still be untouched.
    (void)SendMessageW(window, WM_COMMAND, MAKEWPARAM(command_id, 0), 0);
    CHECK(!executed);
    CHECK(command != nullptr);

    Command::Reference reference;
    bool saw_activation = false;
    for (int attempt = 0; attempt < 8 && !saw_activation; ++attempt) {
        const auto event = backend.pollEvent();
        if (!event.has_value()) break;
        if (std::holds_alternative<CommandActivationEvent>(*event)) {
            reference = std::get<CommandActivationEvent>(*event).command;
            saw_activation = true;
        }
    }
    CHECK(saw_activation);
    CHECK(reference.get() == command.get());
    CHECK(reference.get()->execute());
    CHECK(executed);
    CHECK(command == nullptr);
    CHECK(!backend.pollEvent().has_value());
}

TEST_CASE("Win32 native menu labels escape literal ampersands and refresh them safely") {
    Win32WindowBackend backend{{"SASD Win32 ampersand test", {320, 200}, true, false}};
    backend.initialize();

    MenuBarModel bar;
    MenuModel& top = bar.appendMenu("Tools & More");
    MenuModel& submenu = top.appendSubmenu("Research & Development");
    Command save{"Save & Close"};
    submenu.appendCommand(save);
    Command dynamic{"A & B"};
    top.appendCommand(dynamic);
    backend.syncMenuBar(bar);

    HWND window = FindWindowW(nullptr, L"SASD Win32 ampersand test");
    CHECK(window != nullptr);
    const HMENU root = GetMenu(window);
    const HMENU top_menu = GetSubMenu(root, 0);
    CHECK(menuText(root, 0, MF_BYPOSITION) == L"Tools && More");
    CHECK(menuText(top_menu, 0, MF_BYPOSITION) == L"Research && Development");
    CHECK(menuText(GetSubMenu(top_menu, 0), 0, MF_BYPOSITION) == L"Save && Close");

    const UINT dynamic_id = GetMenuItemID(top_menu, 1);
    CHECK(menuText(top_menu, dynamic_id, MF_BYCOMMAND) == L"A && B");
    dynamic.setText("C & D");
    (void)SendMessageW(window, WM_INITMENU, reinterpret_cast<WPARAM>(root), 0);
    CHECK(menuText(top_menu, dynamic_id, MF_BYCOMMAND) == L"C && D");
}

TEST_CASE("Win32 host consumes the backend-neutral DisplayList and rendered metrics") {
    Win32WindowBackend backend{{"SASD Win32 rendered test", {320, 200}, true, false}};
    backend.initialize();

    rendered::DisplayList frame;
    frame.fillRect({0, 0, 320, 200}, Color::black);
    frame.strokeRect({8, 8, 180, 36}, Color::bright_white);
    frame.drawText({16, 16}, "Win32 Rendered", TextStyle{Color::bright_white});

    CHECK(backend.presentFrame(frame) == frame.size());
    CHECK(!backend.presentationRequested());
    CHECK(backend.measureText("SASD").width > 0);
    CHECK(backend.lineHeight() > 0);
    CHECK(backend.textAdvanceToScalar("SASD", 2).has_value());

    backend.shutdown();
}
