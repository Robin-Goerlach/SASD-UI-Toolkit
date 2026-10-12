#pragma once

#include <sasd/ui/events/event.hpp>

#include <windows.h>

#include <optional>
#include <string_view>

namespace sasd::ui::native::win32::detail {

[[nodiscard]] std::optional<Event> translateWindowMessage(UINT message,
                                                           WPARAM native_width,
                                                           LPARAM native_size) noexcept;

[[nodiscard]] KeyModifier translateModifiers(WPARAM native_modifiers,
                                              KeyModifier additional = KeyModifier::none) noexcept;

[[nodiscard]] std::optional<KeyEvent> translateKeyMessage(UINT message,
                                                           WPARAM virtual_key,
                                                           KeyModifier modifiers) noexcept;

/** Converts one complete UTF-16 payload; malformed/unpaired input fails closed. */
[[nodiscard]] std::optional<TextInputEvent> translateTextInput(std::wstring_view utf16) noexcept;

/** Converts client coordinates from LPARAM without signed 16-bit truncation errors. */
[[nodiscard]] std::optional<Point> translateClientPoint(LPARAM native_point) noexcept;

[[nodiscard]] std::optional<PointerEvent> translatePointerMessage(
    UINT message,
    WPARAM native_state,
    LPARAM native_point,
    KeyModifier additional_modifiers = KeyModifier::none) noexcept;

[[nodiscard]] std::optional<PointerSurfaceEvent> translatePointerSurfaceMessage(
    UINT message) noexcept;

} // namespace sasd::ui::native::win32::detail
