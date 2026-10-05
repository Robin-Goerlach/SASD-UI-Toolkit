#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/terminal/terminal_text_field_hit_test.hpp>
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

TEST_CASE("Terminal TextField scalar hit identifies only cells occupied by visible text") {
    Window window;
    window.arrange({0, 0, 14, 3});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({2, 1, 6, 1}); // chrome + three scalars + one reserved caret cell
    field.setCursorPosition(0);

    /*
     * Scalar hits answer a stricter question than caret hits. The left/right chrome and the reserved
     * end-caret cell are valid insertion geometry, but none of them is a Unicode scalar under the
     * pointer. Only the three cells actually painted with a/b/c may produce scalar identities.
     */
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {2, 1}).has_value());
    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {3, 1}) ==
          std::optional<std::size_t>{0});
    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {4, 1}) ==
          std::optional<std::size_t>{1});
    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {5, 1}) ==
          std::optional<std::size_t>{2});
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {6, 1}).has_value());
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {7, 1}).has_value());
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {8, 1}).has_value());
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {4, 0}).has_value());
}

TEST_CASE("Terminal TextField scalar hit maps both wide-glyph cells to one Unicode scalar") {
    Window window;
    window.arrange({0, 0, 14, 2});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"});
    field.arrange({0, 0, 7, 1}); // five interior cells: A, wide scalar, B, caret room
    field.setCursorPosition(0);

    /*
     * U+754C occupies one lead and one continuation cell in the terminal presentation. Those are two
     * physical cells but one semantic Unicode scalar. Word selection must therefore receive index 1
     * from either cell instead of treating the continuation cell as a second character.
     */
    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {1, 0}) ==
          std::optional<std::size_t>{0});
    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {2, 0}) ==
          std::optional<std::size_t>{1});
    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {3, 0}) ==
          std::optional<std::size_t>{1});
    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {4, 0}) ==
          std::optional<std::size_t>{2});
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {5, 0}).has_value());
}

TEST_CASE("Terminal TextField scalar hit follows horizontal viewport and excludes caret space") {
    Window window;
    window.arrange({0, 0, 12, 2});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({1, 0, 5, 1}); // three interior cells
    field.setCursorPosition(6);

    /*
     * The established terminal viewport scrolls to scalar 4 so e/f plus the end caret fit. Strict
     * scalar mapping must therefore expose only global indices 4 and 5. The third interior cell is
     * still a valid caret location but deliberately has no scalar identity.
     */
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {1, 0}).has_value());
    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {2, 0}) ==
          std::optional<std::size_t>{4});
    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {3, 0}) ==
          std::optional<std::size_t>{5});
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {4, 0}).has_value());
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {5, 0}).has_value());
}

TEST_CASE("Terminal TextField scalar hit does not expose an unpaintable partial wide scalar") {
    Window window;
    window.arrange({0, 0, 10, 2});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C"});
    field.arrange({0, 0, 4, 1}); // two interior cells: only A can be painted completely
    field.setCursorPosition(0);

    CHECK(TerminalTextFieldHitTest::scalarIndexAt(field, {1, 0}) ==
          std::optional<std::size_t>{0});

    /*
     * The wide scalar starts in the second interior cell but would require a third one. Presentation
     * intentionally refuses to draw half of it, so interaction must also report no text at that cell.
     */
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {2, 0}).has_value());
}

TEST_CASE("Terminal TextField scalar hit remains conservative for unsupported control state") {
    Window window;
    window.arrange({0, 0, 12, 2});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({0, 0, 6, 1});
    field.setCursorPosition(0);

    field.setEnabled(false);
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {1, 0}).has_value());
    field.setEnabled(true);

    field.setVisible(false);
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {1, 0}).has_value());
    field.setVisible(true);

    field.setText(std::string{"e\xCC\x81"}); // unsupported combining sequence in current Cell model
    CHECK(!TerminalTextFieldHitTest::scalarIndexAt(field, {1, 0}).has_value());
}

TEST_CASE("Terminal TextField unmodified double click selects a complete basic word atomically") {
    Window window;
    window.arrange({0, 0, 24, 3});

    auto& field = window.emplace<TextField>("alpha beta");
    field.arrange({0, 0, 13, 1}); // chrome + ten scalars + reserved end-caret cell

    PointerRouter pointer_router;

    /*
     * Content begins at x=1, so x=3 is the cell painted with scalar index 2 ('p'). The terminal hit
     * layer identifies that actual scalar and Core basicWordRangeAt() expands it to [0,5), selecting
     * the whole word "alpha". This proves that cell geometry and semantic word boundaries remain
     * separate responsibilities joined only by a Unicode-scalar index.
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{3, 0}, PointerAction::press, PointerButton::primary, 2});

    CHECK(press.targeted);
    CHECK(press.handled);
    CHECK(!press.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 0);
    CHECK(field.cursorPosition() == 5);
    CHECK(field.selectedText() == "alpha");

    /*
     * Word selection is intentionally atomic until terminal word-drag state exists. The press briefly
     * follows normal Core routing so TextField receives its standard event, then the helper releases
     * the newly acquired capture. A later physical release must therefore not collapse the committed
     * semantic word range through the ordinary character-drag path.
     */
    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{3, 0}, PointerAction::release, PointerButton::primary, 2});

    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 0);
    CHECK(field.cursorPosition() == 5);
    CHECK(field.selectedText() == "alpha");
}

TEST_CASE("Terminal TextField double click treats both cells of a wide scalar as one word target") {
    Window window;
    window.arrange({0, 0, 14, 2});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"});
    field.arrange({0, 0, 7, 1}); // chrome + A + two-cell U+754C + B + caret room
    field.setCursorPosition(0);

    PointerRouter pointer_router;

    /*
     * The wide scalar U+754C occupies content cells x=2 and x=3. Double-click the continuation cell,
     * which must still resolve to scalar index 1. Core classifies A/U+754C/B as one contiguous
     * word-like run, so the complete three-scalar UTF-8 text is selected. No terminal continuation-cell
     * concept leaks into the word-boundary helper.
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{3, 0}, PointerAction::press, PointerButton::primary, 2});

    CHECK(press.handled);
    CHECK(!press.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 0);
    CHECK(field.cursorPosition() == 3);
    CHECK(field.selectedText() == std::string{"A\xE7\x95\x8C" "B"});
}

TEST_CASE("Terminal TextField double click on whitespace falls back to ordinary caret behavior") {
    Window window;
    window.arrange({0, 0, 24, 3});

    auto& field = window.emplace<TextField>("alpha beta");
    field.arrange({0, 0, 13, 1});

    PointerRouter pointer_router;

    /*
     * Scalar index 5 is the separating space at content cell x=6. basicWordRangeAt() deliberately
     * returns nullopt for whitespace, so double click must not invent a whitespace word. It instead
     * uses caret geometry; the one-cell midpoint tie maps to boundary 6, immediately after the space.
     * Because this is ordinary caret behavior rather than an atomic word selection, Core capture stays
     * alive until the matching release just like a normal character-granular gesture.
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{6, 0}, PointerAction::press, PointerButton::primary, 2});

    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(field.selectionAnchor() == 6);
    CHECK(field.cursorPosition() == 6);
    CHECK(!field.hasSelection());

    const auto release = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{6, 0}, PointerAction::release, PointerButton::primary, 2});

    CHECK(release.handled);
    CHECK(!release.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 6);
    CHECK(field.cursorPosition() == 6);
    CHECK(!field.hasSelection());
}
