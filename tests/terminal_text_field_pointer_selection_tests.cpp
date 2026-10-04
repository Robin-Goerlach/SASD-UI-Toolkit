#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/terminal/terminal_text_field_pointer_selection.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal TextField pointer selection collapses on press and extends through captured drag") {
    Window window;
    window.arrange({0, 0, 24, 4});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({1, 1, 8, 1}); // left/right chrome + six interior terminal cells

    /*
     * TextField intentionally initializes its semantic cursor at end-of-text. Terminal viewport
     * geometry follows that cursor, so a six-cell interior containing six one-cell scalars would start
     * scrolled at scalar 1 in order to reserve a representable caret cell. This regression wants to
     * exercise a press on the unscrolled "b" cell and then prove that later captured motion can move
     * the active end. Put the cursor at zero explicitly rather than relying on constructor state that
     * legitimately affects viewport geometry.
     */
    field.setCursorPosition(0);

    PointerRouter pointer_router;

    /*
     * TerminalTextFieldHitTest treats an interior terminal report as the center of that whole cell.
     * The field content starts at global x=2, so x=3 is the cell occupied by 'b'. A one-cell scalar's
     * center is an exact midpoint between adjacent caret boundaries and the documented tie-break moves
     * to the later boundary: scalar caret index 2, immediately after 'b'.
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{3, 1}, PointerAction::press, PointerButton::primary, 1});

    CHECK(press.targeted);
    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 2);
    CHECK(!field.hasSelection());

    /*
     * Capture, not geometry, owns the running gesture. Move far beyond both the field and the root to
     * prove that routing still reaches TextField. The drag hit tester clamps to the last caret boundary
     * representable in the current viewport; with this width that is index 5.
     */
    const auto move = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{1000, 1000}, PointerAction::move, PointerButton::none, 0});

    CHECK(move.targeted);
    CHECK(move.handled);
    CHECK(move.capture_active);
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 5);
    CHECK(field.selectedText() == "cde");

    const auto release = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{1000, 1000}, PointerAction::release, PointerButton::primary, 1});

    CHECK(release.targeted);
    CHECK(release.handled);
    CHECK(!release.capture_active);
    CHECK(!pointer_router.hasCapture());

    /*
     * Releasing capture retires only transient gesture ownership. The semantic TextField selection is
     * intentionally persistent and remains available for keyboard editing, clipboard commands and
     * presentation after the pointer button is released.
     */
    CHECK(field.selectionAnchor() == 2);
    CHECK(field.cursorPosition() == 5);
    CHECK(field.selectedText() == "cde");
}

TEST_CASE("Terminal TextField exact Shift press preserves the semantic selection anchor") {
    Window window;
    window.arrange({0, 0, 20, 3});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({0, 0, 8, 1});
    field.setSelection(1, 4);

    PointerRouter pointer_router;

    /*
     * Content begins at x=1. Cell x=5 is the 'e' cell and therefore maps to caret boundary 5. Exact
     * Shift mirrors keyboard selection extension: keep anchor 1 and move only the active cursor.
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {5, 0},
            PointerAction::press,
            PointerButton::primary,
            1,
            KeyModifier::shift});

    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 5);
    CHECK(field.selectedText() == "bcde");

    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{5, 0}, PointerAction::release, PointerButton::primary, 1});
    CHECK(!pointer_router.hasCapture());
}

TEST_CASE("Terminal TextField pointer interaction keeps logical focus as host policy") {
    Window window;
    window.arrange({0, 0, 20, 3});

    auto& first = window.emplace<TextField>("first");
    first.arrange({0, 0, 8, 1});
    auto& second = window.emplace<TextField>("second");
    second.arrange({0, 1, 9, 1});

    FocusManager focus;
    CHECK(focus.requestFocus(first));
    CHECK(first.hasFocus());
    CHECK(!second.hasFocus());

    PointerRouter pointer_router;

    /*
     * The interaction seam owns terminal selection geometry, not application/window focus policy.
     * Routing a valid primary press to the second field can start selection capture there without
     * silently stealing logical focus from the first field. A real host may request that focus
     * explicitly before calling route().
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{2, 1}, PointerAction::press, PointerButton::primary, 1});

    CHECK(press.handled);
    CHECK(pointer_router.capturedWidget() == &second);
    CHECK(first.hasFocus());
    CHECK(!second.hasFocus());

    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{2, 1}, PointerAction::release, PointerButton::primary, 1});
}

TEST_CASE("Terminal TextField pointer selection retires capture when cell geometry is unsupported") {
    Window window;
    window.arrange({0, 0, 20, 3});

    /*
     * Combining marks are preserved semantically by TextField but the current terminal Cell model
     * deliberately defers them. Pointer selection must therefore avoid guessing a caret boundary.
     */
    auto& field = window.emplace<TextField>(std::string{"e\xCC\x81"});
    field.arrange({0, 0, 8, 1});

    const std::size_t original_anchor = field.selectionAnchor();
    const std::size_t original_cursor = field.cursorPosition();

    PointerRouter pointer_router;
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{2, 0}, PointerAction::press, PointerButton::primary, 1});

    /*
     * Core still receives and handles the press, preserving one routing contract for every backend.
     * The helper then releases only the capture that this TextField just acquired because there is no
     * trustworthy semantic start position for a continued drag.
     */
    CHECK(press.targeted);
    CHECK(press.handled);
    CHECK(!press.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == original_anchor);
    CHECK(field.cursorPosition() == original_cursor);
}

TEST_CASE("Terminal TextField pointer selection delegates non TextField controls to PointerRouter") {
    Window window;
    window.arrange({0, 0, 20, 3});

    auto& button = window.emplace<Button>("Run");
    button.arrange({1, 1, 7, 1});

    int activations = 0;
    button.setOnActivated([&] {
        ++activations;
    });

    PointerRouter pointer_router;

    /*
     * The terminal TextField helper is an interaction seam, not a competing router. Non-TextField
     * targets must retain ordinary Core pointer behavior unchanged, including capture and activation.
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{3, 1}, PointerAction::press, PointerButton::primary, 1});

    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(pointer_router.capturedWidget() == &button);
    CHECK(button.isPressed());

    const auto release = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{3, 1}, PointerAction::release, PointerButton::primary, 1});

    CHECK(release.handled);
    CHECK(!release.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(!button.isPressed());
    CHECK(activations == 1);
}
