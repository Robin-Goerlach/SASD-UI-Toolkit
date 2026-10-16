#pragma once

#include <sasd/ui/text/utf8.hpp>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sasd::ui::native::detail {

/**
 * Validates the deliberately small portable Open File extension contract.
 *
 * The returned copy is stable backend input. Validation is intentionally shared by all native
 * consumers so malformed UTF-8, path/glob syntax and duplicate policy cannot diverge by platform.
 */
[[nodiscard]] inline std::vector<std::string> validateOpenFileExtensions(
    const std::vector<std::string>& extensions) {
    std::vector<std::string> validated;
    validated.reserve(extensions.size());
    for (const std::string& extension : extensions) {
        if (extension.empty()) {
            throw std::invalid_argument{"file-dialog extension must not be empty"};
        }
        if (extension.find('\0') != std::string::npos ||
            extension.find_first_of(".*?/\\;") != std::string::npos) {
            throw std::invalid_argument{
                "file-dialog extension must be one literal token without dot, glob, path or semicolon"};
        }
        for (std::size_t offset = 0; offset < extension.size();) {
            const utf8::DecodedScalar decoded = utf8::decodeOne(extension, offset);
            if (decoded.consumed == 0 || !decoded.valid) {
                throw std::invalid_argument{"file-dialog extension must be valid UTF-8"};
            }
            offset += decoded.consumed;
        }
        for (const std::string& previous : validated) {
            if (previous == extension) {
                throw std::invalid_argument{"file-dialog extensions must not contain duplicates"};
            }
        }
        validated.push_back(extension);
    }
    return validated;
}

} // namespace sasd::ui::native::detail
