#include <sasd/ui/application.hpp>
#include <sasd/ui/native/win32_backend.hpp>

#include <iostream>
#include <chrono>
#include <thread>

using namespace sasd::ui;
using sasd::ui::native::win32::Win32WindowBackend;

int main() {
    Win32WindowBackend backend{{"SASD UI Toolkit – M7 File Dialogs", {720, 420}, false, true}};
    Application application{backend};

    FileDialogService* dialogs = backend.fileDialogs();
    if (dialogs == nullptr) {
        std::cerr << "File dialogs are unavailable\n";
        return 1;
    }
    const auto open_id = dialogs->requestOpenFile(OpenFileDialogRequest{"Open an existing file"});

    FileDialogRequestId save_id = 0;
    bool finished = false;
    while (!finished && !application.exitRequested()) {
        (void)application.processRoutedEvents(
            {},
            [&](const Event& event) {
                if (!std::holds_alternative<FileDialogResultEvent>(event)) return;
                const auto& result = std::get<FileDialogResultEvent>(event);
                const bool is_open = result.request_id == open_id;
                const bool is_save = result.request_id == save_id;
                if (!is_open && !is_save) return;
                const char* operation = is_save ? "Save" : "Open";
                if (result.status == FileDialogResultStatus::accepted) {
                    std::cout << operation << " target: " << result.path->string() << '\n';
                } else if (result.status == FileDialogResultStatus::cancelled) {
                    std::cout << operation << " cancelled\n";
                } else {
                    std::cout << operation << " failed\n";
                }
                if (is_open) {
                    // The demo displays the target only; it never creates or writes a file.
                    save_id = dialogs->requestSaveFile(SaveFileDialogRequest{"Save to a target path"});
                } else {
                    finished = true;
                }
            });
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    return 0;
}
