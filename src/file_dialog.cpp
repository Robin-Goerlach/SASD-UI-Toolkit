#include <sasd/ui/file_dialog.hpp>

#include <stdexcept>
#include <utility>

namespace sasd::ui {

FileDialogResultEvent::FileDialogResultEvent(
    FileDialogRequestId value,
    FileDialogResultStatus result,
    std::optional<std::filesystem::path> selected_path)
    : request_id{value}, status{result}, path{std::move(selected_path)} {
    if (request_id == 0) {
        throw std::logic_error{"FileDialogResultEvent requires a non-zero request ID"};
    }
    const bool accepted_with_path = status == FileDialogResultStatus::accepted &&
                                    path.has_value() && !path->empty();
    const bool no_path_result = (status == FileDialogResultStatus::cancelled ||
                                 status == FileDialogResultStatus::failed) &&
                                !path.has_value();
    if (!accepted_with_path && !no_path_result) {
        throw std::logic_error{"FileDialogResultEvent has an invalid status/path combination"};
    }
}

} // namespace sasd::ui
