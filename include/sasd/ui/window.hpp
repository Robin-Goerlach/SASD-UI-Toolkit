#pragma once

#include <sasd/ui/container.hpp>

namespace sasd::ui {

/**
 * Semantic top-level application window/screen container.
 *
 * Window intentionally starts as a very small semantic type. It contains no native handle, terminal
 * device, title/chrome policy or renderer state. Those concerns would prematurely couple the public
 * API to one presentation strategy. M2 initially uses Window as the root of a terminal presentation
 * pass and will add window properties only when at least one real backend can validate their meaning.
 */
class Window : public Container {
public:
    Window() = default;
    ~Window() override = default;
};

} // namespace sasd::ui
