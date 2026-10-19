#include "win32_file_dialog_service.hpp"

#include "../file_dialog_extension_filter.hpp"
#include "../file_dialog_initial_directory.hpp"
#include "../file_dialog_suggested_filename.hpp"

#include <shobjidl.h>

#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sasd::ui::native::win32::detail {
namespace {

[[nodiscard]] HRESULT setDefaultFolder(
    IFileDialog* dialog, const std::optional<std::filesystem::path>& initial_directory) {
    if (!initial_directory.has_value()) return S_OK;

    IShellItem* folder = nullptr;
    const HRESULT item_result = SHCreateItemFromParsingName(
        initial_directory->c_str(), nullptr, IID_PPV_ARGS(&folder));
    if (FAILED(item_result)) return item_result;
    struct ItemRelease final {
        IShellItem* value;
        ~ItemRelease() { value->Release(); }
    } release{folder};
    return dialog->SetDefaultFolder(folder);
}

[[nodiscard]] std::wstring utf8ToWide(std::string_view value) {
    if (value.empty()) return {};
    if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error{"Win32 file-dialog title is too long"};
    }
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0);
    if (length <= 0) throw std::invalid_argument{"Win32 file-dialog title must be valid UTF-8"};
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                            static_cast<int>(value.size()), result.data(), length) != length) {
        throw std::invalid_argument{"Win32 file-dialog title must be valid UTF-8"};
    }
    return result;
}

class ProductionOpenFileInvoker final : public NativeOpenFileInvoker {
public:
    [[nodiscard]] NativeOpenFileOutcome show(
        HWND owner, std::wstring_view title,
        const std::vector<std::wstring>& extensions,
        const std::optional<std::filesystem::path>& initial_directory) override {
        HRESULT initialization = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(initialization)) return {FileDialogResultStatus::failed, {}};

        struct ComApartment final {
            ~ComApartment() { CoUninitialize(); }
        } apartment;

        IFileOpenDialog* dialog = nullptr;
        HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_PPV_ARGS(&dialog));
        if (FAILED(result)) return {FileDialogResultStatus::failed, {}};

        struct DialogRelease final {
            IFileOpenDialog* value;
            ~DialogRelease() { value->Release(); }
        } release{dialog};

        DWORD options = 0;
        if (FAILED(dialog->GetOptions(&options))) return {FileDialogResultStatus::failed, {}};
        options |= FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
        if (FAILED(dialog->SetOptions(options))) return {FileDialogResultStatus::failed, {}};
        if (!title.empty() && FAILED(dialog->SetTitle(std::wstring{title}.c_str()))) {
            return {FileDialogResultStatus::failed, {}};
        }

        if (!extensions.empty()) {
            // COMDLG_FILTERSPEC stores pointers rather than owning strings. Keep both local backing
            // strings alive until SetFileTypes and Show have returned.
            const std::wstring pattern = win32FilterPattern(extensions);
            const std::wstring display_name = pattern;
            const COMDLG_FILTERSPEC filter{display_name.c_str(), pattern.c_str()};
            if (FAILED(dialog->SetFileTypes(1, &filter))) {
                return {FileDialogResultStatus::failed, {}};
            }
        }

        if (FAILED(setDefaultFolder(dialog, initial_directory))) {
            return {FileDialogResultStatus::failed, {}};
        }

        result = dialog->Show(owner);
        if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            return {FileDialogResultStatus::cancelled, {}};
        }
        if (FAILED(result)) return {FileDialogResultStatus::failed, {}};

        IShellItem* item = nullptr;
        if (FAILED(dialog->GetResult(&item))) return {FileDialogResultStatus::failed, {}};
        struct ItemRelease final {
            IShellItem* value;
            ~ItemRelease() { value->Release(); }
        } item_release{item};

        PWSTR native_path = nullptr;
        if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &native_path))) {
            return {FileDialogResultStatus::failed, {}};
        }
        std::wstring path{native_path};
        CoTaskMemFree(native_path);
        return {FileDialogResultStatus::accepted, std::move(path)};
    }
};

class ProductionSaveFileInvoker final : public NativeSaveFileInvoker {
public:
    [[nodiscard]] NativeSaveFileOutcome show(
        HWND owner, std::wstring_view title,
        const std::vector<std::wstring>& extensions,
        std::wstring_view suggested_filename,
        const std::optional<std::filesystem::path>& initial_directory) override {
        HRESULT initialization = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(initialization)) return {FileDialogResultStatus::failed, {}};

        struct ComApartment final {
            ~ComApartment() { CoUninitialize(); }
        } apartment;

        IFileSaveDialog* dialog = nullptr;
        HRESULT result = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_PPV_ARGS(&dialog));
        if (FAILED(result)) return {FileDialogResultStatus::failed, {}};

        struct DialogRelease final {
            IFileSaveDialog* value;
            ~DialogRelease() { value->Release(); }
        } release{dialog};

        DWORD options = 0;
        if (FAILED(dialog->GetOptions(&options))) return {FileDialogResultStatus::failed, {}};
        // FOS_FILEMUSTEXIST is intentionally absent: a save target may be new. PATHMUSTEXIST
        // validates only the navigated parent, while the shell owns overwrite confirmation.
        options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR | FOS_OVERWRITEPROMPT;
        if (FAILED(dialog->SetOptions(options))) return {FileDialogResultStatus::failed, {}};
        if (!title.empty() && FAILED(dialog->SetTitle(std::wstring{title}.c_str()))) {
            return {FileDialogResultStatus::failed, {}};
        }

        if (!extensions.empty()) {
            // COMDLG_FILTERSPEC and SetDefaultExtension retain pointers into caller-owned
            // strings. Keep the pattern, display name and first extension alive through SetFileTypes
            // and Show; none of these pointers may refer to temporaries.
            const std::wstring pattern = win32FilterPattern(extensions);
            const std::wstring display_name = pattern;
            const COMDLG_FILTERSPEC filter{display_name.c_str(), pattern.c_str()};
            if (FAILED(dialog->SetFileTypes(1, &filter))) {
                return {FileDialogResultStatus::failed, {}};
            }
            if (FAILED(dialog->SetDefaultExtension(extensions.front().c_str()))) {
                return {FileDialogResultStatus::failed, {}};
            }
        }
        if (!suggested_filename.empty() &&
            FAILED(dialog->SetFileName(std::wstring{suggested_filename}.c_str()))) {
            return {FileDialogResultStatus::failed, {}};
        }
        if (FAILED(setDefaultFolder(dialog, initial_directory))) {
            return {FileDialogResultStatus::failed, {}};
        }

        result = dialog->Show(owner);
        if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            return {FileDialogResultStatus::cancelled, {}};
        }
        if (FAILED(result)) return {FileDialogResultStatus::failed, {}};

        IShellItem* item = nullptr;
        if (FAILED(dialog->GetResult(&item))) return {FileDialogResultStatus::failed, {}};
        struct ItemRelease final {
            IShellItem* value;
            ~ItemRelease() { value->Release(); }
        } item_release{item};

        PWSTR native_path = nullptr;
        if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &native_path))) {
            return {FileDialogResultStatus::failed, {}};
        }
        std::wstring path{native_path};
        CoTaskMemFree(native_path);
        return {FileDialogResultStatus::accepted, std::move(path)};
    }
};

class ProductionSelectFolderInvoker final : public NativeSelectFolderInvoker {
public:
    [[nodiscard]] NativeSelectFolderOutcome show(
        HWND owner, std::wstring_view title,
        const std::optional<std::filesystem::path>& initial_directory) override {
        HRESULT initialization = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(initialization)) return {FileDialogResultStatus::failed, {}};
        struct ComApartment final { ~ComApartment() { CoUninitialize(); } } apartment;

        IFileOpenDialog* dialog = nullptr;
        HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_PPV_ARGS(&dialog));
        if (FAILED(result)) return {FileDialogResultStatus::failed, {}};
        struct DialogRelease final {
            IFileOpenDialog* value;
            ~DialogRelease() { value->Release(); }
        } release{dialog};

        DWORD options = 0;
        if (FAILED(dialog->GetOptions(&options))) return {FileDialogResultStatus::failed, {}};
        // FOS_PICKFOLDERS makes the shell offer existing directories. FILEMUSTEXIST is deliberately
        // absent: it describes file selection and is not needed for the folder picker.
        options |= FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
        if (FAILED(dialog->SetOptions(options))) return {FileDialogResultStatus::failed, {}};
        if (!title.empty() && FAILED(dialog->SetTitle(std::wstring{title}.c_str()))) {
            return {FileDialogResultStatus::failed, {}};
        }
        if (FAILED(setDefaultFolder(dialog, initial_directory))) {
            return {FileDialogResultStatus::failed, {}};
        }

        result = dialog->Show(owner);
        if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            return {FileDialogResultStatus::cancelled, {}};
        }
        if (FAILED(result)) return {FileDialogResultStatus::failed, {}};

        IShellItem* item = nullptr;
        if (FAILED(dialog->GetResult(&item))) return {FileDialogResultStatus::failed, {}};
        struct ItemRelease final {
            IShellItem* value;
            ~ItemRelease() { value->Release(); }
        } item_release{item};
        PWSTR native_path = nullptr;
        if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &native_path))) {
            return {FileDialogResultStatus::failed, {}};
        }
        std::wstring path{native_path};
        CoTaskMemFree(native_path);
        return path.empty() ? NativeSelectFolderOutcome{FileDialogResultStatus::failed, {}}
                            : NativeSelectFolderOutcome{FileDialogResultStatus::accepted,
                                                        std::move(path)};
    }
};

} // namespace

Win32FileDialogService::Win32FileDialogService(
    HWND owner,
    EventQueue& events,
    std::unique_ptr<NativeOpenFileInvoker> open_invoker,
    std::unique_ptr<NativeSaveFileInvoker> save_invoker,
    std::unique_ptr<NativeSelectFolderInvoker> folder_invoker)
    : owner_{owner},
      events_{events},
      open_invoker_{std::move(open_invoker)},
      save_invoker_{std::move(save_invoker)},
      folder_invoker_{std::move(folder_invoker)} {
    if (open_invoker_ == nullptr) open_invoker_ = std::make_unique<ProductionOpenFileInvoker>();
    if (save_invoker_ == nullptr) save_invoker_ = std::make_unique<ProductionSaveFileInvoker>();
    if (folder_invoker_ == nullptr) {
        folder_invoker_ = std::make_unique<ProductionSelectFolderInvoker>();
    }
}

FileDialogRequestId Win32FileDialogService::requestOpenFile(OpenFileDialogRequest request) {
    // Validate text before allocating an ID: malformed input is a caller setup error and no
    // request has started yet, so it is reported synchronously rather than as a fake completion.
    const std::wstring title = utf8ToWide(request.title);
    const std::vector<std::string> extensions =
        sasd::ui::native::detail::validateFileDialogExtensions(request.extensions);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    std::vector<std::wstring> wide_extensions;
    wide_extensions.reserve(extensions.size());
    for (const std::string& extension : extensions) {
        wide_extensions.push_back(utf8ToWide(extension));
    }
    if (pending_) throw std::logic_error{"Win32 file dialog already has a pending request"};
    if (next_request_id_ == 0) {
        throw std::overflow_error{"Win32 file-dialog request ID space exhausted"};
    }

    const FileDialogRequestId request_id = next_request_id_++;
    pending_ = true;
    try {
        const NativeOpenFileOutcome outcome =
            open_invoker_->show(owner_, title, wide_extensions, initial_directory);
        if (outcome.status == FileDialogResultStatus::accepted) {
            if (outcome.path.empty()) throw std::runtime_error{"Win32 file dialog returned no path"};
            events_.push(Event{FileDialogResultEvent{
                request_id, FileDialogResultStatus::accepted, std::filesystem::path{outcome.path}}});
        } else if (outcome.status == FileDialogResultStatus::cancelled) {
            events_.push(Event{FileDialogResultEvent{request_id, FileDialogResultStatus::cancelled}});
        } else {
            events_.push(Event{FileDialogResultEvent{request_id, FileDialogResultStatus::failed}});
        }
    } catch (...) {
        // Once the request ID has been issued, every technical failure becomes exactly one failed
        // completion. No application callback runs here, and the exception never crosses native UI.
        events_.push(Event{FileDialogResultEvent{request_id, FileDialogResultStatus::failed}});
    }
    pending_ = false;
    return request_id;
}

FileDialogRequestId Win32FileDialogService::requestSaveFile(SaveFileDialogRequest request) {
    // Validate before allocating the shared ID. After allocation, even native setup failures are
    // represented by exactly one queued failed result, matching Open File.
    const std::wstring title = utf8ToWide(request.title);
    const std::string suggested_filename =
        sasd::ui::native::detail::validateSuggestedFilename(request.suggested_filename);
    const std::wstring wide_suggested_filename = utf8ToWide(suggested_filename);
    const std::vector<std::string> extensions =
        sasd::ui::native::detail::validateFileDialogExtensions(request.extensions);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    std::vector<std::wstring> wide_extensions;
    wide_extensions.reserve(extensions.size());
    for (const std::string& extension : extensions) {
        wide_extensions.push_back(utf8ToWide(extension));
    }
    if (pending_) throw std::logic_error{"Win32 file dialog already has a pending request"};
    if (next_request_id_ == 0) {
        throw std::overflow_error{"Win32 file-dialog request ID space exhausted"};
    }

    const FileDialogRequestId request_id = next_request_id_++;
    pending_ = true;
    try {
        const NativeSaveFileOutcome outcome = save_invoker_->show(
            owner_, title, wide_extensions, wide_suggested_filename, initial_directory);
        if (outcome.status == FileDialogResultStatus::accepted) {
            if (outcome.path.empty()) throw std::runtime_error{"Win32 file dialog returned no path"};
            // The returned path is the application's destination. Do not inspect or mutate the
            // filesystem: it is valid for a Save dialog to choose a not-yet-existing path.
            events_.push(Event{FileDialogResultEvent{
                request_id, FileDialogResultStatus::accepted, std::filesystem::path{outcome.path}}});
        } else if (outcome.status == FileDialogResultStatus::cancelled) {
            events_.push(Event{FileDialogResultEvent{request_id, FileDialogResultStatus::cancelled}});
        } else {
            events_.push(Event{FileDialogResultEvent{request_id, FileDialogResultStatus::failed}});
        }
    } catch (...) {
        events_.push(Event{FileDialogResultEvent{request_id, FileDialogResultStatus::failed}});
    }
    pending_ = false;
    return request_id;
}

FileDialogRequestId Win32FileDialogService::requestSelectFolder(SelectFolderDialogRequest request) {
    const std::wstring title = utf8ToWide(request.title);
    const std::optional<std::filesystem::path> initial_directory =
        sasd::ui::native::detail::validateInitialDirectory(request.initial_directory);
    if (pending_) throw std::logic_error{"Win32 file dialog already has a pending request"};
    if (next_request_id_ == 0) {
        throw std::overflow_error{"Win32 file-dialog request ID space exhausted"};
    }

    const FileDialogRequestId request_id = next_request_id_++;
    pending_ = true;
    try {
        const NativeSelectFolderOutcome outcome =
            folder_invoker_->show(owner_, title, initial_directory);
        if (outcome.status == FileDialogResultStatus::accepted && !outcome.path.empty()) {
            events_.push(Event{FileDialogResultEvent{
                request_id, FileDialogResultStatus::accepted, std::filesystem::path{outcome.path}}});
        } else if (outcome.status == FileDialogResultStatus::cancelled) {
            events_.push(Event{FileDialogResultEvent{request_id, FileDialogResultStatus::cancelled}});
        } else {
            events_.push(Event{FileDialogResultEvent{request_id, FileDialogResultStatus::failed}});
        }
    } catch (...) {
        events_.push(Event{FileDialogResultEvent{request_id, FileDialogResultStatus::failed}});
    }
    pending_ = false;
    return request_id;
}

} // namespace sasd::ui::native::win32::detail
