#pragma once

#include <sasd/ui/rendered/display_list.hpp>

namespace sasd::ui::rendered {

/**
 * Device-facing execution boundary for one already-built rendered DisplayList.
 *
 * RenderedPresentationSink answers the semantic question "what should this Widget look like?" by
 * producing backend-neutral DrawCommands. RenderDevice deliberately starts one layer lower: it
 * answers "how does this concrete drawing substrate execute those commands?".
 *
 * A future SDL3, native 2D or test device implements this interface without making semantic Widgets
 * depend on SDL/native handles. Commands remain expressed in the same logical coordinate system as
 * DisplayList. Device-pixel conversion, DPI scaling, color realization, font resources and actual
 * rasterization belong behind this boundary.
 *
 * The interface intentionally has no window/event-loop/frame-lifecycle methods yet. Those concerns
 * differ across real adapters and would be premature to freeze before the first desktop window is
 * implemented. DisplayListExecutor only replays drawing commands into an already-active device.
 */
class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    /** Executes one filled-rectangle command exactly as recorded in the DisplayList. */
    virtual void fillRect(const FillRectCommand& command) = 0;

    /** Executes one stroked-rectangle command exactly as recorded in the DisplayList. */
    virtual void strokeRect(const StrokeRectCommand& command) = 0;

    /**
     * Executes one UTF-8 text command exactly as recorded in the DisplayList.
     *
     * A concrete device owns font selection/shaping and must honor command.clip_bounds. The command
     * reference is valid only for the duration of this call; asynchronous devices must copy any data
     * they need after the callback returns.
     */
    virtual void drawText(const DrawTextCommand& command) = 0;
};

} // namespace sasd::ui::rendered
