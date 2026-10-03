#pragma once

#include <sasd/ui/backend/capabilities.hpp>
#include <sasd/ui/events/event.hpp>

#include <optional>
#include <string_view>

namespace sasd::ui {

class Clipboard;

/**
 * Minimal platform/backend boundary for M1 and later capability services.
 *
 * Lifecycle, capabilities and semantic input polling remain the core responsibilities. Optional
 * facilities that genuinely belong to the host environment are exposed as narrow backend-neutral
 * services instead of leaking platform handles into Core. Clipboard is the first such service.
 * Rendering and native peer creation stay outside this interface until their concrete contracts are
 * proven by the corresponding presentation layers.
 */
class Backend {
public:
    virtual ~Backend() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual BackendCapabilities capabilities() const noexcept = 0;

    virtual void initialize() = 0;
    virtual void shutdown() noexcept = 0;

    [[nodiscard]] virtual std::optional<Event> pollEvent() = 0;

    /**
     * Returns the backend-provided clipboard service when one exists.
     *
     * The returned pointer is non-owning and must not outlive the Backend. A null pointer means this
     * backend does not currently expose clipboard functionality. Backends that report
     * BackendCapabilities::clipboard == true are expected to return a non-null service while their
     * clipboard facility is usable; concrete operations may still throw on transient native errors.
     *
     * Default implementations return nullptr so existing backends remain source-compatible while
     * clipboard support is added deliberately per adapter instead of being faked universally.
     */
    [[nodiscard]] virtual Clipboard* clipboard() noexcept { return nullptr; }
    [[nodiscard]] virtual const Clipboard* clipboard() const noexcept { return nullptr; }
};

} // namespace sasd::ui
