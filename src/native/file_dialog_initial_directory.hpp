#pragma once

#include <sasd/ui/file_dialog.hpp>

#include <stdexcept>

namespace sasd::ui::native::detail {

/**
 * Validates the small initial-directory hint contract before request-ID allocation.
 *
 * The path remains in its native filesystem representation. In particular, this helper does not
 * impose a UTF-8 rule, query the filesystem, normalize components, or turn a relative path into an
 * absolute one. A relative hint would otherwise silently depend on the process working directory.
 */
[[nodiscard]] inline std::optional<std::filesystem::path> validateInitialDirectory(
    const std::optional<std::filesystem::path>& directory) {
    if (!directory.has_value()) return std::nullopt;
    if (directory->empty()) {
        throw std::invalid_argument{"initial directory must not be empty"};
    }
    if (!directory->is_absolute()) {
        throw std::invalid_argument{"initial directory must be absolute"};
    }
    const auto native = directory->native();
    if (native.find(std::filesystem::path::value_type{}) != decltype(native)::npos) {
        throw std::invalid_argument{"initial directory must not contain NUL"};
    }
    return directory;
}

} // namespace sasd::ui::native::detail
