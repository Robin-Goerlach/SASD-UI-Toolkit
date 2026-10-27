#pragma once

#include <sasd/ui/component.hpp>
#include <sasd/ui/ui_description_document.hpp>

#include <cstddef>
#include <string_view>

#include <optional>
#include <string>

namespace sasd::ui {

/**
 * Captures an unowned exact built-in tree and encodes canonical UI Description JSON v1.
 *
 * JSON is only an encoding above the format-neutral reference graph snapshot. All persistent
 * values (including defaults) are explicit; capture metadata, model payloads, callbacks and
 * transient/derived state are excluded. Output uses two-space indentation, LF and one final LF.
 * Capture failure, invalid UTF-8 or an unmapped enum/inconsistent encoding invariant returns
 * nullopt, never partial output. Allocation failures propagate. No file I/O is performed.
 * Recommended file extension: .sasd-ui.json. See docs/en/UI_DESCRIPTION_FORMAT.md.
 */
[[nodiscard]] std::optional<std::string> encodeBuiltInUiDescriptionJson(const Component& root);

enum class UiDescriptionJsonParseErrorCode {
    none,
    invalid_utf8,
    syntax_error,
    duplicate_member,
    unknown_member,
    missing_member,
    wrong_value_type,
    unsupported_format,
    unsupported_format_version,
    unknown_component_type,
    unknown_property,
    property_kind_mismatch,
    invalid_property_value,
    invalid_ownership,
    invalid_reference,
};

struct UiDescriptionJsonParseError final {
    UiDescriptionJsonParseErrorCode code{UiDescriptionJsonParseErrorCode::none};
    std::size_t byte_offset{0};
};

struct BuiltInUiDescriptionJsonParseResult final {
    std::optional<BuiltInUiDescriptionDocument> document;
    UiDescriptionJsonParseError error{};

    [[nodiscard]] explicit operator bool() const noexcept { return document.has_value(); }
};

/**
 * Parses strict UTF-8 JSON v1 into an owning, format-neutral validated document, without
 * constructing Components. Object/member and array presentation order may differ from the
 * canonical writer; unknown/duplicate members and incomplete property schemas fail closed.
 * Success has document and error.code == none. Failure has no document and a non-none code.
 * byte_offset is a zero-based input byte position at/near detection, including size() at EOF.
 * No default completion, file I/O or runtime restoration occurs. Allocation failures propagate.
 */
[[nodiscard]] BuiltInUiDescriptionJsonParseResult parseBuiltInUiDescriptionJson(std::string_view json);

} // namespace sasd::ui
