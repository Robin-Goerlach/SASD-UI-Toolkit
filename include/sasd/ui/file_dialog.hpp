#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace sasd::ui {

using FileDialogRequestId = std::uint64_t;

/** The deliberately small open-file request: title only, with no portable filter policy. */
struct OpenFileDialogRequest {
    std::string title{};
};

/** The deliberately small save-file request: title only, with no portable option policy. */
struct SaveFileDialogRequest {
    std::string title{};
};

/** The deliberately small folder-selection request: title only, with no portable option policy. */
struct SelectFolderDialogRequest {
    std::string title{};
};

enum class FileDialogResultStatus : std::uint8_t {
    accepted,
    cancelled,
    failed,
};

/**
 * Application-level completion of one FileDialogService request.
 *
 * The value is intentionally an owned filesystem path rather than UI text. An accepted result has
 * exactly one non-empty path; cancelled and failed results never carry a path. Constructing an
 * invalid combination throws std::logic_error so backends cannot accidentally use an empty path as
 * a cancellation sentinel. Request ID zero is reserved as invalid, and every real request uses a
 * non-zero ID.
 */
struct FileDialogResultEvent {
    FileDialogRequestId request_id;
    FileDialogResultStatus status;
    std::optional<std::filesystem::path> path;

    FileDialogResultEvent(FileDialogRequestId request_id,
                          FileDialogResultStatus status,
                          std::optional<std::filesystem::path> path = {});
};

/**
 * Optional host service for choosing one filesystem file for opening or saving, or one existing
 * local filesystem directory.
 *
 * requestOpenFile() starts a request and returns its non-zero identity; it never returns the chosen
 * path. requestSaveFile() uses the same contract for a destination path; an accepted save path may
 * already exist or may not exist yet. The toolkit never creates, opens, truncates or writes that
 * path. Folder selection never creates a directory or changes the process working directory.
 * Completion is always delivered later through FileDialogResultEvent on the backend event queue,
 * even when the native platform operation uses a synchronous modal loop. A backend may enter that
 * loop during this call. Open, save and folder selection share one request-ID sequence and one
 * pending boundary: version 1 permits at most one pending request of any kind. Pending lasts from
 * successful
 * request-ID allocation until exactly one completion event has been queued, not until application
 * code consumes that event. A second request while the first is pending fails closed with
 * std::logic_error. Native dialogs own their normal overwrite confirmation; the service does not
 * perform a post-dialog existence check or confirmation. There are no application callbacks.
 *
 * The service does not invoke application callbacks. The returned pointer from Backend is non-owning
 * and valid only while its backend is initialized.
 */
class FileDialogService {
public:
    virtual ~FileDialogService() = default;

    FileDialogService(const FileDialogService&) = delete;
    FileDialogService& operator=(const FileDialogService&) = delete;
    FileDialogService(FileDialogService&&) = delete;
    FileDialogService& operator=(FileDialogService&&) = delete;

    [[nodiscard]] virtual FileDialogRequestId requestOpenFile(OpenFileDialogRequest request) = 0;
    [[nodiscard]] virtual FileDialogRequestId requestSaveFile(SaveFileDialogRequest request) = 0;
    [[nodiscard]] virtual FileDialogRequestId requestSelectFolder(
        SelectFolderDialogRequest request) = 0;

protected:
    FileDialogService() = default;
};

} // namespace sasd::ui
