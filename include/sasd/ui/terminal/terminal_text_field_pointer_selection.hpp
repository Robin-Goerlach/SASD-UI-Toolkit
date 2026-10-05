#pragma once

#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/terminal/terminal_text_field_hit_test.hpp>
#include <sasd/ui/text/selection_boundaries.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/widget.hpp>

#include <optional>

namespace sasd::ui::terminal {

/**
 * Applies terminal TextField selection geometry before performing normal Core pointer routing.
 *
 * Core PointerRouter remains the sole owner of capture lifetime, TextField remains the sole owner of
 * the semantic anchor/cursor pair, and TerminalTextFieldHitTest contributes only cell-based geometry.
 * TerminalEventPump may enrich raw SGR transitions with click counts, but this interaction seam owns
 * the meaning of those counts for TextField selection.
 *
 * Logical focus is deliberately not owned here. Focus-on-primary-press is a top-level host/window
 * policy because modal scopes and application-specific focus rules are not terminal geometry. A host
 * may request focus for the geometric hit before calling route(), exactly as the SDL3 demo already
 * does for rendered controls.
 */
class TerminalTextFieldPointerSelection final {
public:
    /**
     * Host-owned transient state for semantic gestures that span multiple terminal PointerEvents.
     *
     * The state deliberately stores only the scalar-domain origin word. It owns no Widget pointer and
     * therefore cannot accidentally extend a TextField lifetime. PointerRouter capture remains the
     * authoritative source for the current gesture owner on every event.
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
        friend class TerminalTextFieldPointerSelection;

        std::optional<text::ScalarRange> word_origin_{};
    };

    TerminalTextFieldPointerSelection() = delete;

    /**
     * Stateless routing preserving atomic multi-click behavior.
     *
     * Double-click word selection and triple-click select-all are committed on the press and then
     * release any just-created TextField capture. This preserves the behavior introduced by ADR 0102
     * and ADR 0103 for hosts that do not keep semantic gesture state between events.
     */
    [[nodiscard]] static PointerRouteResult route(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        return routeImpl(root, pointer_router, event, ambiguous_width, nullptr);
    }

    /**
     * Stateful routing that additionally supports unmodified double-click-and-drag by semantic words.
     *
     * The initial double click selects the complete basic word/punctuation run under the painted
     * terminal scalar and keeps normal TextField capture alive. While that capture remains active,
     * move/release events map to visible scalar geometry and expand the active selection by complete
     * semantic runs:
     *
     * - dragging left anchors at the original word's end and moves the cursor to target-word start;
     * - dragging right anchors at the original word's start and moves the cursor to target-word end;
     * - returning to the origin word restores exactly the original range;
     * - whitespace leaves the latest word-granular selection unchanged instead of switching to a
     *   character endpoint midway through the gesture.
     *
     * Horizontal motion outside the control is clamped by scalarIndexForDrag() to the first/last scalar
     * actually painted in the current viewport. Moving the semantic cursor may change that viewport;
     * later motion is then evaluated against the new viewport, without hidden timer-driven auto-scroll.
     *
     * Triple-click select-all remains atomic even in this overload. Ordinary/Shift pointer gestures
     * retain the established character-granular caret drag behavior.
     */
    [[nodiscard]] static PointerRouteResult route(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        AmbiguousWidthMode ambiguous_width,
        GestureState& gesture_state) {
        return routeImpl(root, pointer_router, event, ambiguous_width, &gesture_state);
    }

private:
    [[nodiscard]] static PointerRouteResult routeImpl(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        AmbiguousWidthMode ambiguous_width,
        GestureState* gesture_state) {
        TextField* pressed_field = nullptr;
        bool press_mapping_failed = false;
        bool atomic_multi_click_selection = false;

        /*
         * A host may retire Core capture explicitly between calls (for example when entering a modal
         * menu). GestureState has no callback dependency on PointerRouter, so the next non-press event
         * without capture is a safe synchronization point for discarding a stale semantic word origin.
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
                 * Every fresh primary press starts a new semantic gesture. Reset before hit testing so
                 * unsupported geometry or a different target can never inherit an old word origin.
                 */
                gesture_state->reset();
            }

            /*
             * HitTest is used here only to decide whether TextField-specific terminal semantics should
             * be prepared. It does not establish capture. PointerRouter still dispatches the event and
             * captures whichever Widget actually handles the press after normal bubbling.
             *
             * TextField is currently a leaf control, so deepestAt() identifies the semantic target
             * directly. Keeping this distinction explicit avoids turning presentation geometry into a
             * second, backend-specific routing system.
             */
            if (auto* hit = HitTest::deepestAt(root, event.position); hit != nullptr) {
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
                     * TextField is intrinsically single-line, so whole-content selection is the natural
                     * terminal counterpart of desktop triple-click line selection. Once HitTest has
                     * identified the TextField target no terminal scalar geometry is required: the
                     * semantic selection is simply the complete Unicode-scalar interval [0,count).
                     *
                     * utf8::scalarCount() is intentionally used instead of std::string::size(). TextField
                     * selection indices are Unicode-scalar indices throughout Core, and a multi-byte
                     * UTF-8 scalar must therefore contribute exactly one semantic position.
                     */
                    pressed_field->setSelection(
                        0,
                        utf8::scalarCount(pressed_field->text()));
                    atomic_multi_click_selection = true;
                } else if (exact_unmodified_double_click) {
                    /*
                     * Word selection needs text identity, not a nearest insertion position. The strict
                     * scalar hit therefore rejects chrome and reserved caret space and maps both cells
                     * of one wide glyph to the same Unicode scalar.
                     */
                    if (const auto scalar = TerminalTextFieldHitTest::scalarIndexAt(
                            *pressed_field,
                            event.position,
                            ambiguous_width)) {
                        if (const auto word = text::basicWordRangeAt(
                                pressed_field->text(),
                                *scalar)) {
                            pressed_field->setSelection(word->start, word->end);

                            if (gesture_state != nullptr) {
                                /*
                                 * Stateful hosts keep only the scalar-domain origin range. Core capture
                                 * remains the authoritative Widget identity/lifetime channel, so no raw
                                 * TextField pointer is retained across backend events.
                                 */
                                gesture_state->word_origin_ = *word;
                            } else {
                                /*
                                 * Stateless hosts preserve the existing atomic contract: the completed
                                 * word selection must not be degraded by later character-granular motion.
                                 */
                                atomic_multi_click_selection = true;
                            }
                        } else if (const auto caret = TerminalTextFieldHitTest::caretIndexAt(
                                       *pressed_field,
                                       event.position,
                                       ambiguous_width)) {
                            /*
                             * Whitespace is not a word in the current basic boundary policy. Fall back
                             * to the same useful insertion behavior as an ordinary click instead of
                             * inventing a whitespace word-selection rule.
                             */
                            pressed_field->setSelection(*caret, *caret);
                        } else {
                            press_mapping_failed = true;
                        }
                    } else if (const auto caret = TerminalTextFieldHitTest::caretIndexAt(
                                   *pressed_field,
                                   event.position,
                                   ambiguous_width)) {
                        /*
                         * A strict scalar miss may still be valid insertion geometry, most notably the
                         * reserved trailing caret cell. Preserve ordinary caret behavior there while
                         * keeping chrome/trailing space from masquerading as text identity.
                         */
                        pressed_field->setSelection(*caret, *caret);
                    } else {
                        press_mapping_failed = true;
                    }
                } else if (const auto caret = TerminalTextFieldHitTest::caretIndexAt(
                               *pressed_field,
                               event.position,
                               ambiguous_width)) {
                    const bool extends_existing_selection =
                        event.modifiers == KeyModifier::shift;

                    if (extends_existing_selection) {
                        /*
                         * Match keyboard Shift navigation and the rendered pointer policy: TextField's
                         * existing anchor remains stable while only the active cursor follows the new
                         * terminal-cell caret boundary. Direction may therefore reverse naturally when
                         * the press crosses the anchor.
                         */
                        pressed_field->setSelection(
                            pressed_field->selectionAnchor(),
                            *caret);
                    } else {
                        /*
                         * Every ordinary primary press starts a new character-granular gesture from a
                         * collapsed anchor/cursor pair. Modified multi-clicks intentionally land here;
                         * only exact unmodified double/triple clicks receive special semantic selection.
                         */
                        pressed_field->setSelection(*caret, *caret);
                    }
                } else {
                    /*
                     * The control may still handle the Core press even when terminal geometry cannot
                     * be represented (for example combining-mark content). Remember that condition so
                     * a just-created capture can be retired after routing instead of leaving a gesture
                     * active with no trustworthy semantic start position.
                     */
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
                     * A semantic word gesture targets painted scalars, not insertion boundaries. The
                     * captured scalar mapper may clamp outside/trailing positions to visible text while
                     * still refusing unsupported/unpaintable geometry.
                     */
                    if (const auto scalar = TerminalTextFieldHitTest::scalarIndexForDrag(
                            *captured,
                            event.position,
                            ambiguous_width)) {
                        if (const auto target_word = text::basicWordRangeAt(
                                captured->text(),
                                *scalar)) {
                            if (target_word->end <= word_origin->start) {
                                /*
                                 * Dragging left keeps the entire origin word selected by anchoring at
                                 * its right boundary. The moving cursor uses the target word's complete
                                 * left boundary, naturally including intervening separators.
                                 */
                                captured->setSelection(
                                    word_origin->end,
                                    target_word->start);
                            } else if (target_word->start >= word_origin->end) {
                                /*
                                 * Symmetric right extension: anchor at the origin word's left boundary
                                 * and move to the complete right edge of the target semantic run.
                                 */
                                captured->setSelection(
                                    word_origin->start,
                                    target_word->end);
                            } else {
                                /*
                                 * Returning anywhere into the origin word restores exactly the original
                                 * double-click range instead of collapsing to a character caret.
                                 */
                                captured->setSelection(
                                    word_origin->start,
                                    word_origin->end);
                            }
                        }
                        /*
                         * Whitespace has no basic word range. In word-granular mode motion across it
                         * deliberately leaves the previous selection unchanged until another semantic
                         * run is reached, avoiding a silent granularity switch mid-gesture.
                         */
                    }
                } else if (const auto caret = TerminalTextFieldHitTest::caretIndexForDrag(
                               *captured,
                               event.position,
                               ambiguous_width)) {
                    /*
                     * Ordinary/Shift drag remains character-granular: preserve the semantic anchor from
                     * the press/pre-existing selection while only the active cursor follows caret geometry.
                     */
                    captured->setSelection(captured->selectionAnchor(), *caret);
                }
                /*
                 * A failed mapping leaves the previous semantic selection intact. Routing still proceeds
                 * so a matching release can retire Core capture and TextField transient pointer state.
                 */
            }
        } else if (gesture_state != nullptr && pointer_router.hasCapture()) {
            /*
             * Another Widget owns capture, so stored TextField word state cannot belong to this gesture.
             */
            gesture_state->reset();
        }

        PointerRouteResult result = pointer_router.route(root, event);

        if (pressed_field != nullptr &&
            pointer_router.capturedWidget() == pressed_field &&
            (press_mapping_failed || atomic_multi_click_selection)) {
            /*
             * Unsupported geometry and atomic multi-click semantic selections intentionally retire the
             * capture created by this same TextField press. releaseCapture() also invokes TextField's
             * normal noexcept capture-lost cleanup, keeping Core gesture state synchronized with the
             * semantic selection that has already been committed.
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
                 * PointerRouter has either completed the gesture or no longer owns one. Semantic word
                 * origin must not leak into a future primary press.
                 */
                gesture_state->reset();
            }
        }

        return result;
    }
};

} // namespace sasd::ui::terminal
