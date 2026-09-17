#pragma once

#include <sasd/ui/backend/capabilities.hpp>
#include <sasd/ui/events/event.hpp>

#include <optional>
#include <string_view>

namespace sasd::ui {

/**
 * Minimal platform/backend boundary for M1.
 *
 * The interface intentionally contains only lifecycle, capabilities and input-event polling.
 * Rendering and native peer creation will be added when the first visible backends make their
 * requirements concrete.
 */
class Backend {
public:
    virtual ~Backend() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual BackendCapabilities capabilities() const noexcept = 0;

    virtual void initialize() = 0;
    virtual void shutdown() noexcept = 0;

    [[nodiscard]] virtual std::optional<Event> pollEvent() = 0;
};

} // namespace sasd::ui
