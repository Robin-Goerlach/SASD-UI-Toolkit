#pragma once

#include <sasd/ui/geometry.hpp>

#include <cstddef>
#include <optional>

namespace sasd::ui {

class TextField;

namespace rendered {

class RenderedMeasurementContext;

/**
 * Maps a logical pointer position to the insertion-caret scalar used by a rendered TextField.
 *
 * The mapping deliberately lives in the Rendered layer rather than TextField/Core because it depends
 * on font shaping, scalar advances and the current horizontal viewport. It uses the exact same
 * viewport builder as RenderedPresentationSink, preventing pointer placement and painted caret
 * geometry from silently diverging.
 */
class RenderedTextFieldHitTest final {
public:
    RenderedTextFieldHitTest() = delete;

    /**
     * Returns the nearest representable Unicode-scalar boundary for a point inside the field.
     *
     * point is expressed in the same top-level logical coordinate system as PointerEvent.
     * std::nullopt means the field is not interactable at that point, has no drawable interior, or
     * the current metric provider cannot faithfully express a visible scalar boundary. Callers must
     * not guess an index in that case.
     *
     * Only caret boundaries that fit in the current visible viewport are selectable. Clicking the
     * left/right border clamps to the first/last visible boundary. Midpoint ties deliberately choose
     * the later boundary, matching conventional "right half of glyph advances caret" behavior.
     */
    [[nodiscard]] static std::optional<std::size_t> caretIndexAt(
        const TextField& field,
        Point point,
        const RenderedMeasurementContext& metrics);

    /**
     * Returns the nearest representable boundary for a captured drag position.
     *
     * Unlike caretIndexAt(), point is allowed to lie outside the TextField bounds. This is intended
     * for pointer-capture based text selection: once a primary press has established the TextField as
     * the gesture owner, subsequent motion may legitimately leave the original rectangle. Horizontal
     * coordinates are clamped to the current visible text capacity; the vertical coordinate does not
     * participate in the single-line caret decision.
     *
     * The method still rejects hidden/disabled fields and any viewport or shaping state that cannot
     * be represented exactly. It does not mutate TextField selection state and it does not implement
     * auto-scroll; callers remain responsible for gesture lifetime and for applying the returned
     * scalar through TextField's existing selection APIs.
     */
    [[nodiscard]] static std::optional<std::size_t> caretIndexForDrag(
        const TextField& field,
        Point point,
        const RenderedMeasurementContext& metrics);
};

} // namespace rendered
} // namespace sasd::ui
