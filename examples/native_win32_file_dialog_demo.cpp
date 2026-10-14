#include <sasd/ui/application.hpp>
#include <sasd/ui/native/win32_backend.hpp>

#include <iostream>
#include <chrono>
#include <thread>

using namespace sasd::ui;
using sasd::ui::native::win32::Win32WindowBackend;

int main() {
    Win32WindowBackend backend{{"SASD UI Toolkit – M7 Open File", {720, 420}, false, true}};
    Application application{backend};

    FileDialogService* dialogs = backend.fileDialogs();
    if (dialogs == nullptr) {
        std::cerr << "File dialogs are unavailable\n";
        return 1;
    }
    (void)dialogs->requestOpenFile(OpenFileDialogRequest{"Open an existing file"});

    bool finished = false;
    while (!finished && !application.exitRequested()) {
        (void)application.processRoutedEvents(
            {},
            [&](const Event& event) {
                if (!std::holds_alternative<FileDialogResultEvent>(event)) return;
                const auto& result = std::get<FileDialogResultEvent>(event);
                if (result.status == FileDialogResultStatus::accepted) {
                    std::cout << "Selected: " << result.path->wstring() << '\n';
                } else if (result.status == FileDialogResultStatus::cancelled) {
                    std::cout << "Open cancelled\n";
                } else {
                    std::cout << "Open failed\n";
                }
                finished = true;
            });
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    return 0;
}
