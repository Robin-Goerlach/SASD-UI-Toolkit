#pragma once

#include <sasd/ui/events/event.hpp>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <optional>
#include <string_view>

namespace sasd::ui::native::gtk::detail {
[[nodiscard]] std::optional<Key> translateKey(guint keyval) noexcept;
[[nodiscard]] KeyModifier translateModifiers(GdkModifierType state) noexcept;
[[nodiscard]] std::optional<PointerEvent> translatePointerButton(guint button, bool pressed,
                                                                  double x, double y,
                                                                  GdkModifierType state) noexcept;
[[nodiscard]] std::optional<Point> translatePoint(double x, double y) noexcept;
[[nodiscard]] std::optional<std::string> committedText(guint keyval) noexcept;
} // namespace sasd::ui::native::gtk::detail
