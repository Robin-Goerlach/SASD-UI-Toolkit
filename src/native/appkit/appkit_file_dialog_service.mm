#include "appkit_file_dialog_service.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <limits>
#include <stdexcept>
#include <utility>

namespace sasd::ui::native::appkit::detail {
namespace {

struct PanelState {
    NSSavePanel* __strong panel{nil};
};

class ProductionOpenFileInvoker final : public NativeOpenFileInvoker {
public:
    void begin(NSWindow* owner, NSString* title, NativeOpenFileCompletion completion) override {
        @autoreleasepool {
            @try {
                auto panel_state = std::make_shared<PanelState>();
                NSOpenPanel* panel = [NSOpenPanel openPanel];
                panel_state->panel = panel;
                panel.canChooseFiles = YES;
                panel.canChooseDirectories = NO;
                panel.allowsMultipleSelection = NO;
                if (title != nil) panel.title = title;

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

class ProductionSaveFileInvoker final : public NativeSaveFileInvoker {
public:
    void begin(NSWindow* owner, NSString* title, NativeSaveFileCompletion completion) override {
        @autoreleasepool {
            @try {
                auto panel_state = std::make_shared<PanelState>();
                NSSavePanel* panel = [NSSavePanel savePanel];
                panel_state->panel = panel;
                if (title != nil) panel.title = title;

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
                                                 std::unique_ptr<NativeSaveFileInvoker> save_invoker)
    : owner_{owner},
      open_invoker_{std::move(open_invoker)},
      save_invoker_{std::move(save_invoker)},
      state_{std::make_shared<CompletionState>()} {
    state_->queue = &events;
    if (open_invoker_ == nullptr) open_invoker_ = std::make_unique<ProductionOpenFileInvoker>();
    if (save_invoker_ == nullptr) save_invoker_ = std::make_unique<ProductionSaveFileInvoker>();
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
        open_invoker_->begin(owner_, title, completion);
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
        save_invoker_->begin(owner_, title, completion);
    } catch (...) {
        completion(static_cast<NSModalResponse>(-1), nil);
    }
    return id;
}

} // namespace sasd::ui::native::appkit::detail
