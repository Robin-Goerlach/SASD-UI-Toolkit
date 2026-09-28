#include <sasd/ui/rendered/display_list_executor.hpp>

#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/render_device.hpp>

#include <variant>

namespace sasd::ui::rendered {
namespace {

/**
 * Variant visitor kept local to the executor so concrete devices never need to duplicate
 * std::variant dispatch logic. Adding a new DrawCommand type will make this visitor incomplete at
 * compile time until the execution boundary is extended deliberately.
 */
class DeviceCommandVisitor final {
public:
    explicit DeviceCommandVisitor(RenderDevice& device) noexcept
        : device_{device} {}

    void operator()(const FillRectCommand& command) const {
        device_.fillRect(command);
    }

    void operator()(const StrokeRectCommand& command) const {
        device_.strokeRect(command);
    }

    void operator()(const DrawTextCommand& command) const {
        device_.drawText(command);
    }

private:
    RenderDevice& device_;
};

} // namespace

std::size_t DisplayListExecutor::execute(const DisplayList& list, RenderDevice& device) {
    std::size_t executed = 0;
    const DeviceCommandVisitor visitor{device};

    /*
     * DisplayList exposes an immutable span. Replay therefore cannot accidentally remove/reorder
     * commands while a device is consuming them. Increment the counter only after the callback
     * returns successfully; a throwing device stops at the failing command and the exception keeps
     * its original type for the adapter/application boundary to handle.
     */
    for (const DrawCommand& command : list.commands()) {
        std::visit(visitor, command);
        ++executed;
    }

    return executed;
}

} // namespace sasd::ui::rendered
