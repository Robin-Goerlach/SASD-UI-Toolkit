#pragma once

#include <sasd/ui/events/event_queue.hpp>
#include <sasd/ui/file_dialog.hpp>

#include <windows.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sasd::ui::native::win32::detail {

struct NativeOpenFileOutcome {
    FileDialogResultStatus status{FileDialogResultStatus::failed};
    std::wstring path{};
};

struct NativeOpenFilesOutcome {
    FileDialogResultStatus status{FileDialogResultStatus::failed};
    std::vector<std::wstring> paths{};
};

using NativeSaveFileOutcome = NativeOpenFileOutcome;
using NativeSelectFolderOutcome = NativeOpenFileOutcome;

/** Private invocation seam: production uses IFileOpenDialog; tests use deterministic completion. */
class NativeOpenFileInvoker {
public:
    virtual ~NativeOpenFileInvoker() = default;
    [[nodiscard]] virtual NativeOpenFileOutcome show(
        HWND owner, std::wstring_view title, const std::vector<std::wstring>& extensions,
        const std::optional<std::filesystem::path>& initial_directory) = 0;
};

/** Private invocation seam for the distinct multiple-file operation. */
class NativeOpenFilesInvoker {
public:
    virtual ~NativeOpenFilesInvoker() = default;
    [[nodiscard]] virtual NativeOpenFilesOutcome show(
        HWND owner, std::wstring_view title, const std::vector<std::wstring>& extensions,
        const std::optional<std::filesystem::path>& initial_directory) = 0;
};

[[nodiscard]] inline std::wstring win32FilterPattern(
    const std::vector<std::wstring>& extensions) {
    std::wstring pattern;
    for (std::size_t index = 0; index < extensions.size(); ++index) {
        if (index != 0) pattern += L';';
        pattern += L"*.";
        pattern += extensions[index];
    }
    return pattern;
}

/** Private invocation seam: production uses IFileSaveDialog; tests use deterministic completion. */
class NativeSaveFileInvoker {
public:
    virtual ~NativeSaveFileInvoker() = default;
    [[nodiscard]] virtual NativeSaveFileOutcome show(
        HWND owner, std::wstring_view title, const std::vector<std::wstring>& extensions,
        std::wstring_view suggested_filename,
        const std::optional<std::filesystem::path>& initial_directory) = 0;
};

/** Private invocation seam: production uses IFileOpenDialog with FOS_PICKFOLDERS. */
class NativeSelectFolderInvoker {
public:
    virtual ~NativeSelectFolderInvoker() = default;
    [[nodiscard]] virtual NativeSelectFolderOutcome show(
        HWND owner, std::wstring_view title,
        const std::optional<std::filesystem::path>& initial_directory) = 0;
};

/** Win32-only FileDialogService implementation; this header is never installed. */
class Win32FileDialogService final : public FileDialogService {
public:
    Win32FileDialogService(HWND owner,
                           EventQueue& events,
                           std::unique_ptr<NativeOpenFileInvoker> open_invoker = nullptr,
                           std::unique_ptr<NativeSaveFileInvoker> save_invoker = nullptr,
                           std::unique_ptr<NativeSelectFolderInvoker> folder_invoker = nullptr,
                           std::unique_ptr<NativeOpenFilesInvoker> open_files_invoker = nullptr);

    [[nodiscard]] FileDialogRequestId requestOpenFile(OpenFileDialogRequest request) override;
    [[nodiscard]] FileDialogRequestId requestOpenFiles(OpenFilesDialogRequest request) override;
    [[nodiscard]] FileDialogRequestId requestSaveFile(SaveFileDialogRequest request) override;
    [[nodiscard]] FileDialogRequestId requestSelectFolder(
        SelectFolderDialogRequest request) override;

    [[nodiscard]] bool pending() const noexcept { return pending_; }

private:
    HWND owner_;
    EventQueue& events_;
    std::unique_ptr<NativeOpenFileInvoker> open_invoker_;
    std::unique_ptr<NativeOpenFilesInvoker> open_files_invoker_;
    std::unique_ptr<NativeSaveFileInvoker> save_invoker_;
    std::unique_ptr<NativeSelectFolderInvoker> folder_invoker_;
    FileDialogRequestId next_request_id_{1};
    bool pending_{false};
};

} // namespace sasd::ui::native::win32::detail
