#pragma once

#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_text_field_hit_test.hpp>
#include <sasd/ui/text/selection_boundaries.hpp>
#include <sasd/ui/text/utf8.hpp>
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
     * - an exact unmodified triple click selects the complete single-line TextField content;
     * - an unmodified double click uses scalarIndexAt() plus Core's basic word-boundary policy;
     * - an exact Shift press preserves the existing semantic anchor and moves only the active end;
     * - every other successful press collapses selection at caretIndexAt();
     * - TextField handles the press, allowing PointerRouter to establish capture.
     *
     * Multi-click semantic selections are atomic in this slice. Once the TextField has consumed an
     * unmodified double/triple-click press, the helper immediately releases the just-created capture so
     * the matching release cannot collapse the semantic range through ordinary drag-finalization logic.
     * Word-/line-granular multi-click dragging is deliberately deferred until gesture granularity has
     * an explicit state model of its own.
     *
     * Captured move/release for ordinary/Shift-started gestures:
     * - the current captured Widget is queried from PointerRouter; no Widget pointer is retained here;
     * - caretIndexForDrag() maps even out-of-bounds positions against the current rendered viewport;
     * - the selection anchor established by the press stays stable while only the active cursor moves;
     * - release is finally routed so PointerRouter can retire capture normally.
     *
     * Shift is deliberately interpreted only when it is the exact modifier set. Control/Alt/Meta
     * combinations remain available for later word-selection or platform-specific policies instead of
     * being silently treated as ordinary Shift extension today. Likewise, double/triple-click semantic
     * selection currently requires exact no-modifier input; modified multi-click policy is left open.
     *
     * If a primary press hits a TextField but its required scalar geometry cannot be represented by the
     * metric provider, the event is still consumed by the TextField but any capture created by that
     * press is immediately released. This prevents a later move from extending stale selection state
     * while preserving the conservative "do not guess text geometry" rule.
     */
    [[nodiscard]] static PointerRouteResult route(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        const RenderedMeasurementContext& metrics) {
        TextField* pressed_field = nullptr;
        bool press_mapping_failed = false;
        bool atomic_multi_click_selection = false;

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
                const bool exact_unmodified_triple_click =
                    event.modifiers == KeyModifier::none &&
                    event.click_count == 3;
                const bool exact_unmodified_double_click =
                    event.modifiers == KeyModifier::none &&
                    event.click_count == 2;

                if (exact_unmodified_triple_click) {
                    /*
                     * TextField is intrinsically single-line. Its whole-content selection therefore is
                     * the natural single-line counterpart of desktop editors' triple-click line
                     * selection. Delegate the actual scalar range to Core's selectAll() operation so
                     * future menu/command integrations and this rendered gesture share one semantic
                     * definition of "complete TextField content".
                     */
                    pressed_field->selectAll();
                    atomic_multi_click_selection = true;
                } else if (exact_unmodified_double_click) {
                    /*
                     * Word selection needs the scalar whose painted span is under the pointer, not the
                     * nearest insertion boundary. Using caretIndexAt() here would select the following
                     * word when the pointer lies in the right half of the final glyph of the current
                     * word. ADR 0090 introduced scalarIndexAt() specifically to keep those two geometry
                     * questions separate.
                     */
                    if (const auto scalar = RenderedTextFieldHitTest::scalarIndexAt(
                            *pressed_field,
                            event.position,
                            metrics)) {
                        if (const auto word = text::basicWordRangeAt(
                                pressed_field->text(),
                                *scalar)) {
                            pressed_field->setSelection(word->start, word->end);
                            atomic_multi_click_selection = true;
                        } else if (const auto caret = RenderedTextFieldHitTest::caretIndexAt(
                                       *pressed_field,
                                       event.position,
                                       metrics)) {
                            /*
                             * Whitespace is intentionally not a "word" in the basic boundary policy.
                             * Falling back to ordinary caret placement is less surprising than selecting
                             * an arbitrary separator run and keeps empty/spacing clicks conservative.
                             */
                            pressed_field->setSelection(*caret, *caret);
                        } else {
                            press_mapping_failed = true;
                        }
                    } else {
                        /*
                         * scalarIndexAt() can legitimately return nullopt for blank interior space after
                         * the rendered text. In that case retain normal click behavior if an insertion
                         * caret can still be mapped; otherwise use the same unsupported-geometry path as
                         * an ordinary press.
                         */
                        if (const auto caret = RenderedTextFieldHitTest::caretIndexAt(
                                *pressed_field,
                                event.position,
                                metrics)) {
                            pressed_field->setSelection(*caret, *caret);
                        } else {
                            press_mapping_failed = true;
                        }
                    }
                } else if (const auto scalar = RenderedTextFieldHitTest::caretIndexAt(
                               *pressed_field,
                               event.position,
                               metrics)) {
                    const bool extends_existing_selection =
                        event.modifiers == KeyModifier::shift;

                    if (extends_existing_selection) {
                        /*
                         * Shift+press mirrors keyboard Shift navigation: preserve the semantic anchor
                         * that already belongs to TextField and move only the active end to the newly
                         * mapped scalar. This works for both collapsed and directed selections and
                         * naturally permits crossing the anchor without normalizing away direction.
                         */
                        pressed_field->setSelection(
                            pressed_field->selectionAnchor(),
                            *scalar);
                    } else {
                        /*
                         * A normal primary gesture starts from a fresh collapsed anchor/cursor pair.
                         * Subsequent captured moves therefore preserve exactly this press boundary as
                         * the stable anchor.
                         */
                        pressed_field->setSelection(*scalar, *scalar);
                    }
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
                     * Preserve the anchor established at pointer-down and move only the active end.
                     * For an ordinary press that anchor is the press boundary; for Shift+press it is
                     * the pre-existing TextField anchor. Crossing it remains a valid directed selection
                     * in either case.
                     */
                    captured->setSelection(captured->selectionAnchor(), *scalar);
                }
            }
        }

        PointerRouteResult result = pointer_router.route(root, event);

        if (pressed_field != nullptr &&
            pointer_router.capturedWidget() == pressed_field &&
            (press_mapping_failed || atomic_multi_click_selection)) {
            /*
             * Two categories intentionally retire the just-created TextField capture immediately:
             *
             * 1. unsupported geometry must not start a drag from stale selection state;
             * 2. an atomic multi-click semantic selection must survive the matching release unchanged.
             *
             * releaseCapture() invokes TextField::onPointerCaptureLost(), so Core's transient gesture
             * flag is also retired through the normal lifetime-safe handshake rather than by a special
             * back door in this helper.
             */
            pointer_router.releaseCapture();
            result.capture_active = false;
        }

        return result;
    }
};

} // namespace sasd::ui::rendered
