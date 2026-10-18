#pragma once

#include <sasd/ui/text/utf8.hpp>

#include <stdexcept>
#include <string>
#include <string_view>

namespace sasd::ui::native::detail {

/** Validates the small Save filename-component hint contract before request-ID allocation. */
[[nodiscard]] inline std::string validateSuggestedFilename(std::string_view filename) {
    if (filename.empty()) return {};
    if (filename.find('\0') != std::string_view::npos) {
        throw std::invalid_argument{"suggested filename must not contain NUL"};
    }
    if (filename.find_first_of("/\\") != std::string_view::npos) {
        throw std::invalid_argument{"suggested filename must be one filename component"};
    }
    if (filename == "." || filename == "..") {
        throw std::invalid_argument{"suggested filename must not be a directory component"};
    }
    for (std::size_t offset = 0; offset < filename.size();) {
        const utf8::DecodedScalar decoded = utf8::decodeOne(filename, offset);
        if (decoded.consumed == 0 || !decoded.valid) {
            throw std::invalid_argument{"suggested filename must be valid UTF-8"};
        }
        offset += decoded.consumed;
    }
    return std::string{filename};
}

} // namespace sasd::ui::native::detail
