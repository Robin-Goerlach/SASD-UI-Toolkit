#pragma once

#include <sasd/ui/events/event_queue.hpp>
#include <sasd/ui/file_dialog.hpp>

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace sasd::ui::native::gtk::detail {

// The GFile is borrowed for the duration of the completion call. The production invoker owns the
// GFile returned by GTK and releases it immediately after the service has copied its local path.
using NativeOpenFileCompletion = std::function<void(FileDialogResultStatus, GFile*)>;

/** Private seam: production invokes the selected GTK4 native API; tests never show a chooser. */
class NativeOpenFileInvoker {
public:
    virtual ~NativeOpenFileInvoker() = default;
    virtual void begin(GtkWindow* owner, std::string title, NativeOpenFileCompletion completion) = 0;
    virtual void cancel() noexcept = 0;
};

/** Private GTK consumer of the backend-neutral FileDialogService contract. */
class GtkFileDialogService final : public FileDialogService {
public:
    GtkFileDialogService(GtkWindow* owner, EventQueue& events, std::thread::id owner_thread,
                         std::unique_ptr<NativeOpenFileInvoker> invoker = nullptr);
    ~GtkFileDialogService() override;

    GtkFileDialogService(const GtkFileDialogService&) = delete;
    GtkFileDialogService& operator=(const GtkFileDialogService&) = delete;

    [[nodiscard]] FileDialogRequestId requestOpenFile(OpenFileDialogRequest request) override;
    [[nodiscard]] bool pending() const noexcept;
    void shutdown() noexcept;

private:
    struct CompletionState;

    GtkWindow* owner_{}; // non-owning: GtkWindowBackend owns the window
    EventQueue* events_{}; // non-owning and nulled before backend teardown
    std::thread::id owner_thread_{};
    std::unique_ptr<NativeOpenFileInvoker> invoker_;
    std::shared_ptr<CompletionState> state_;
    FileDialogRequestId next_request_id_{1};
};

} // namespace sasd::ui::native::gtk::detail
