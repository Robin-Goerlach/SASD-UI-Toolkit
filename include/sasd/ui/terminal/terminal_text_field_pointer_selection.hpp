#pragma once

#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/terminal/terminal_text_field_hit_test.hpp>
#include <sasd/ui/text/selection_boundaries.hpp>
#include <sasd/ui/text/utf8.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/widget.hpp>

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
    TerminalTextFieldPointerSelection() = delete;

    /**
     * Maps one terminal PointerEvent to TextField selection state and routes the event through Core.
     *
     * For a fresh primary press on a TextField:
     * - an exact unmodified triple click selects the complete semantic TextField contents;
     * - an exact unmodified double click selects the basic semantic word/punctuation run under the
     *   actually painted scalar;
     * - double click on whitespace or trailing caret space falls back to ordinary caret placement;
     * - ordinary press collapses the selection at the mapped caret boundary;
     * - exact Shift+press preserves TextField's existing selection anchor and moves only the active end;
     * - unsupported/unrepresentable terminal geometry leaves semantic selection unchanged.
     *
     * Double- and triple-click semantic selections are atomic in this stateless interaction layer.
     * After Core has processed the multi-click press, any capture just acquired by that TextField is
     * released immediately. This prevents the existing character-granular drag/release path from
     * collapsing or partially extending a complete word/all-text selection before dedicated
     * word-granular gesture state exists.
     *
     * Ordinary character-granular gestures retain capture. Their move and matching primary release use
     * caretIndexForDrag(), so the stable semantic selection anchor survives motion outside the control
     * while the active cursor follows the representable terminal viewport.
     *
     * Word-granular double-click dragging and auto-scroll remain separate later policies.
     */
    [[nodiscard]] static PointerRouteResult route(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        TextField* pressed_field = nullptr;
        bool press_mapping_failed = false;
        bool atomic_multi_click_selection = false;

        if (!pointer_router.hasCapture() &&
            event.action == PointerAction::press &&
            event.button == PointerButton::primary) {
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

                            /*
                             * The complete semantic run is committed on the press. We deliberately
                             * retire capture after routing because character-granular drag semantics
                             * must not subsequently erode that range. A later word-drag slice can add
                             * explicit host-owned gesture state without changing this atomic contract.
                             */
                            atomic_multi_click_selection = true;
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
                if (const auto caret = TerminalTextFieldHitTest::caretIndexForDrag(
                        *captured,
                        event.position,
                        ambiguous_width)) {
                    /*
                     * Gesture ownership came from PointerRouter capture, so the point may now be far
                     * outside the TextField. The terminal hit tester clamps only to geometry that the
                     * current viewport can honestly represent. Applying the mapped active end may in
                     * turn alter the viewport; the next event is then evaluated against the new state.
                     */
                    captured->setSelection(captured->selectionAnchor(), *caret);
                }
                /*
                 * A failed drag mapping deliberately leaves the previous semantic selection intact.
                 * Routing still proceeds so a matching release can retire capture and TextField's
                 * transient pointer-selection flag normally.
                 */
            }
        }

        PointerRouteResult result = pointer_router.route(root, event);

        if (pressed_field != nullptr &&
            pointer_router.capturedWidget() == pressed_field &&
            (press_mapping_failed || atomic_multi_click_selection)) {
            /*
             * Unsupported geometry and atomic double/triple-click semantic selections intentionally
             * retire the capture created by this same TextField press. releaseCapture() also invokes
             * TextField's normal noexcept capture-lost cleanup, keeping Core gesture state synchronized
             * with the semantic selection that has already been committed.
             */
            pointer_router.releaseCapture();
            result.capture_active = false;
        }

        return result;
    }
};

} // namespace sasd::ui::terminal
