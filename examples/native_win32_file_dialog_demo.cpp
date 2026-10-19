#include <sasd/ui/application.hpp>
#include <sasd/ui/native/win32_backend.hpp>

#include <iostream>
#include <chrono>
#include <filesystem>
#include <optional>
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
    std::error_code directory_error;
    const std::filesystem::path demo_directory = std::filesystem::current_path(directory_error);
    const std::optional<std::filesystem::path> initial_directory =
        directory_error ? std::nullopt : std::optional<std::filesystem::path>{demo_directory};
    const auto open_id = dialogs->requestOpenFile(
        OpenFileDialogRequest{"Open a text or source file", {"txt", "md", "cpp", "hpp"},
                              initial_directory});

    FileDialogRequestId save_id = 0;
    FileDialogRequestId folder_id = 0;
    bool finished = false;
    while (!finished && !application.exitRequested()) {
        (void)application.processRoutedEvents(
            {},
            [&](const Event& event) {
                if (!std::holds_alternative<FileDialogResultEvent>(event)) return;
                const auto& result = std::get<FileDialogResultEvent>(event);
                const bool is_open = result.request_id == open_id;
                const bool is_save = result.request_id == save_id;
                const bool is_folder = result.request_id == folder_id;
                if (!is_open && !is_save && !is_folder) return;
                const char* operation = is_folder ? "Folder" : (is_save ? "Save" : "Open");
                if (result.status == FileDialogResultStatus::accepted) {
                    std::cout << operation << " target: " << result.path->string() << '\n';
                } else if (result.status == FileDialogResultStatus::cancelled) {
                    std::cout << operation << " cancelled\n";
                } else {
                    std::cout << operation << " failed\n";
                }
                if (is_open) {
                    // The demo displays the target only; it never creates or writes a file.
                    save_id = dialogs->requestSaveFile(
                        SaveFileDialogRequest{"Save Text or Source File", {"txt", "md"},
                                              "untitled.txt", initial_directory});
                } else if (is_save) {
                    // Folder selection only displays the selected existing directory.
                    folder_id = dialogs->requestSelectFolder(
                        SelectFolderDialogRequest{"Select a folder", initial_directory});
                } else {
                    finished = true;
                }
            });
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    return 0;
}
