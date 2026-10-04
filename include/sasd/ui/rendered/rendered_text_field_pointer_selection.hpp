#pragma once

#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/rendered/rendered_text_field_hit_test.hpp>
#include <sasd/ui/text/selection_boundaries.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/widget.hpp>

#include <optional>

namespace sasd::ui::rendered {

/**
 * Applies rendered TextField pointer-selection geometry and then performs normal pointer routing.
 *
 * The helper keeps Core free of font/pixel/native backend knowledge. PointerRouter remains the sole
 * owner of capture lifetime, TextField remains the owner of semantic anchor/cursor state, and the
 * Rendered layer contributes only geometry plus desktop-style pointer-selection policy.
 *
 * Most callers can continue using the stateless route() overload. Hosts that want word-granular
 * double-click dragging pass a small GestureState object across successive pointer events. The state
 * intentionally stores no Widget pointer, so it cannot outlive a TextField accidentally; the current
 * gesture owner is always re-obtained from PointerRouter capture.
 */
class RenderedTextFieldPointerSelection final {
public:
    /**
     * Host-owned transient state for selection gestures whose semantics span multiple pointer events.
     *
     * The type deliberately exposes no mutable fields. A host only has to preserve the object for as
     * long as it preserves PointerRouter. reset() is public so a top-level host may retire semantic
     * gesture state together with a native surface/capture reset if desired.
     */
    class GestureState final {
    public:
        GestureState() = default;

        void reset() noexcept {
            word_origin_.reset();
        }

        [[nodiscard]] bool hasWordGesture() const noexcept {
            return word_origin_.has_value();
        }

    private:
        friend class RenderedTextFieldPointerSelection;

        std::optional<text::ScalarRange> word_origin_{};
    };

    RenderedTextFieldPointerSelection() = delete;

    /**
     * Backward-compatible stateless routing.
     *
     * Double/triple-click semantic selections remain atomic in this overload, matching the behavior
     * introduced by ADR 0091/0092: the just-created TextField capture is released immediately so the
     * matching release cannot collapse the selected range. Use the GestureState overload when a host
     * wants double-click-and-drag word extension across later move/release events.
     */
    [[nodiscard]] static PointerRouteResult route(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        const RenderedMeasurementContext& metrics) {
        return routeImpl(root, pointer_router, event, metrics, nullptr);
    }

    /**
     * Stateful routing that additionally supports unmodified double-click-and-drag by semantic runs.
     *
     * The initial double click selects the word/punctuation run under the painted scalar and keeps the
     * normal TextField capture alive. While capture remains active, each move/release maps to the scalar
     * under the captured drag position and then expands to that scalar's complete basic word run.
     * Crossing the original run flips selection direction without losing the original run:
     *
     * - dragging left anchors at the original run's end and moves the cursor to target-run start;
     * - dragging right anchors at the original run's start and moves the cursor to target-run end;
     * - returning to the original run restores exactly the initial range.
     *
     * Whitespace is not a word in the current basic boundary policy. Motion over whitespace therefore
     * leaves the most recent word-granular selection unchanged until a semantic run is hit again. This
     * avoids silently falling back to character-granular endpoints in the middle of a word gesture.
     * Intervening whitespace is naturally included once the pointer reaches a run on the far side.
     *
     * Triple-click select-all remains atomic. Shift/click and ordinary character dragging retain their
     * established behavior. The state is reset after matching release, when routing observes that
     * capture disappeared, and at the start of every new primary press.
     */
    [[nodiscard]] static PointerRouteResult route(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        const RenderedMeasurementContext& metrics,
        GestureState& gesture_state) {
        return routeImpl(root, pointer_router, event, metrics, &gesture_state);
    }

private:
    [[nodiscard]] static PointerRouteResult routeImpl(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        const RenderedMeasurementContext& metrics,
        GestureState* gesture_state) {
        TextField* pressed_field = nullptr;
        bool press_mapping_failed = false;
        bool atomic_multi_click_selection = false;

        /*
         * A host may have retired capture through leaveRoot()/releaseCapture() between two calls. The
         * state object intentionally has no callback dependency on PointerRouter, so a subsequent event
         * without capture is the safe synchronization point for discarding any stale word origin.
         */
        if (gesture_state != nullptr &&
            !pointer_router.hasCapture() &&
            event.action != PointerAction::press) {
            gesture_state->reset();
        }

        if (!pointer_router.hasCapture() &&
            event.action == PointerAction::press &&
            event.button == PointerButton::primary) {
            if (gesture_state != nullptr) {
                /*
                 * Every fresh primary press starts a new semantic gesture. Reset before HitTest so an
                 * unsupported/new target cannot inherit word granularity from an older capture.
                 */
                gesture_state->reset();
            }

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
                     * TextField is intrinsically single-line. Whole-content selection is therefore the
                     * natural counterpart of line-selection semantics and needs no rendered geometry
                     * once HitTest has identified the TextField target.
                     */
                    pressed_field->setSelection(
                        0,
                        utf8::scalarCount(pressed_field->text()));
                    atomic_multi_click_selection = true;
                } else if (exact_unmodified_double_click) {
                    /*
                     * Word selection needs the scalar whose painted span is under the pointer, not the
                     * nearest insertion boundary. caretIndexAt() would be wrong in the right half of a
                     * glyph because it can already refer to the following insertion boundary.
                     */
                    if (const auto scalar = RenderedTextFieldHitTest::scalarIndexAt(
                            *pressed_field,
                            event.position,
                            metrics)) {
                        if (const auto word = text::basicWordRangeAt(
                                pressed_field->text(),
                                *scalar)) {
                            pressed_field->setSelection(word->start, word->end);

                            if (gesture_state != nullptr) {
                                /*
                                 * Keep only scalar-domain origin information. PointerRouter capture is
                                 * the authoritative owner identity/lifetime; retaining TextField* here
                                 * would create a second, potentially dangling gesture-lifetime channel.
                                 */
                                gesture_state->word_origin_ = *word;
                            } else {
                                atomic_multi_click_selection = true;
                            }
                        } else if (const auto caret = RenderedTextFieldHitTest::caretIndexAt(
                                       *pressed_field,
                                       event.position,
                                       metrics)) {
                            /*
                             * Whitespace is intentionally not a word. A double click there falls back
                             * to ordinary caret placement and does not start word-granular drag state.
                             */
                            pressed_field->setSelection(*caret, *caret);
                        } else {
                            press_mapping_failed = true;
                        }
                    } else if (const auto caret = RenderedTextFieldHitTest::caretIndexAt(
                                   *pressed_field,
                                   event.position,
                                   metrics)) {
                        /*
                         * Trailing blank viewport space has no scalar under it, but it can still map to
                         * a valid insertion caret. Preserve normal click behavior in that case.
                         */
                        pressed_field->setSelection(*caret, *caret);
                    } else {
                        press_mapping_failed = true;
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
                         * already owned by TextField and move only the active end.
                         */
                        pressed_field->setSelection(
                            pressed_field->selectionAnchor(),
                            *scalar);
                    } else {
                        /*
                         * Ordinary primary dragging starts from a fresh collapsed anchor/cursor pair.
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
                const auto word_origin =
                    gesture_state != nullptr ? gesture_state->word_origin_ : std::nullopt;

                if (word_origin.has_value()) {
                    /*
                     * Word dragging maps to a painted scalar span, not a caret boundary. The captured
                     * variant clamps outside/trailing positions to visible scalar geometry while still
                     * refusing to invent targets for unsupported shaping boundaries.
                     */
                    if (const auto scalar = RenderedTextFieldHitTest::scalarIndexForDrag(
                            *captured,
                            event.position,
                            metrics)) {
                        if (const auto target_word = text::basicWordRangeAt(
                                captured->text(),
                                *scalar)) {
                            if (target_word->end <= word_origin->start) {
                                /*
                                 * Dragging left keeps the original word wholly selected by anchoring at
                                 * its right boundary. The target's complete left boundary becomes the
                                 * moving cursor, so intervening separators are included automatically.
                                 */
                                captured->setSelection(
                                    word_origin->end,
                                    target_word->start);
                            } else if (target_word->start >= word_origin->end) {
                                /*
                                 * Symmetric right extension: anchor at the original word's left edge and
                                 * move the cursor to the target run's complete right boundary.
                                 */
                                captured->setSelection(
                                    word_origin->start,
                                    target_word->end);
                            } else {
                                /*
                                 * Returning anywhere into the original semantic run restores exactly the
                                 * range created by the double click instead of collapsing to a caret.
                                 */
                                captured->setSelection(
                                    word_origin->start,
                                    word_origin->end);
                            }
                        }
                        /*
                         * basicWordRangeAt() returns nullopt for whitespace. In word-granular mode we
                         * deliberately keep the previous selection unchanged until a word/punctuation
                         * run is reached, rather than switching granularity mid-gesture.
                         */
                    }
                } else if (const auto scalar = RenderedTextFieldHitTest::caretIndexForDrag(
                               *captured,
                               event.position,
                               metrics)) {
                    /*
                     * Character-granular ordinary/Shift drag keeps the press/pre-existing anchor stable
                     * while the active cursor follows rendered caret geometry.
                     */
                    captured->setSelection(captured->selectionAnchor(), *scalar);
                }
            }
        } else if (gesture_state != nullptr && pointer_router.hasCapture()) {
            /*
             * Another Widget owns capture, so TextField word state cannot belong to the current gesture.
             */
            gesture_state->reset();
        }

        PointerRouteResult result = pointer_router.route(root, event);

        if (pressed_field != nullptr &&
            pointer_router.capturedWidget() == pressed_field &&
            (press_mapping_failed || atomic_multi_click_selection)) {
            /*
             * Unsupported geometry and stateless/atomic semantic multi-click selections intentionally
             * retire the just-created capture immediately. releaseCapture() also invokes the normal
             * TextField::onPointerCaptureLost() lifetime handshake.
             */
            pointer_router.releaseCapture();
            result.capture_active = false;
        }

        if (gesture_state != nullptr) {
            const bool matching_primary_release =
                event.action == PointerAction::release &&
                event.button == PointerButton::primary;

            if (matching_primary_release || !result.capture_active) {
                /*
                 * PointerRouter has either just completed the gesture or no longer owns one. Semantic
                 * word-origin state must not leak into a future press.
                 */
                gesture_state->reset();
            }
        }

        return result;
    }
};

} // namespace sasd::ui::rendered
