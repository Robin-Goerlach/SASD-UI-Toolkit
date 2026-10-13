#pragma once

#include <sasd/ui/events/event.hpp>

#include <cstdint>
#include <optional>

namespace sasd::ui::native::appkit::detail {

// These helpers intentionally use plain values so translation can be tested without constructing
// NSEvent objects. Values are the stable AppKit virtual-key codes and modifier-bit values consumed by
// the private Objective-C++ adapter.
[[nodiscard]] std::optional<Key> translateKey(std::uint16_t key_code) noexcept;
[[nodiscard]] std::optional<Key> translateLogicalLetter(std::uint32_t scalar) noexcept;
[[nodiscard]] KeyModifier translateModifiers(std::uint64_t flags) noexcept;
[[nodiscard]] std::optional<Point> translatePoint(double x, double y) noexcept;
[[nodiscard]] std::optional<PointerEvent> translatePointerButton(
    std::uint16_t button, bool pressed, double x, double y, std::uint64_t flags) noexcept;

} // namespace sasd::ui::native::appkit::detail
