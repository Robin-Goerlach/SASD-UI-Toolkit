#pragma once

#include <sasd/ui/geometry.hpp>

#include <cstddef>
#include <optional>

namespace sasd::ui {

class TextField;

namespace rendered {

class RenderedMeasurementContext;

/**
 * Maps logical pointer positions onto rendered TextField scalar geometry.
 *
 * The mapping deliberately lives in the Rendered layer rather than TextField/Core because it depends
 * on font shaping, scalar advances and the current horizontal viewport. It uses the exact same
 * viewport builder as RenderedPresentationSink, preventing pointer placement and painted text/caret
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

    /**
     * Returns the Unicode-scalar whose shaped horizontal span actually contains point.
     *
     * This differs intentionally from caretIndexAt(): caret placement chooses the nearest boundary,
     * while multi-click selection needs to know which visible text scalar was hit. Empty trailing
     * viewport space, borders and zero-width spans therefore return std::nullopt instead of being
     * coerced to a neighboring scalar. A scalar that is partially clipped at the right viewport edge
     * remains hittable over its visible portion.
     *
     * The result uses logical scalar order, not grapheme clusters or bidirectional visual clusters.
     * Those richer text semantics remain a later Unicode-text concern; this API only supplies the
     * geometry fact needed by a higher-level selection policy.
     */
    [[nodiscard]] static std::optional<std::size_t> scalarIndexAt(
        const TextField& field,
        Point point,
        const RenderedMeasurementContext& metrics);

    /**
     * Returns the visible Unicode-scalar targeted by an already captured drag gesture.
     *
     * This is the scalar-span counterpart of caretIndexForDrag(). Once capture has established that a
     * TextField owns the gesture, motion may be vertically or horizontally outside the Widget. The
     * horizontal position is therefore clamped to the first/last *hittable visible scalar span* rather
     * than rejected. Blank viewport space to the right of short text likewise maps to the last visible
     * scalar, which is the useful semantic target for word-granular dragging.
     *
     * Zero-width spans are never invented as geometric targets. Disabled/hidden fields, empty visible
     * text, unsupported shaping boundaries and viewports with no positive-width scalar span still return
     * std::nullopt. Like every hit-test helper in this class, the method is read-only and performs no
     * scrolling or selection mutation itself.
     */
    [[nodiscard]] static std::optional<std::size_t> scalarIndexForDrag(
        const TextField& field,
        Point point,
        const RenderedMeasurementContext& metrics);
};

} // namespace rendered
} // namespace sasd::ui
