#pragma once

#include <sasd/ui/component.hpp>

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

} // namespace sasd::ui
