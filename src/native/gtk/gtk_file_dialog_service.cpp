#include "gtk_file_dialog_service.hpp"

#include "../file_dialog_extension_filter.hpp"
#include "../file_dialog_initial_directory.hpp"
#include "../file_dialog_suggested_filename.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace sasd::ui::native::gtk::detail {
namespace {

struct CallbackContext {
    NativeOpenFileCompletion completion;
};

struct FilesCallbackContext {
    NativeOpenFilesCompletion completion;
};

[[nodiscard]] GFile* makeInitialFolder(const std::filesystem::path& directory) {
    // The full GFile reference is released by the caller after the native setter has retained
    // or copied the folder. No URI conversion or filesystem query is performed.
    return g_file_new_for_path(directory.c_str());
}

#if GTK_CHECK_VERSION(4, 10, 0)
void setInitialFolder(GtkFileDialog* dialog,
                      const std::optional<std::filesystem::path>& initial_directory) {
    if (!initial_directory.has_value()) return;
    GFile* folder = makeInitialFolder(*initial_directory);
    if (folder == nullptr) throw std::runtime_error{"GTK initial directory creation failed"};
    gtk_file_dialog_set_initial_folder(dialog, folder);
    g_object_unref(folder);
}
#else
void setInitialFolder(GtkFileChooser* chooser,
                      const std::optional<std::filesystem::path>& initial_directory) {
    if (!initial_directory.has_value()) return;
    GFile* folder = makeInitialFolder(*initial_directory);
    if (folder == nullptr) throw std::runtime_error{"GTK initial directory creation failed"};
    GError* error = nullptr;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    const gboolean accepted = gtk_file_chooser_set_current_folder(chooser, folder, &error);
#pragma GCC diagnostic pop
    g_object_unref(folder);
    if (!accepted || error != nullptr) {
        if (error != nullptr) g_error_free(error);
        throw std::runtime_error{"GTK initial directory setup failed"};
    }
}
#endif

[[nodiscard]] std::string filterPatternList(const std::vector<std::string>& extensions) {
    std::string result;
    for (std::size_t index = 0; index < extensions.size(); ++index) {
        if (index != 0) result += "; ";
        result += "*.";
        result += extensions[index];
    }
    return result;
}

#if !GTK_CHECK_VERSION(4, 10, 0)
void addFileFilter(GtkFileChooser* chooser, const std::vector<std::string>& extensions) {
    if (extensions.empty()) return;
    GtkFileFilter* filter = gtk_file_filter_new();
    if (filter == nullptr) throw std::runtime_error{"GTK file filter creation failed"};
    const std::string name = filterPatternList(extensions);
    gtk_file_filter_set_name(filter, name.c_str());
    for (const std::string& extension : extensions) {
        const std::string pattern = "*." + extension;
        gtk_file_filter_add_pattern(filter, pattern.c_str());
    }
    // GtkFileChooser takes ownership of this newly-created filter reference. This is the
    // compatibility-only API for GTK4 headers older than 4.10; keep its deprecation warning local.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gtk_file_chooser_add_filter(chooser, filter);
#pragma GCC diagnostic pop
}
#endif

#if GTK_CHECK_VERSION(4, 10, 0)
void setFileFilter(GtkFileDialog* dialog, const std::vector<std::string>& extensions) {
    if (extensions.empty()) return;
    GtkFileFilter* filter = gtk_file_filter_new();
    if (filter == nullptr) throw std::runtime_error{"GTK file filter creation failed"};
    const std::string name = filterPatternList(extensions);
    gtk_file_filter_set_name(filter, name.c_str());
    for (const std::string& extension : extensions) {
        const std::string pattern = "*." + extension;
        gtk_file_filter_add_pattern(filter, pattern.c_str());
    }
    GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    if (filters == nullptr) {
        g_object_unref(filter);
        throw std::runtime_error{"GTK file filter list creation failed"};
    }
    // The list model and default filter retain their own references; release both local full
    // references after the setters. GtkFileDialog keeps the model for the async operation.
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    gtk_file_dialog_set_default_filter(dialog, filter);
    g_object_unref(filters);
    g_object_unref(filter);
}
#endif

[[nodiscard]] std::optional<std::filesystem::path> localPath(GFile* file) {
    if (file == nullptr) return std::nullopt;
    // g_file_get_path() returns newly allocated native bytes, not a borrowed URI string. Copy them
    // into std::filesystem::path before freeing the GLib allocation; no canonicalization or
    // existence check belongs to this request/result contract.
    gchar* native_path = g_file_get_path(file);
    if (native_path == nullptr || *native_path == '\0') {
        g_free(native_path);
        return std::nullopt;
    }
    std::filesystem::path result{native_path};
    g_free(native_path);
    if (result.empty()) return std::nullopt;
    return result;
}

[[nodiscard]] std::string strictTitle(std::string_view title) {
    if (title.find('\0') != std::string_view::npos) {
        throw std::invalid_argument{"GTK file-dialog title must not contain NUL"};
    }
    for (std::size_t offset = 0; offset < title.size();) {
        const utf8::DecodedScalar decoded = utf8::decodeOne(title, offset);
        if (decoded.consumed == 0 || !decoded.valid) {
            throw std::invalid_argument{"GTK file-dialog title must be valid UTF-8"};
        }
        offset += decoded.consumed;
    }
    return std::string{title};
}

[[nodiscard]] std::string strictSuggestedFilename(std::string_view filename) {
    return sasd::ui::native::detail::validateSuggestedFilename(filename);
}

class ProductionOpenFileInvoker final : public NativeOpenFileInvoker {
public:
    ~ProductionOpenFileInvoker() override { cancel(); }

    void begin(GtkWindow* owner, std::string title, std::vector<std::string> extensions,
               const std::optional<std::filesystem::path>& initial_directory,
               NativeOpenFileCompletion completion) override {
        cancel();
        if (owner == nullptr) throw std::logic_error{"GTK file dialog requires a window owner"};

#if GTK_CHECK_VERSION(4, 10, 0)
        GtkFileDialog* dialog = gtk_file_dialog_new();
        if (dialog == nullptr) throw std::runtime_error{"GtkFileDialog creation failed"};
        if (!title.empty()) gtk_file_dialog_set_title(dialog, title.c_str());
        setFileFilter(dialog, extensions);
        setInitialFolder(dialog, initial_directory);
        auto* context = new CallbackContext{std::move(completion)};
        active_dialog_ = G_OBJECT(dialog);
        active_cancellable_ = g_cancellable_new();
        gtk_file_dialog_open(dialog, owner, active_cancellable_, &completeFileDialog, context);
#else
        // GtkFileDialog is only available since GTK 4.10. GtkFileChooserNative remains the
        // compatible asynchronous response API on older GTK4 installations, so this fallback does
        // not silently raise the project's GTK minimum merely to add Open File.
        GtkFileChooserNative* dialog = gtk_file_chooser_native_new(
            title.empty() ? nullptr : title.c_str(), owner, GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Open", "_Cancel");
        if (dialog == nullptr) throw std::runtime_error{"GtkFileChooserNative creation failed"};
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);
        addFileFilter(GTK_FILE_CHOOSER(dialog), extensions);
        setInitialFolder(GTK_FILE_CHOOSER(dialog), initial_directory);
        auto* context = new CallbackContext{std::move(completion)};
        active_dialog_ = G_OBJECT(dialog);
        g_signal_connect_data(dialog, "response", G_CALLBACK(&completeChooser), context,
                              &destroyContext, G_CONNECT_DEFAULT);
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
#endif
    }

    void cancel() noexcept override {
        if (active_dialog_ == nullptr) return;
#if GTK_CHECK_VERSION(4, 10, 0)
        if (active_cancellable_ != nullptr) g_cancellable_cancel(active_cancellable_);
        if (active_cancellable_ != nullptr) {
            g_object_unref(active_cancellable_);
            active_cancellable_ = nullptr;
        }
        g_object_unref(active_dialog_);
#else
        gtk_native_dialog_hide(GTK_NATIVE_DIALOG(active_dialog_));
        g_object_unref(active_dialog_);
#endif
        active_dialog_ = nullptr;
    }

private:
    static void destroyContext(gpointer data, GClosure*) noexcept {
        delete static_cast<CallbackContext*>(data);
    }

#if GTK_CHECK_VERSION(4, 10, 0)
    static void completeFileDialog(GObject* source, GAsyncResult* result, gpointer data) noexcept {
        std::unique_ptr<CallbackContext> context{static_cast<CallbackContext*>(data)};
        GError* error = nullptr;
        GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &error);
        FileDialogResultStatus status = FileDialogResultStatus::failed;
        if (error != nullptr) {
            if (error->domain == GTK_DIALOG_ERROR && error->code == GTK_DIALOG_ERROR_DISMISSED) {
                status = FileDialogResultStatus::cancelled;
            }
            g_error_free(error);
        } else if (file != nullptr) {
            status = FileDialogResultStatus::accepted;
        }
        try {
            context->completion(status, file);
        } catch (...) {
            // No C++ exception may cross a GLib callback boundary.
        }
        if (file != nullptr) g_object_unref(file); // open_finish transfers a full GFile reference
    }
#else
    static void completeChooser(GtkNativeDialog* native_dialog, int response,
                                gpointer data) noexcept {
        // The signal's destroy notifier owns CallbackContext if shutdown dismisses the native
        // dialog before a response. On a normal response GTK keeps the chooser alive until the
        // invoker releases it, so this callback must not delete the context itself.
        auto* context = static_cast<CallbackContext*>(data);
        GFile* file = nullptr;
        FileDialogResultStatus status = FileDialogResultStatus::failed;
        if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
            status = FileDialogResultStatus::cancelled;
        } else if (response == GTK_RESPONSE_ACCEPT) {
            // gtk_file_chooser_get_file() transfers a full reference to the caller.
            file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(native_dialog));
            if (file != nullptr) status = FileDialogResultStatus::accepted;
        }
        try {
            context->completion(status, file);
        } catch (...) {
        }
        if (file != nullptr) g_object_unref(file);
    }
#endif

    GObject* active_dialog_{nullptr};
    GCancellable* active_cancellable_{nullptr};
};

class ProductionOpenFilesInvoker final : public NativeOpenFilesInvoker {
public:
    ~ProductionOpenFilesInvoker() override { cancel(); }

    void begin(GtkWindow* owner, std::string title, std::vector<std::string> extensions,
               const std::optional<std::filesystem::path>& initial_directory,
               NativeOpenFilesCompletion completion) override {
        cancel();
        if (owner == nullptr) throw std::logic_error{"GTK file dialog requires a window owner"};
#if GTK_CHECK_VERSION(4, 10, 0)
        GtkFileDialog* dialog = gtk_file_dialog_new();
        if (dialog == nullptr) throw std::runtime_error{"GtkFileDialog creation failed"};
        if (!title.empty()) gtk_file_dialog_set_title(dialog, title.c_str());
        setFileFilter(dialog, extensions);
        setInitialFolder(dialog, initial_directory);
        auto* context = new FilesCallbackContext{std::move(completion)};
        active_dialog_ = G_OBJECT(dialog);
        active_cancellable_ = g_cancellable_new();
        gtk_file_dialog_open_multiple(dialog, owner, active_cancellable_, &completeFileDialog, context);
#else
        GtkFileChooserNative* dialog = gtk_file_chooser_native_new(
            title.empty() ? nullptr : title.c_str(), owner, GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Open", "_Cancel");
        if (dialog == nullptr) throw std::runtime_error{"GtkFileChooserNative creation failed"};
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
        addFileFilter(GTK_FILE_CHOOSER(dialog), extensions);
        setInitialFolder(GTK_FILE_CHOOSER(dialog), initial_directory);
        auto* context = new FilesCallbackContext{std::move(completion)};
        active_dialog_ = G_OBJECT(dialog);
        g_signal_connect_data(dialog, "response", G_CALLBACK(&completeChooser), context,
                              &destroyContext, G_CONNECT_DEFAULT);
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
#endif
    }

    void cancel() noexcept override {
        if (active_dialog_ == nullptr) return;
#if GTK_CHECK_VERSION(4, 10, 0)
        if (active_cancellable_ != nullptr) g_cancellable_cancel(active_cancellable_);
        if (active_cancellable_ != nullptr) {
            g_object_unref(active_cancellable_);
            active_cancellable_ = nullptr;
        }
        g_object_unref(active_dialog_);
#else
        gtk_native_dialog_hide(GTK_NATIVE_DIALOG(active_dialog_));
        g_object_unref(active_dialog_);
#endif
        active_dialog_ = nullptr;
    }

private:
    static void destroyContext(gpointer data, GClosure*) noexcept {
        delete static_cast<FilesCallbackContext*>(data);
    }

#if GTK_CHECK_VERSION(4, 10, 0)
    static void completeFileDialog(GObject* source, GAsyncResult* result, gpointer data) noexcept {
        std::unique_ptr<FilesCallbackContext> context{static_cast<FilesCallbackContext*>(data)};
        GError* error = nullptr;
        GListModel* model = gtk_file_dialog_open_multiple_finish(
            GTK_FILE_DIALOG(source), result, &error);
        FileDialogResultStatus status = FileDialogResultStatus::failed;
        if (error != nullptr) {
            if (error->domain == GTK_DIALOG_ERROR && error->code == GTK_DIALOG_ERROR_DISMISSED) {
                status = FileDialogResultStatus::cancelled;
            }
            g_error_free(error);
        } else if (model != nullptr) {
            status = FileDialogResultStatus::accepted;
        }
        try { context->completion(status, {model, nullptr}); } catch (...) {}
        if (model != nullptr) g_object_unref(model);
    }
#else
    static void completeChooser(GtkNativeDialog* native_dialog, int response,
                                gpointer data) noexcept {
        auto* context = static_cast<FilesCallbackContext*>(data);
        GSList* files = nullptr;
        FileDialogResultStatus status = FileDialogResultStatus::failed;
        if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
            status = FileDialogResultStatus::cancelled;
        } else if (response == GTK_RESPONSE_ACCEPT) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(native_dialog));
#pragma GCC diagnostic pop
            if (files != nullptr) status = FileDialogResultStatus::accepted;
        }
        try { context->completion(status, {nullptr, files}); } catch (...) {}
        if (files != nullptr) g_slist_free_full(files, g_object_unref);
    }
#endif

    GObject* active_dialog_{nullptr};
    GCancellable* active_cancellable_{nullptr};
};

class ProductionSelectFolderInvoker final : public NativeSelectFolderInvoker {
public:
    ~ProductionSelectFolderInvoker() override { cancel(); }

    void begin(GtkWindow* owner, std::string title,
               const std::optional<std::filesystem::path>& initial_directory,
               NativeSelectFolderCompletion completion) override {
        cancel();
        if (owner == nullptr) throw std::logic_error{"GTK file dialog requires a window owner"};
#if GTK_CHECK_VERSION(4, 10, 0)
        GtkFileDialog* dialog = gtk_file_dialog_new();
        if (dialog == nullptr) throw std::runtime_error{"GtkFileDialog creation failed"};
        if (!title.empty()) gtk_file_dialog_set_title(dialog, title.c_str());
        setInitialFolder(dialog, initial_directory);
        auto* context = new CallbackContext{std::move(completion)};
        active_dialog_ = G_OBJECT(dialog);
        active_cancellable_ = g_cancellable_new();
        gtk_file_dialog_select_folder(dialog, owner, active_cancellable_, &completeFileDialog, context);
#else
        GtkFileChooserNative* dialog = gtk_file_chooser_native_new(
            title.empty() ? nullptr : title.c_str(), owner, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
            "_Select", "_Cancel");
        if (dialog == nullptr) throw std::runtime_error{"GtkFileChooserNative creation failed"};
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);
        setInitialFolder(GTK_FILE_CHOOSER(dialog), initial_directory);
        auto* context = new CallbackContext{std::move(completion)};
        active_dialog_ = G_OBJECT(dialog);
        g_signal_connect_data(dialog, "response", G_CALLBACK(&completeChooser), context,
                              &destroyContext, G_CONNECT_DEFAULT);
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
#endif
    }

    void cancel() noexcept override {
        if (active_dialog_ == nullptr) return;
#if GTK_CHECK_VERSION(4, 10, 0)
        if (active_cancellable_ != nullptr) g_cancellable_cancel(active_cancellable_);
        if (active_cancellable_ != nullptr) {
            g_object_unref(active_cancellable_);
            active_cancellable_ = nullptr;
        }
        g_object_unref(active_dialog_);
#else
        gtk_native_dialog_hide(GTK_NATIVE_DIALOG(active_dialog_));
        g_object_unref(active_dialog_);
#endif
        active_dialog_ = nullptr;
    }

private:
    static void destroyContext(gpointer data, GClosure*) noexcept {
        delete static_cast<CallbackContext*>(data);
    }

#if GTK_CHECK_VERSION(4, 10, 0)
    static void completeFileDialog(GObject* source, GAsyncResult* result, gpointer data) noexcept {
        std::unique_ptr<CallbackContext> context{static_cast<CallbackContext*>(data)};
        GError* error = nullptr;
        GFile* file = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
        FileDialogResultStatus status = FileDialogResultStatus::failed;
        if (error != nullptr) {
            if (error->domain == GTK_DIALOG_ERROR && error->code == GTK_DIALOG_ERROR_DISMISSED) {
                status = FileDialogResultStatus::cancelled;
            }
            g_error_free(error);
        } else if (file != nullptr) {
            status = FileDialogResultStatus::accepted;
        }
        try { context->completion(status, file); } catch (...) {}
        if (file != nullptr) g_object_unref(file);
    }
#else
    static void completeChooser(GtkNativeDialog* native_dialog, int response, gpointer data) noexcept {
        auto* context = static_cast<CallbackContext*>(data);
        GFile* file = nullptr;
        FileDialogResultStatus status = FileDialogResultStatus::failed;
        if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
            status = FileDialogResultStatus::cancelled;
        } else if (response == GTK_RESPONSE_ACCEPT) {
            file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(native_dialog));
            if (file != nullptr) status = FileDialogResultStatus::accepted;
        }
        try { context->completion(status, file); } catch (...) {}
        if (file != nullptr) g_object_unref(file);
    }
#endif

    GObject* active_dialog_{nullptr};
    GCancellable* active_cancellable_{nullptr};
};

class ProductionSaveFileInvoker final : public NativeSaveFileInvoker {
public:
    ~ProductionSaveFileInvoker() override { cancel(); }

    void begin(GtkWindow* owner, std::string title, std::vector<std::string> extensions,
               std::string suggested_filename,
               const std::optional<std::filesystem::path>& initial_directory,
               NativeSaveFileCompletion completion) override {
        cancel();
        if (owner == nullptr) throw std::logic_error{"GTK file dialog requires a window owner"};

#if GTK_CHECK_VERSION(4, 10, 0)
        GtkFileDialog* dialog = gtk_file_dialog_new();
        if (dialog == nullptr) throw std::runtime_error{"GtkFileDialog creation failed"};
        if (!title.empty()) gtk_file_dialog_set_title(dialog, title.c_str());
        setFileFilter(dialog, extensions);
        setInitialFolder(dialog, initial_directory);
        if (!suggested_filename.empty()) {
            gtk_file_dialog_set_initial_name(dialog, suggested_filename.c_str());
        }
        auto* context = new CallbackContext{std::move(completion)};
        active_dialog_ = G_OBJECT(dialog);
        active_cancellable_ = g_cancellable_new();
        gtk_file_dialog_save(dialog, owner, active_cancellable_, &completeFileDialog, context);
#else
        GtkFileChooserNative* dialog = gtk_file_chooser_native_new(
            title.empty() ? nullptr : title.c_str(), owner, GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Save", "_Cancel");
        if (dialog == nullptr) throw std::runtime_error{"GtkFileChooserNative creation failed"};
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);
        addFileFilter(GTK_FILE_CHOOSER(dialog), extensions);
        setInitialFolder(GTK_FILE_CHOOSER(dialog), initial_directory);
        if (!suggested_filename.empty()) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), suggested_filename.c_str());
#pragma GCC diagnostic pop
        }
        // GTK4's save chooser owns its stock overwrite confirmation. The removed GTK3
        // do-overwrite-confirmation property must not be guessed at or emulated here.
        auto* context = new CallbackContext{std::move(completion)};
        active_dialog_ = G_OBJECT(dialog);
        g_signal_connect_data(dialog, "response", G_CALLBACK(&completeChooser), context,
                              &destroyContext, G_CONNECT_DEFAULT);
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
#endif
    }

    void cancel() noexcept override {
        if (active_dialog_ == nullptr) return;
#if GTK_CHECK_VERSION(4, 10, 0)
        if (active_cancellable_ != nullptr) g_cancellable_cancel(active_cancellable_);
        if (active_cancellable_ != nullptr) {
            g_object_unref(active_cancellable_);
            active_cancellable_ = nullptr;
        }
        g_object_unref(active_dialog_);
#else
        gtk_native_dialog_hide(GTK_NATIVE_DIALOG(active_dialog_));
        g_object_unref(active_dialog_);
#endif
        active_dialog_ = nullptr;
    }

private:
    static void destroyContext(gpointer data, GClosure*) noexcept {
        delete static_cast<CallbackContext*>(data);
    }

#if GTK_CHECK_VERSION(4, 10, 0)
    static void completeFileDialog(GObject* source, GAsyncResult* result, gpointer data) noexcept {
        std::unique_ptr<CallbackContext> context{static_cast<CallbackContext*>(data)};
        GError* error = nullptr;
        GFile* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, &error);
        FileDialogResultStatus status = FileDialogResultStatus::failed;
        if (error != nullptr) {
            if (error->domain == GTK_DIALOG_ERROR && error->code == GTK_DIALOG_ERROR_DISMISSED) {
                status = FileDialogResultStatus::cancelled;
            }
            g_error_free(error);
        } else if (file != nullptr) {
            status = FileDialogResultStatus::accepted;
        }
        try {
            context->completion(status, file);
        } catch (...) {
        }
        if (file != nullptr) g_object_unref(file);
    }
#else
    static void completeChooser(GtkNativeDialog* native_dialog, int response,
                                gpointer data) noexcept {
        auto* context = static_cast<CallbackContext*>(data);
        GFile* file = nullptr;
        FileDialogResultStatus status = FileDialogResultStatus::failed;
        if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
            status = FileDialogResultStatus::cancelled;
        } else if (response == GTK_RESPONSE_ACCEPT) {
            file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(native_dialog));
            if (file != nullptr) status = FileDialogResultStatus::accepted;
        }
        try {
            context->completion(status, file);
        } catch (...) {
        }
        if (file != nullptr) g_object_unref(file);
    }
#endif

    GObject* active_dialog_{nullptr};
    GCancellable* active_cancellable_{nullptr};
};

} // namespace

struct GtkFileDialogService::CompletionState {
    EventQueue* queue{};
    FileDialogRequestId request_id{};
    std::uint64_t generation{};
    bool active{true};
    bool pending{false};
    bool completed{false};
};

GtkFileDialogService::GtkFileDialogService(GtkWindow* owner, EventQueue& events,
                                           std::thread::id owner_thread,
                                           std::unique_ptr<NativeOpenFileInvoker> open_invoker,
                                           std::unique_ptr<NativeSaveFileInvoker> save_invoker,
                                           std::unique_ptr<NativeSelectFolderInvoker> folder_invoker,
                                           std::unique_ptr<NativeOpenFilesInvoker> open_files_invoker)
    : owner_{owner}, events_{&events}, owner_thread_{owner_thread},
      open_invoker_{std::move(open_invoker)},
      open_files_invoker_{std::move(open_files_invoker)},
      save_invoker_{std::move(save_invoker)},
      folder_invoker_{std::move(folder_invoker)},
      state_{std::make_shared<CompletionState>()} {
    if (open_invoker_ == nullptr) open_invoker_ = std::make_unique<ProductionOpenFileInvoker>();
    if (open_files_invoker_ == nullptr) {
        open_files_invoker_ = std::make_unique<ProductionOpenFilesInvoker>();
    }
    if (save_invoker_ == nullptr) save_invoker_ = std::make_unique<ProductionSaveFileInvoker>();
    if (folder_invoker_ == nullptr) {
        folder_invoker_ = std::make_unique<ProductionSelectFolderInvoker>();
    }
    state_->queue = events_;
}

GtkFileDialogService::~GtkFileDialogService() { shutdown(); }

bool GtkFileDialogService::pending() const noexcept { return state_ != nullptr && state_->pending; }

void GtkFileDialogService::shutdown() noexcept {
    if (state_ == nullptr || !state_->active) return;
    // Invalidate before cancel/hide: GTK may synchronously deliver a response while native state is
    // being retired. The callback then sees an inert weak state and cannot touch the queue/window.
    state_->active = false;
    state_->pending = false;
    state_->completed = true;
    state_->queue = nullptr;
    ++state_->generation;
    if (open_invoker_ != nullptr) open_invoker_->cancel();
    if (open_files_invoker_ != nullptr) open_files_invoker_->cancel();
    if (save_invoker_ != nullptr) save_invoker_->cancel();
    if (folder_invoker_ != nullptr) folder_invoker_->cancel();
    events_ = nullptr;
    owner_ = nullptr;
}

FileDialogRequestId GtkFileDialogService::requestOpenFile(OpenFileDialogRequest request) {
    if (events_ == nullptr || owner_ == nullptr || state_ == nullptr || !state_->active) {
        throw std::logic_error{"GTK file dialog requires initialize()"};
    }
    if (std::this_thread::get_id() != owner_thread_) {
        throw std::runtime_error{"GTK file dialogs must start on their owner thread"};
    }
    const std::string title = strictTitle(request.title); // before ID allocation
    const std::vector<std::string> extensions =
        sasd::ui::native::detail::validateFileDialogExtensions(request.extensions);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    if (state_->pending) throw std::logic_error{"GTK file dialog already has a pending request"};
    if (next_request_id_ == 0) {
        throw std::overflow_error{"GTK file-dialog request ID space exhausted"};
    }

    const FileDialogRequestId id = next_request_id_++;
    const std::uint64_t generation = ++state_->generation;
    state_->request_id = id;
    state_->pending = true;
    state_->completed = false;
    const std::weak_ptr<CompletionState> weak_state = state_;
    const NativeOpenFileCompletion completion = [weak_state, id, generation](
                                                    FileDialogResultStatus native_status,
                                                    GFile* file) noexcept {
        try {
            const std::shared_ptr<CompletionState> state = weak_state.lock();
            if (state == nullptr || !state->active || !state->pending || state->completed ||
                state->request_id != id || state->generation != generation || state->queue == nullptr) {
                return;
            }
            FileDialogResultStatus status = FileDialogResultStatus::failed;
            std::optional<std::filesystem::path> path;
            if (native_status == FileDialogResultStatus::cancelled) {
                status = FileDialogResultStatus::cancelled;
            } else if (native_status == FileDialogResultStatus::accepted) {
                path = localPath(file);
                if (path && !path->empty()) status = FileDialogResultStatus::accepted;
            }
            state->completed = true;
            state->queue->push(Event{FileDialogResultEvent{id, status, std::move(path)}});
            // Pending ends at successful enqueue, not when Application later consumes the event.
            state->pending = false;
        } catch (...) {
            // Native callback boundaries are noexcept. A malformed/native result is still a
            // post-ID technical failure, so make the required single failed completion when the
            // queue is usable. If queue allocation itself fails there is no transport left that can
            // represent the result; either way the generation is retired and cannot be reused.
            if (const std::shared_ptr<CompletionState> state = weak_state.lock()) {
                if (state->active && state->pending && !state->completed && state->queue != nullptr) {
                    try {
                        state->completed = true;
                        state->queue->push(Event{FileDialogResultEvent{
                            id, FileDialogResultStatus::failed}});
                    } catch (...) {
                    }
                }
                state->pending = false;
            }
        }
    };

    try {
        open_invoker_->begin(owner_, title, extensions, initial_directory, completion);
    } catch (...) {
        completion(FileDialogResultStatus::failed, nullptr);
    }
    return id;
}

FileDialogRequestId GtkFileDialogService::requestOpenFiles(OpenFilesDialogRequest request) {
    if (events_ == nullptr || owner_ == nullptr || state_ == nullptr || !state_->active) {
        throw std::logic_error{"GTK file dialog requires initialize()"};
    }
    if (std::this_thread::get_id() != owner_thread_) {
        throw std::runtime_error{"GTK file dialogs must start on their owner thread"};
    }
    const std::string title = strictTitle(request.title);
    const std::vector<std::string> extensions =
        sasd::ui::native::detail::validateFileDialogExtensions(request.extensions);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    if (state_->pending) throw std::logic_error{"GTK file dialog already has a pending request"};
    if (next_request_id_ == 0) {
        throw std::overflow_error{"GTK file-dialog request ID space exhausted"};
    }

    const FileDialogRequestId id = next_request_id_++;
    const std::uint64_t generation = ++state_->generation;
    state_->request_id = id;
    state_->pending = true;
    state_->completed = false;
    const std::weak_ptr<CompletionState> weak_state = state_;
    const NativeOpenFilesCompletion completion = [weak_state, id, generation](
        FileDialogResultStatus native_status, NativeOpenFilesSelection selection) noexcept {
        try {
            const std::shared_ptr<CompletionState> state = weak_state.lock();
            if (state == nullptr || !state->active || !state->pending || state->completed ||
                state->request_id != id || state->generation != generation || state->queue == nullptr) return;
            FileDialogResultStatus status = FileDialogResultStatus::failed;
            std::vector<std::filesystem::path> paths;
            if (native_status == FileDialogResultStatus::cancelled) {
                status = FileDialogResultStatus::cancelled;
            } else if (native_status == FileDialogResultStatus::accepted) {
                bool valid = false;
                if (selection.model != nullptr) {
                    const guint count = g_list_model_get_n_items(selection.model);
                    valid = count != 0;
                    paths.reserve(static_cast<std::size_t>(count));
                    for (guint index = 0; valid && index < count; ++index) {
                        GObject* object = static_cast<GObject*>(
                            g_list_model_get_item(selection.model, index));
                        if (object == nullptr || !G_IS_FILE(object)) {
                            if (object != nullptr) g_object_unref(object);
                            valid = false;
                            break;
                        }
                        const auto path = localPath(G_FILE(object));
                        g_object_unref(object);
                        if (!path || path->empty()) {
                            valid = false;
                            break;
                        }
                        paths.push_back(*path);
                    }
                } else if (selection.files != nullptr) {
                    valid = true;
                    for (GSList* node = selection.files; node != nullptr; node = node->next) {
                        const auto path = localPath(G_FILE(node->data));
                        if (!path || path->empty()) {
                            valid = false;
                            break;
                        }
                        paths.push_back(*path);
                    }
                    valid = valid && !paths.empty();
                }
                if (valid) status = FileDialogResultStatus::accepted;
                else paths.clear();
            }
            state->completed = true;
            state->queue->push(Event{OpenFilesDialogResultEvent{id, status, std::move(paths)}});
            state->pending = false;
        } catch (...) {
            if (const std::shared_ptr<CompletionState> state = weak_state.lock()) {
                if (state->active && state->pending && !state->completed && state->queue != nullptr) {
                    try {
                        state->completed = true;
                        state->queue->push(Event{OpenFilesDialogResultEvent{
                            id, FileDialogResultStatus::failed}});
                    } catch (...) {}
                }
                state->pending = false;
            }
        }
    };
    try {
        open_files_invoker_->begin(owner_, title, extensions, initial_directory, completion);
    } catch (...) {
        completion(FileDialogResultStatus::failed, {});
    }
    return id;
}

FileDialogRequestId GtkFileDialogService::requestSaveFile(SaveFileDialogRequest request) {
    if (events_ == nullptr || owner_ == nullptr || state_ == nullptr || !state_->active) {
        throw std::logic_error{"GTK file dialog requires initialize()"};
    }
    if (std::this_thread::get_id() != owner_thread_) {
        throw std::runtime_error{"GTK file dialogs must start on their owner thread"};
    }
    const std::string title = strictTitle(request.title);
    const std::string suggested_filename = strictSuggestedFilename(request.suggested_filename);
    const std::vector<std::string> extensions =
        sasd::ui::native::detail::validateFileDialogExtensions(request.extensions);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    if (state_->pending) throw std::logic_error{"GTK file dialog already has a pending request"};
    if (next_request_id_ == 0) {
        throw std::overflow_error{"GTK file-dialog request ID space exhausted"};
    }

    const FileDialogRequestId id = next_request_id_++;
    const std::uint64_t generation = ++state_->generation;
    state_->request_id = id;
    state_->pending = true;
    state_->completed = false;
    const std::weak_ptr<CompletionState> weak_state = state_;
    const NativeSaveFileCompletion completion = [weak_state, id, generation](
                                                    FileDialogResultStatus native_status,
                                                    GFile* file) noexcept {
        try {
            const std::shared_ptr<CompletionState> state = weak_state.lock();
            if (state == nullptr || !state->active || !state->pending || state->completed ||
                state->request_id != id || state->generation != generation || state->queue == nullptr) {
                return;
            }
            FileDialogResultStatus status = FileDialogResultStatus::failed;
            std::optional<std::filesystem::path> path;
            if (native_status == FileDialogResultStatus::cancelled) {
                status = FileDialogResultStatus::cancelled;
            } else if (native_status == FileDialogResultStatus::accepted) {
                path = localPath(file);
                if (path && !path->empty()) status = FileDialogResultStatus::accepted;
            }
            state->completed = true;
            state->queue->push(Event{FileDialogResultEvent{id, status, std::move(path)}});
            state->pending = false;
        } catch (...) {
            if (const std::shared_ptr<CompletionState> state = weak_state.lock()) {
                if (state->active && state->pending && !state->completed && state->queue != nullptr) {
                    try {
                        state->completed = true;
                        state->queue->push(Event{FileDialogResultEvent{
                            id, FileDialogResultStatus::failed}});
                    } catch (...) {
                    }
                }
                state->pending = false;
            }
        }
    };

    try {
        save_invoker_->begin(owner_, title, extensions, suggested_filename, initial_directory, completion);
    } catch (...) {
        completion(FileDialogResultStatus::failed, nullptr);
    }
    return id;
}

FileDialogRequestId GtkFileDialogService::requestSelectFolder(SelectFolderDialogRequest request) {
    if (events_ == nullptr || owner_ == nullptr || state_ == nullptr || !state_->active) {
        throw std::logic_error{"GTK file dialog requires initialize()"};
    }
    if (std::this_thread::get_id() != owner_thread_) {
        throw std::runtime_error{"GTK file dialogs must start on their owner thread"};
    }
    const std::string title = strictTitle(request.title);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    if (state_->pending) throw std::logic_error{"GTK file dialog already has a pending request"};
    if (next_request_id_ == 0) {
        throw std::overflow_error{"GTK file-dialog request ID space exhausted"};
    }

    const FileDialogRequestId id = next_request_id_++;
    const std::uint64_t generation = ++state_->generation;
    state_->request_id = id;
    state_->pending = true;
    state_->completed = false;
    const std::weak_ptr<CompletionState> weak_state = state_;
    const NativeSelectFolderCompletion completion = [weak_state, id, generation](
                                                        FileDialogResultStatus native_status,
                                                        GFile* file) noexcept {
        try {
            const std::shared_ptr<CompletionState> state = weak_state.lock();
            if (state == nullptr || !state->active || !state->pending || state->completed ||
                state->request_id != id || state->generation != generation || state->queue == nullptr) {
                return;
            }
            FileDialogResultStatus status = FileDialogResultStatus::failed;
            std::optional<std::filesystem::path> path;
            if (native_status == FileDialogResultStatus::cancelled) {
                status = FileDialogResultStatus::cancelled;
            } else if (native_status == FileDialogResultStatus::accepted) {
                path = localPath(file);
                if (path && !path->empty()) status = FileDialogResultStatus::accepted;
            }
            state->completed = true;
            state->queue->push(Event{FileDialogResultEvent{id, status, std::move(path)}});
            state->pending = false;
        } catch (...) {
            if (const std::shared_ptr<CompletionState> state = weak_state.lock()) {
                if (state->active && state->pending && !state->completed && state->queue != nullptr) {
                    try {
                        state->completed = true;
                        state->queue->push(Event{FileDialogResultEvent{
                            id, FileDialogResultStatus::failed}});
                    } catch (...) {}
                }
                state->pending = false;
            }
        }
    };

    try {
        folder_invoker_->begin(owner_, title, initial_directory, completion);
    } catch (...) {
        completion(FileDialogResultStatus::failed, nullptr);
    }
    return id;
}

} // namespace sasd::ui::native::gtk::detail
