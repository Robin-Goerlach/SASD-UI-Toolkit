#include "appkit_file_dialog_service.hpp"

#include "../file_dialog_extension_filter.hpp"
#include "../file_dialog_initial_directory.hpp"
#include "../file_dialog_suggested_filename.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <limits>
#include <stdexcept>
#include <utility>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace sasd::ui::native::appkit::detail {
namespace {

struct PanelState {
    NSSavePanel* __strong panel{nil};
};

[[nodiscard]] NSURL* nativeDirectoryURL(const std::filesystem::path& directory);

class ProductionOpenFileInvoker final : public NativeOpenFileInvoker {
public:
    void begin(NSWindow* owner, NSString* title, std::vector<std::string> extensions,
               const std::optional<std::filesystem::path>& initial_directory,
               NativeOpenFileCompletion completion) override {
        @autoreleasepool {
            @try {
                auto panel_state = std::make_shared<PanelState>();
                NSOpenPanel* panel = [NSOpenPanel openPanel];
                panel_state->panel = panel;
                panel.canChooseFiles = YES;
                panel.canChooseDirectories = NO;
                panel.allowsMultipleSelection = NO;
                if (title != nil) panel.title = title;
                if (initial_directory.has_value()) {
                    panel.directoryURL = nativeDirectoryURL(*initial_directory);
                    if (panel.directoryURL == nil) {
                        throw std::runtime_error{"AppKit initial directory cannot be represented"};
                    }
                }
                if (!extensions.empty()) {
                    if (@available(macOS 11.0, *)) {
                        NSMutableArray* content_types =
                            [NSMutableArray arrayWithCapacity:extensions.size()];
                        for (const std::string& extension : extensions) {
                            NSString* value = [[NSString alloc] initWithBytes:extension.data()
                                                                           length:extension.size()
                                                                         encoding:NSUTF8StringEncoding];
                            if (value == nil) throw std::invalid_argument{
                                "AppKit file-dialog extension must be valid UTF-8"};
                            UTType* type = [UTType typeWithFilenameExtension:value];
                            if (type == nil) throw std::invalid_argument{
                                "AppKit file-dialog extension could not become a UTType"};
                            [content_types addObject:type];
                        }
                        panel.allowedContentTypes = content_types;
                    } else {
                        // Keep the previously supported runtime floor. This deprecated API is
                        // isolated to old systems; modern macOS uses UTType above.
                        NSMutableArray* legacy_types = [NSMutableArray arrayWithCapacity:extensions.size()];
                        for (const std::string& extension : extensions) {
                            NSString* value = [[NSString alloc] initWithBytes:extension.data()
                                                                           length:extension.size()
                                                                         encoding:NSUTF8StringEncoding];
                            [legacy_types addObject:value];
                        }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                        panel.allowedFileTypes = legacy_types;
#pragma clang diagnostic pop
                    }
                }

                // The block owns only PanelState and the supplied completion. The completion itself
                // captures a weak CompletionState, never this service, its backend Impl or its queue.
                [panel beginSheetModalForWindow:owner completionHandler:
                    ^(NSModalResponse response) {
                        NSURL* selected = panel_state->panel.URL;
                        completion(response, selected);
                        panel_state->panel = nil;
                    }];
                active_panel_ = std::move(panel_state);
            } @catch (NSException* exception) {
                const char* reason = exception.reason.UTF8String;
                throw std::runtime_error{std::string{"NSOpenPanel start failed"} +
                                          (reason != nullptr ? std::string{": "} + reason : std::string{})};
            }
        }
    }

    void cancel() noexcept override {
        @autoreleasepool {
            if (active_panel_ != nullptr && active_panel_->panel != nil) {
                @try {
                    NSWindow* sheet_parent = active_panel_->panel.sheetParent;
                    if (sheet_parent != nil) {
                        [sheet_parent endSheet:active_panel_->panel returnCode:NSModalResponseCancel];
                    } else {
                        [active_panel_->panel close];
                    }
                }
                @catch (NSException*) {}
                active_panel_->panel = nil;
            }
            active_panel_.reset();
        }
    }

private:
    std::shared_ptr<PanelState> active_panel_;
};

class ProductionSelectFolderInvoker final : public NativeSelectFolderInvoker {
public:
    void begin(NSWindow* owner, NSString* title,
               const std::optional<std::filesystem::path>& initial_directory,
               NativeSelectFolderCompletion completion) override {
        @autoreleasepool {
            @try {
                auto panel_state = std::make_shared<PanelState>();
                NSOpenPanel* panel = [NSOpenPanel openPanel];
                panel_state->panel = panel;
                panel.canChooseFiles = NO;
                panel.canChooseDirectories = YES;
                panel.allowsMultipleSelection = NO;
                if (title != nil) panel.title = title;
                if (initial_directory.has_value()) {
                    panel.directoryURL = nativeDirectoryURL(*initial_directory);
                    if (panel.directoryURL == nil) {
                        throw std::runtime_error{"AppKit initial directory cannot be represented"};
                    }
                }
                [panel beginSheetModalForWindow:owner completionHandler:
                    ^(NSModalResponse response) {
                        NSURL* selected = panel_state->panel.URL;
                        completion(response, selected);
                        panel_state->panel = nil;
                    }];
                active_panel_ = std::move(panel_state);
            } @catch (NSException* exception) {
                const char* reason = exception.reason.UTF8String;
                throw std::runtime_error{std::string{"NSOpenPanel folder start failed"} +
                                          (reason != nullptr ? std::string{": "} + reason : std::string{})};
            }
        }
    }

    void cancel() noexcept override {
        @autoreleasepool {
            if (active_panel_ != nullptr && active_panel_->panel != nil) {
                @try {
                    NSWindow* sheet_parent = active_panel_->panel.sheetParent;
                    if (sheet_parent != nil) {
                        [sheet_parent endSheet:active_panel_->panel returnCode:NSModalResponseCancel];
                    } else {
                        [active_panel_->panel close];
                    }
                } @catch (NSException*) {}
                active_panel_->panel = nil;
            }
            active_panel_.reset();
        }
    }

private:
    std::shared_ptr<PanelState> active_panel_;
};

class ProductionSaveFileInvoker final : public NativeSaveFileInvoker {
public:
    void begin(NSWindow* owner, NSString* title, std::vector<std::string> extensions,
               NSString* suggested_filename,
               const std::optional<std::filesystem::path>& initial_directory,
               NativeSaveFileCompletion completion) override {
        @autoreleasepool {
            @try {
                auto panel_state = std::make_shared<PanelState>();
                NSSavePanel* panel = [NSSavePanel savePanel];
                panel_state->panel = panel;
                if (title != nil) panel.title = title;
                if (initial_directory.has_value()) {
                    panel.directoryURL = nativeDirectoryURL(*initial_directory);
                    if (panel.directoryURL == nil) {
                        throw std::runtime_error{"AppKit initial directory cannot be represented"};
                    }
                }
                if (suggested_filename != nil) panel.nameFieldStringValue = suggested_filename;
                if (!extensions.empty()) {
                    if (@available(macOS 11.0, *)) {
                        NSMutableArray* content_types =
                            [NSMutableArray arrayWithCapacity:extensions.size()];
                        for (const std::string& extension : extensions) {
                            NSString* value = [[NSString alloc] initWithBytes:extension.data()
                                                                           length:extension.size()
                                                                         encoding:NSUTF8StringEncoding];
                            if (value == nil) throw std::invalid_argument{
                                "AppKit file-dialog extension must be valid UTF-8"};
                            UTType* type = [UTType typeWithFilenameExtension:value];
                            if (type == nil) throw std::invalid_argument{
                                "AppKit file-dialog extension could not become a UTType"};
                            // Preserve order: the first content type is the portable preferred
                            // native extension. The panel, not this service, decides the result name.
                            [content_types addObject:type];
                        }
                        panel.allowedContentTypes = content_types;
                    } else {
                        // Keep the existing deployment floor. This deprecated fallback is local
                        // to old runtimes; modern macOS uses the ordered UTType projection above.
                        NSMutableArray* legacy_types =
                            [NSMutableArray arrayWithCapacity:extensions.size()];
                        for (const std::string& extension : extensions) {
                            NSString* value = [[NSString alloc] initWithBytes:extension.data()
                                                                           length:extension.size()
                                                                         encoding:NSUTF8StringEncoding];
                            if (value == nil) throw std::invalid_argument{
                                "AppKit file-dialog extension must be valid UTF-8"};
                            [legacy_types addObject:value];
                        }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                        panel.allowedFileTypes = legacy_types;
#pragma clang diagnostic pop
                    }
                }

                // NSSavePanel owns the platform's normal existing-file replacement confirmation.
                // The toolkit only transports the selected destination and never writes it.
                [panel beginSheetModalForWindow:owner completionHandler:
                    ^(NSModalResponse response) {
                        NSURL* selected = panel_state->panel.URL;
                        completion(response, selected);
                        panel_state->panel = nil;
                    }];
                active_panel_ = std::move(panel_state);
            } @catch (NSException* exception) {
                const char* reason = exception.reason.UTF8String;
                throw std::runtime_error{std::string{"NSSavePanel start failed"} +
                                          (reason != nullptr ? std::string{": "} + reason : std::string{})};
            }
        }
    }

    void cancel() noexcept override {
        @autoreleasepool {
            if (active_panel_ != nullptr && active_panel_->panel != nil) {
                @try {
                    NSWindow* sheet_parent = active_panel_->panel.sheetParent;
                    if (sheet_parent != nil) {
                        [sheet_parent endSheet:active_panel_->panel returnCode:NSModalResponseCancel];
                    } else {
                        [active_panel_->panel close];
                    }
                }
                @catch (NSException*) {}
                active_panel_->panel = nil;
            }
            active_panel_.reset();
        }
    }

private:
    std::shared_ptr<PanelState> active_panel_;
};

[[nodiscard]] NSString* strictTitle(const std::string& title) {
    for (std::size_t offset = 0; offset < title.size();) {
        const utf8::DecodedScalar decoded = utf8::decodeOne(title, offset);
        if (decoded.consumed == 0 || !decoded.valid) {
            throw std::invalid_argument{"AppKit file-dialog title must be valid UTF-8"};
        }
        offset += decoded.consumed;
    }
    if (title.empty()) return nil;
    NSString* result = [[NSString alloc] initWithBytes:title.data() length:title.size()
                                                  encoding:NSUTF8StringEncoding];
    if (result == nil) throw std::invalid_argument{"AppKit file-dialog title must be valid UTF-8"};
    return result;
}

[[nodiscard]] NSString* strictSuggestedFilename(const std::string& filename) {
    const std::string validated = sasd::ui::native::detail::validateSuggestedFilename(filename);
    if (validated.empty()) return nil;
    NSString* result = [[NSString alloc] initWithBytes:validated.data() length:validated.size()
                                                  encoding:NSUTF8StringEncoding];
    if (result == nil) {
        throw std::invalid_argument{"AppKit suggested filename must be valid UTF-8"};
    }
    return result;
}

[[nodiscard]] NSURL* nativeDirectoryURL(const std::filesystem::path& directory) {
    const auto native = directory.native();
    return [NSURL fileURLWithFileSystemRepresentation:native.c_str()
                                           isDirectory:YES
                                         relativeToURL:nil];
}

[[nodiscard]] std::optional<std::filesystem::path> nativeFilePath(NSURL* url) {
    if (url == nil || !url.isFileURL) return std::nullopt;
    const char* representation = url.fileSystemRepresentation;
    if (representation == nullptr || *representation == '\0') return std::nullopt;
    return std::filesystem::path{representation};
}

} // namespace

struct AppKitFileDialogService::CompletionState {
    EventQueue* queue{}; // non-owning while the backend is alive
    FileDialogRequestId request_id{};
    std::uint64_t generation{};
    bool active{true};
    bool pending{false};
    bool completed{false};
};

AppKitFileDialogService::AppKitFileDialogService(NSWindow* owner, EventQueue& events,
                                                 std::unique_ptr<NativeOpenFileInvoker> open_invoker,
                                                 std::unique_ptr<NativeSaveFileInvoker> save_invoker,
                                                 std::unique_ptr<NativeSelectFolderInvoker> folder_invoker)
    : owner_{owner},
      open_invoker_{std::move(open_invoker)},
      save_invoker_{std::move(save_invoker)},
      folder_invoker_{std::move(folder_invoker)},
      state_{std::make_shared<CompletionState>()} {
    state_->queue = &events;
    if (open_invoker_ == nullptr) open_invoker_ = std::make_unique<ProductionOpenFileInvoker>();
    if (save_invoker_ == nullptr) save_invoker_ = std::make_unique<ProductionSaveFileInvoker>();
    if (folder_invoker_ == nullptr) {
        folder_invoker_ = std::make_unique<ProductionSelectFolderInvoker>();
    }
}

AppKitFileDialogService::~AppKitFileDialogService() { shutdown(); }

bool AppKitFileDialogService::pending() const noexcept { return state_->pending; }

void AppKitFileDialogService::shutdown() noexcept {
    if (state_ == nullptr || !state_->active) return;
    // Invalidate first. AppKit may synchronously deliver cancellation during cancel(), and that
    // late completion must observe an inert state before the queue/window is torn down.
    state_->active = false;
    state_->pending = false;
    state_->completed = true;
    state_->queue = nullptr;
    ++state_->generation;
    if (open_invoker_ != nullptr) open_invoker_->cancel();
    if (save_invoker_ != nullptr) save_invoker_->cancel();
    if (folder_invoker_ != nullptr) folder_invoker_->cancel();
    owner_ = nullptr;
}

FileDialogRequestId AppKitFileDialogService::requestOpenFile(OpenFileDialogRequest request) {
    if (![NSThread isMainThread]) {
        throw std::runtime_error{"AppKit file dialogs must start on the AppKit main thread"};
    }
    if (state_->queue == nullptr || owner_ == nil) {
        throw std::logic_error{"AppKit file dialog requires initialize()"};
    }
    NSString* title = strictTitle(request.title); // setup validation precedes ID allocation
    const std::vector<std::string> extensions =
        sasd::ui::native::detail::validateFileDialogExtensions(request.extensions);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    if (state_->pending) throw std::logic_error{"AppKit file dialog already has a pending request"};
    if (next_request_id_ == 0) throw std::overflow_error{"AppKit file-dialog request ID space exhausted"};

    const FileDialogRequestId id = next_request_id_++;
    const std::uint64_t generation = ++state_->generation;
    state_->request_id = id;
    state_->pending = true;
    state_->completed = false;
    const std::weak_ptr<CompletionState> weak_state = state_;
    const auto completion = [weak_state, id, generation](NSModalResponse response, NSURL* url) noexcept {
        try {
            const std::shared_ptr<CompletionState> state = weak_state.lock();
            if (state == nullptr || !state->active || !state->pending || state->completed ||
                state->request_id != id || state->generation != generation || state->queue == nullptr) return;

            FileDialogResultStatus status = FileDialogResultStatus::failed;
            std::optional<std::filesystem::path> path;
            if (response == NSModalResponseCancel) {
                status = FileDialogResultStatus::cancelled;
            } else if (response == NSModalResponseOK) {
                path = nativeFilePath(url);
                if (path && !path->empty()) status = FileDialogResultStatus::accepted;
            }
            state->completed = true;
            state->queue->push(Event{FileDialogResultEvent{id, status, std::move(path)}});
            state->pending = false;
        } catch (...) {
            // Native completion never lets Objective-C/C++ exceptions cross its boundary. If
            // queue allocation fails, retire the operation; no stale completion can reuse it.
            if (const std::shared_ptr<CompletionState> state = weak_state.lock()) {
                state->completed = true;
                state->pending = false;
            }
        }
    };

    try {
        open_invoker_->begin(owner_, title, extensions, initial_directory, completion);
    } catch (...) {
        // There is no public "other" response constant on all supported SDKs; any value other
        // than OK/Cancel is deliberately mapped to failed by the completion contract.
        completion(static_cast<NSModalResponse>(-1), nil);
    }
    return id;
}

FileDialogRequestId AppKitFileDialogService::requestSaveFile(SaveFileDialogRequest request) {
    if (![NSThread isMainThread]) {
        throw std::runtime_error{"AppKit file dialogs must start on the AppKit main thread"};
    }
    if (state_->queue == nullptr || owner_ == nil) {
        throw std::logic_error{"AppKit file dialog requires initialize()"};
    }
    NSString* title = strictTitle(request.title);
    NSString* suggested_filename = strictSuggestedFilename(request.suggested_filename);
    const std::vector<std::string> extensions =
        sasd::ui::native::detail::validateFileDialogExtensions(request.extensions);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    if (state_->pending) throw std::logic_error{"AppKit file dialog already has a pending request"};
    if (next_request_id_ == 0) throw std::overflow_error{"AppKit file-dialog request ID space exhausted"};

    const FileDialogRequestId id = next_request_id_++;
    const std::uint64_t generation = ++state_->generation;
    state_->request_id = id;
    state_->pending = true;
    state_->completed = false;
    const std::weak_ptr<CompletionState> weak_state = state_;
    const NativeSaveFileCompletion completion = [weak_state, id, generation](
                                                    NSModalResponse response, NSURL* url) noexcept {
        try {
            const std::shared_ptr<CompletionState> state = weak_state.lock();
            if (state == nullptr || !state->active || !state->pending || state->completed ||
                state->request_id != id || state->generation != generation || state->queue == nullptr) return;

            FileDialogResultStatus status = FileDialogResultStatus::failed;
            std::optional<std::filesystem::path> path;
            if (response == NSModalResponseCancel) {
                status = FileDialogResultStatus::cancelled;
            } else if (response == NSModalResponseOK) {
                path = nativeFilePath(url);
                if (path && !path->empty()) status = FileDialogResultStatus::accepted;
            }
            state->completed = true;
            state->queue->push(Event{FileDialogResultEvent{id, status, std::move(path)}});
            state->pending = false;
        } catch (...) {
            if (const std::shared_ptr<CompletionState> state = weak_state.lock()) {
                state->completed = true;
                state->pending = false;
            }
        }
    };

    try {
        save_invoker_->begin(owner_, title, extensions, suggested_filename, initial_directory, completion);
    } catch (...) {
        completion(static_cast<NSModalResponse>(-1), nil);
    }
    return id;
}

FileDialogRequestId AppKitFileDialogService::requestSelectFolder(SelectFolderDialogRequest request) {
    if (![NSThread isMainThread]) {
        throw std::runtime_error{"AppKit file dialogs must start on the AppKit main thread"};
    }
    if (state_->queue == nullptr || owner_ == nil) {
        throw std::logic_error{"AppKit file dialog requires initialize()"};
    }
    NSString* title = strictTitle(request.title);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    if (state_->pending) throw std::logic_error{"AppKit file dialog already has a pending request"};
    if (next_request_id_ == 0) throw std::overflow_error{"AppKit file-dialog request ID space exhausted"};

    const FileDialogRequestId id = next_request_id_++;
    const std::uint64_t generation = ++state_->generation;
    state_->request_id = id;
    state_->pending = true;
    state_->completed = false;
    const std::weak_ptr<CompletionState> weak_state = state_;
    const NativeSelectFolderCompletion completion = [weak_state, id, generation](
                                                        NSModalResponse response, NSURL* url) noexcept {
        try {
            const std::shared_ptr<CompletionState> state = weak_state.lock();
            if (state == nullptr || !state->active || !state->pending || state->completed ||
                state->request_id != id || state->generation != generation || state->queue == nullptr) return;
            FileDialogResultStatus status = FileDialogResultStatus::failed;
            std::optional<std::filesystem::path> path;
            if (response == NSModalResponseCancel) {
                status = FileDialogResultStatus::cancelled;
            } else if (response == NSModalResponseOK) {
                path = nativeFilePath(url);
                if (path && !path->empty()) status = FileDialogResultStatus::accepted;
            }
            state->completed = true;
            state->queue->push(Event{FileDialogResultEvent{id, status, std::move(path)}});
            state->pending = false;
        } catch (...) {
            if (const std::shared_ptr<CompletionState> state = weak_state.lock()) {
                state->completed = true;
                state->pending = false;
            }
        }
    };

    try {
        folder_invoker_->begin(owner_, title, initial_directory, completion);
    } catch (...) {
        completion(static_cast<NSModalResponse>(-1), nil);
    }
    return id;
}

} // namespace sasd::ui::native::appkit::detail
