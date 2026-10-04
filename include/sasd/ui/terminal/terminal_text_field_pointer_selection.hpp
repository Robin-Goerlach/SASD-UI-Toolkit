#pragma once

#include <sasd/ui/hit_test.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/terminal/terminal_text_field_hit_test.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/widget.hpp>

namespace sasd::ui::terminal {

/**
 * Applies terminal TextField caret/selection geometry before performing normal Core pointer routing.
 *
 * This helper is intentionally the terminal counterpart of the rendered interaction seam, but it is
 * smaller because current SGR mouse input reports only ordinary press/move/release semantics. Core
 * PointerRouter remains the sole owner of capture lifetime, TextField remains the sole owner of the
 * semantic anchor/cursor pair, and TerminalTextFieldHitTest contributes only cell-based geometry.
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
     * - ordinary press collapses the selection at the mapped caret boundary;
     * - exact Shift+press preserves TextField's existing selection anchor and moves only the active end;
     * - unsupported/unrepresentable terminal geometry leaves semantic selection unchanged.
     *
     * Once PointerRouter capture belongs to a TextField, move and matching primary release use
     * caretIndexForDrag(). The stable semantic selection anchor therefore survives motion outside the
     * control while the active cursor follows the representable terminal viewport.
     *
     * No auto-scroll, multi-click synthesis or word-granular terminal selection is hidden in this
     * first interaction layer. Those policies can be added later without changing TextField or
     * PointerRouter ownership contracts.
     */
    [[nodiscard]] static PointerRouteResult route(
        Widget& root,
        PointerRouter& pointer_router,
        const PointerEvent& event,
        AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        TextField* pressed_field = nullptr;
        bool press_mapping_failed = false;

        if (!pointer_router.hasCapture() &&
            event.action == PointerAction::press &&
            event.button == PointerButton::primary) {
            /*
             * HitTest is used here only to decide whether TextField-specific terminal geometry should
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
                if (const auto caret = TerminalTextFieldHitTest::caretIndexAt(
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
                         * collapsed anchor/cursor pair. Other modifier combinations deliberately do
                         * not acquire special semantics yet; only exact Shift extends an old selection.
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
            press_mapping_failed &&
            pointer_router.capturedWidget() == pressed_field) {
            /*
             * Geometry failure is not a reason to suppress normal Widget dispatch, but retaining
             * capture would create a half-started selection gesture. Release only the capture created
             * by this same TextField press. releaseCapture() also invokes TextField's normal noexcept
             * capture-lost cleanup, keeping Core gesture state synchronized with routing state.
             */
            pointer_router.releaseCapture();
            result.capture_active = false;
        }

        return result;
    }
};

} // namespace sasd::ui::terminal
