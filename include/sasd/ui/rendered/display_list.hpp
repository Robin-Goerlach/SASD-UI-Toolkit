#pragma once

#include <sasd/ui/geometry.hpp>
#include <sasd/ui/style.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace sasd::ui::rendered {

/**
 * Paints a logical rectangle with one portable toolkit color.
 *
 * The command deliberately stores semantic Color rather than device RGB pixels. A concrete renderer
 * or theme adapter owns the final color mapping, which keeps DisplayList independent from SDL,
 * Win32, GTK, AppKit and a particular framebuffer format.
 */
struct FillRectCommand {
    Rect bounds{};
    Color color{Color::default_color};

    friend bool operator==(const FillRectCommand&, const FillRectCommand&) = default;
};

/** Draws a logical rectangle outline with a positive logical-unit thickness. */
struct StrokeRectCommand {
    Rect bounds{};
    Color color{Color::default_color};
    Coordinate thickness{1};

    friend bool operator==(const StrokeRectCommand&, const StrokeRectCommand&) = default;
};

/**
 * Draws owned UTF-8 text at a logical origin.
 *
 * Text is copied into the command intentionally. A display list represents one immutable frame
 * snapshot; later mutations of Label/TextField source strings must not change commands already
 * handed to a renderer.
 */
struct DrawTextCommand {
    Point origin{};
    std::string text;
    TextStyle style{};

    /**
     * Optional logical clipping rectangle.
     *
     * A per-command clip is enough for the first widget-rendering slice and avoids prematurely
     * introducing a mutable graphics-state/clip stack. The concrete renderer must honor this bound
     * after converting logical coordinates to device coordinates.
     */
    std::optional<Rect> clip_bounds;

    friend bool operator==(const DrawTextCommand&, const DrawTextCommand&) = default;
};

/** Initial M3 drawing vocabulary. It grows only when real widget rendering demonstrates a need. */
using DrawCommand = std::variant<FillRectCommand, StrokeRectCommand, DrawTextCommand>;

/**
 * Deterministic ordered intermediate representation for rendered-desktop presentation.
 *
 * DisplayList is intentionally much smaller than a general graphics API. It records the minimum
 * operations needed by current SASD widgets, while a later device adapter (for example SDL3)
 * converts logical coordinates/colors/text into platform-specific drawing calls.
 */
class DisplayList final {
public:
    [[nodiscard]] bool empty() const noexcept { return commands_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return commands_.size(); }
    [[nodiscard]] std::span<const DrawCommand> commands() const noexcept { return commands_; }

    /** Drops all commands while retaining reusable vector capacity. */
    void clear() noexcept { commands_.clear(); }

    /**
     * Appends a filled rectangle.
     *
     * Negative extents are malformed and rejected. Zero-area rectangles are valid no-ops: keeping
     * them out of the command stream makes later renderers simpler and deterministic.
     */
    void fillRect(Rect bounds, Color color = Color::default_color);

    /**
     * Appends a rectangle outline.
     *
     * thickness is expressed in logical units and must be positive. Device-pixel conversion belongs
     * to the concrete renderer/DPI policy, not this backend-neutral command list.
     */
    void strokeRect(Rect bounds,
                    Color color = Color::default_color,
                    Coordinate thickness = 1);

    /**
     * Appends owned UTF-8 text. Empty strings are no-ops.
     *
     * DisplayList preserves bytes exactly and does not impose a font/grapheme policy. Decoding,
     * shaping and glyph fallback are responsibilities of the later rendered text subsystem.
     *
     * clip_bounds, when supplied, uses the same logical coordinate space as origin. Negative clip
     * extents are rejected; an empty clip is a deterministic no-op.
     */
    void drawText(Point origin,
                  std::string_view utf8_text,
                  TextStyle style = {},
                  std::optional<Rect> clip_bounds = std::nullopt);

private:
    static void validateRect(Rect bounds);

    std::vector<DrawCommand> commands_;
};

} // namespace sasd::ui::rendered
