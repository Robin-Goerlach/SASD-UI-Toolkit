#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui {

using FileDialogRequestId = std::uint64_t;

/**
 * Open one existing local file. Each extension is one literal UTF-8 token such as `txt` or
 * `cpp`; it is not a glob, path, MIME type or security validation. An empty list leaves the native
 * picker unfiltered. initial_directory is an optional absolute native filesystem path used only as
 * a start-location preference; it is not a sandbox or result-containment rule. The native picker
 * remains authoritative for the accepted result.
 */
struct OpenFileDialogRequest {
    std::string title{};
    std::vector<std::string> extensions{};
    /** Optional absolute native directory preference for the initial dialog location. */
    std::optional<std::filesystem::path> initial_directory{};
};

/**
 * Open one or more existing local files. Its options intentionally mirror OpenFileDialogRequest,
 * while the distinct type keeps selection cardinality explicit in the public contract.
 */
struct OpenFilesDialogRequest {
    std::string title{};
    std::vector<std::string> extensions{};
    /** Optional absolute native directory preference for the initial dialog location. */
    std::optional<std::filesystem::path> initial_directory{};
};

/**
 * Save one destination path. Extensions use the same literal UTF-8 token contract as
 * OpenFileDialogRequest. An empty list leaves the native picker unfiltered; otherwise the
 * ordered list is one native filter set and its first token is the preferred native extension
 * where the platform provides that naming policy. The optional suggested_filename is an editable
 * initial filename component, not a path or result requirement; an empty value means no suggestion.
 * The accepted native path remains authoritative. initial_directory is an optional absolute native
 * filesystem path preference, independent of the suggested filename and without result restriction.
 */
struct SaveFileDialogRequest {
    std::string title{};
    std::vector<std::string> extensions{};
    std::string suggested_filename{};
    /** Optional absolute native directory preference, independent of suggested_filename. */
    std::optional<std::filesystem::path> initial_directory{};
};

/** Select one existing directory, optionally preferring an absolute initial native directory. */
struct SelectFolderDialogRequest {
    std::string title{};
    /** Optional absolute native directory preference for the initial folder location. */
    std::optional<std::filesystem::path> initial_directory{};
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
 * Application-level completion of a multiple-file open request.
 *
 * Accepted results contain one or more non-empty local filesystem paths in native selection order.
 * Cancellation and failure contain no paths. The native result is intentionally not sorted,
 * deduplicated, normalized or otherwise rewritten.
 */
struct OpenFilesDialogResultEvent {
    FileDialogRequestId request_id;
    FileDialogResultStatus status;
    std::vector<std::filesystem::path> paths;

    OpenFilesDialogResultEvent(FileDialogRequestId request_id,
                               FileDialogResultStatus status,
                               std::vector<std::filesystem::path> paths = {});
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
    [[nodiscard]] virtual FileDialogRequestId requestOpenFiles(OpenFilesDialogRequest request) = 0;
    [[nodiscard]] virtual FileDialogRequestId requestSaveFile(SaveFileDialogRequest request) = 0;
    [[nodiscard]] virtual FileDialogRequestId requestSelectFolder(
        SelectFolderDialogRequest request) = 0;

protected:
    FileDialogService() = default;
};

} // namespace sasd::ui
