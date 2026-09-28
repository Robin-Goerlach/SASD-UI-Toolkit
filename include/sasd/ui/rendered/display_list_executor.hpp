#pragma once

#include <cstddef>

namespace sasd::ui::rendered {

class DisplayList;
class RenderDevice;

/**
 * Replays an immutable DisplayList into one concrete RenderDevice in deterministic command order.
 *
 * Keeping replay separate from DisplayList itself is intentional:
 * - DisplayList remains a passive, easily testable frame/intermediate-representation value;
 * - device adapters implement drawing rather than owning semantic presentation traversal;
 * - a later SDL3 adapter can reuse the exact same command stream produced by headless tests.
 *
 * Execution is synchronous. If a RenderDevice callback throws, the exception propagates immediately,
 * remaining commands are not executed, and the DisplayList itself remains unchanged. The executor
 * does not promise rollback of commands already sent to the device; adapters that need transactional
 * presentation should render into their own back buffer/staging target and present only after a
 * successful replay.
 */
class DisplayListExecutor final {
public:
    DisplayListExecutor() = delete;

    /**
     * Executes every command in list and returns the number of commands successfully dispatched.
     *
     * For an empty list this returns zero and performs no device callbacks.
     */
    [[nodiscard]] static std::size_t execute(const DisplayList& list, RenderDevice& device);
};

} // namespace sasd::ui::rendered
