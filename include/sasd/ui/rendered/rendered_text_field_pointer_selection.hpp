#pragma once

#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_text_field_hit_test.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/widget.hpp>

namespace sasd::ui::rendered {

/**
 * Applies rendered TextField pointer-selection geometry and then performs normal pointer routing.
 *
 * This helper is intentionally stateless. Gesture lifetime remains owned by PointerRouter capture and
 * TextField's transient primary-pointer state; the stable semantic selection anchor remains stored in
 * TextField itself. The Rendered layer contributes only the font/viewport-dependent conversion from
 * logical pointer coordinates to Unicode-scalar boundaries.
 *
 * Hosts remain free to apply focus-on-press before calling route(). Focus policy is deliberately not
 * folded into this helper: FocusManager, PointerRouter and rendered text geometry remain independent
 * responsibilities.
 */
class RenderedTextFieldPointerSelection final {
public:
    RenderedTextFieldPointerSelection() = delete;

    /**
     * Updates TextField selection state when the pointer belongs to a rendered text-selection gesture,
     * then delegates the event to PointerRouter.
     *
     * Primary press:
     * - HitTest locates the TextField under the pointer.
     * - caretIndexAt() maps the press to an exact scalar boundary.
     * - a successful mapping collapses selection at that boundary before routing the press.
     * - TextField handles the press, allowing PointerRouter to establish capture.
     *
     * Captured move/release:
     * - the current captured Widget is queried from PointerRouter; no Widget pointer is retained here;
     * - caretIndexForDrag() maps even out-of-bounds positions against the current rendered viewport;
     * - the original TextField selectionAnchor() stays stable while only the active cursor moves;
     * - release is finally routed so PointerRouter can retire capture normally.
     *
     * If a primary press hits a TextField but its scalar boundary cannot be represented by the metric
     * provider, the event is still consumed by the TextField but any capture created by that press is
     * immediately released. This prevents a later move from extending a stale pre-existing anchor while
     * preserving the conservative "do not guess text geometry" rule.
     */
    [[nodiscard]] static PointerRouteResult route(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        const RenderedMeasurementContext& metrics) {
        TextField* pressed_field = nullptr;
        bool press_mapping_failed = false;

        if (!pointer_router.hasCapture() &&
            event.action == PointerAction::press &&
            event.button == PointerButton::primary) {
            /*
             * Do not ask PointerRouter for the future capture target before dispatch: capture belongs
             * to the Widget that actually handles the press, which may differ from the deepest hit due
             * to bubbling. We only use HitTest here to decide whether TextField-specific rendered
             * geometry should be prepared for the deepest semantic target.
             */
            if (auto* hit = HitTest::deepestAt(root, event.position);
                hit != nullptr) {
                pressed_field = dynamic_cast<TextField*>(hit);
            }

            if (pressed_field != nullptr) {
                if (const auto scalar = RenderedTextFieldHitTest::caretIndexAt(
                        *pressed_field,
                        event.position,
                        metrics)) {
                    /*
                     * A fresh pointer gesture always starts from a collapsed anchor/cursor pair. The
                     * subsequent captured moves therefore preserve exactly this press boundary as the
                     * stable anchor, matching keyboard Shift-selection semantics.
                     */
                    pressed_field->setSelection(*scalar, *scalar);
                } else {
                    press_mapping_failed = true;
                }
            }
        } else if (auto* captured =
                       dynamic_cast<TextField*>(pointer_router.capturedWidget());
                   captured != nullptr) {
            const bool extends_selection =
                event.action == PointerAction::move ||
                (event.action == PointerAction::release &&
                 event.button == PointerButton::primary);

            if (extends_selection) {
                if (const auto scalar = RenderedTextFieldHitTest::caretIndexForDrag(
                        *captured,
                        event.position,
                        metrics)) {
                    /*
                     * Preserve the original pointer-down anchor and move only the active end. Crossing
                     * the anchor is valid and naturally reverses the directed selection without any
                     * special-case normalization in the interaction layer.
                     */
                    captured->setSelection(captured->selectionAnchor(), *scalar);
                }
            }
        }

        PointerRouteResult result = pointer_router.route(root, event);

        if (press_mapping_failed &&
            pressed_field != nullptr &&
            pointer_router.capturedWidget() == pressed_field) {
            /*
             * TextField handled the primary press so the event does not unexpectedly bubble as if the
             * editor were inert, but unsupported shaping geometry must not create a drag based on old
             * selection state. Retire that just-created capture immediately and report the final router
             * state rather than the transient state returned by route().
             */
            pointer_router.releaseCapture();
            result.capture_active = false;
        }

        return result;
    }
};

} // namespace sasd::ui::rendered
