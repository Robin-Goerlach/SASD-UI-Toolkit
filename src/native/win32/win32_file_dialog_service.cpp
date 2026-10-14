#include "win32_file_dialog_service.hpp"

#include <shobjidl.h>

#include <limits>
#include <stdexcept>
#include <utility>

namespace sasd::ui::native::win32::detail {
namespace {

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
    [[nodiscard]] NativeOpenFileOutcome show(HWND owner, std::wstring_view title) override {
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

} // namespace

Win32FileDialogService::Win32FileDialogService(
    HWND owner,
    EventQueue& events,
    std::unique_ptr<NativeOpenFileInvoker> invoker)
    : owner_{owner}, events_{events}, invoker_{std::move(invoker)} {
    if (invoker_ == nullptr) invoker_ = std::make_unique<ProductionOpenFileInvoker>();
}

FileDialogRequestId Win32FileDialogService::requestOpenFile(OpenFileDialogRequest request) {
    // Validate text before allocating an ID: malformed input is a caller setup error and no
    // request has started yet, so it is reported synchronously rather than as a fake completion.
    const std::wstring title = utf8ToWide(request.title);
    if (pending_) throw std::logic_error{"Win32 file dialog already has a pending request"};
    if (next_request_id_ == 0) {
        throw std::overflow_error{"Win32 file-dialog request ID space exhausted"};
    }

    const FileDialogRequestId request_id = next_request_id_++;
    pending_ = true;
    try {
        const NativeOpenFileOutcome outcome = invoker_->show(owner_, title);
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

} // namespace sasd::ui::native::win32::detail
