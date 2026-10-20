#pragma once

#include <sasd/ui/events/event_queue.hpp>
#include <sasd/ui/file_dialog.hpp>

#import <AppKit/AppKit.h>

#include <functional>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui::native::appkit::detail {

using NativeOpenFileCompletion = std::function<void(NSModalResponse, NSURL*)>;
using NativeOpenFilesCompletion = std::function<void(NSModalResponse, NSArray<NSURL*>*)>;
using NativeSaveFileCompletion = NativeOpenFileCompletion;
using NativeSelectFolderCompletion = NativeOpenFileCompletion;

/** Private seam: production starts an NSOpenPanel sheet; tests retain this completion. */
class NativeOpenFileInvoker {
public:
    virtual ~NativeOpenFileInvoker() = default;
    virtual void begin(NSWindow* owner, NSString* title, std::vector<std::string> extensions,
                       const std::optional<std::filesystem::path>& initial_directory,
                       NativeOpenFileCompletion completion) = 0;
    virtual void cancel() noexcept = 0;
};

/** Private seam for multiple existing local files; the single-file seam remains unchanged. */
class NativeOpenFilesInvoker {
public:
    virtual ~NativeOpenFilesInvoker() = default;
    virtual void begin(NSWindow* owner, NSString* title, std::vector<std::string> extensions,
                       const std::optional<std::filesystem::path>& initial_directory,
                       NativeOpenFilesCompletion completion) = 0;
    virtual void cancel() noexcept = 0;
};

/** Private seam: production starts an NSSavePanel sheet; tests retain this completion. */
class NativeSaveFileInvoker {
public:
    virtual ~NativeSaveFileInvoker() = default;
    virtual void begin(NSWindow* owner, NSString* title, std::vector<std::string> extensions,
                       NSString* suggested_filename,
                       const std::optional<std::filesystem::path>& initial_directory,
                       NativeSaveFileCompletion completion) = 0;
    virtual void cancel() noexcept = 0;
};

/** Private seam: production starts an NSOpenPanel sheet configured for directories only. */
class NativeSelectFolderInvoker {
public:
    virtual ~NativeSelectFolderInvoker() = default;
    virtual void begin(NSWindow* owner, NSString* title,
                       const std::optional<std::filesystem::path>& initial_directory,
                       NativeSelectFolderCompletion completion) = 0;
    virtual void cancel() noexcept = 0;
};

/** Private AppKit consumer of the backend-neutral FileDialogService contract. */
class AppKitFileDialogService final : public FileDialogService {
public:
    AppKitFileDialogService(NSWindow* owner, EventQueue& events,
                            std::unique_ptr<NativeOpenFileInvoker> open_invoker = nullptr,
                            std::unique_ptr<NativeSaveFileInvoker> save_invoker = nullptr,
                            std::unique_ptr<NativeSelectFolderInvoker> folder_invoker = nullptr,
                            std::unique_ptr<NativeOpenFilesInvoker> open_files_invoker = nullptr);
    ~AppKitFileDialogService() override;

    AppKitFileDialogService(const AppKitFileDialogService&) = delete;
    AppKitFileDialogService& operator=(const AppKitFileDialogService&) = delete;

    [[nodiscard]] FileDialogRequestId requestOpenFile(OpenFileDialogRequest request) override;
    [[nodiscard]] FileDialogRequestId requestOpenFiles(OpenFilesDialogRequest request) override;
    [[nodiscard]] FileDialogRequestId requestSaveFile(SaveFileDialogRequest request) override;
    [[nodiscard]] FileDialogRequestId requestSelectFolder(
        SelectFolderDialogRequest request) override;
    [[nodiscard]] bool pending() const noexcept;
    void shutdown() noexcept;

private:
    struct CompletionState;
    NSWindow* owner_{}; // non-owning: the AppKitWindowBackend owns the window
    std::unique_ptr<NativeOpenFileInvoker> open_invoker_;
    std::unique_ptr<NativeOpenFilesInvoker> open_files_invoker_;
    std::unique_ptr<NativeSaveFileInvoker> save_invoker_;
    std::unique_ptr<NativeSelectFolderInvoker> folder_invoker_;
    std::shared_ptr<CompletionState> state_;
    FileDialogRequestId next_request_id_{1};
};

} // namespace sasd::ui::native::appkit::detail
