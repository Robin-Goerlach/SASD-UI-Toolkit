#include "test_framework.hpp"

#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/terminal/terminal_text_field_hit_test.hpp>
#include <sasd/ui/terminal/terminal_text_field_pointer_selection.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <optional>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal TextField captured scalar hit clamps to visible text rather than caret space") {
    Window window;
    window.arrange({0, 0, 16, 3});

    auto& field = window.emplace<TextField>("abc");
    field.arrange({2, 1, 6, 1}); // chrome + a/b/c + one reserved caret cell
    field.setCursorPosition(0);

    /*
     * Strict scalar hit testing deliberately reports no scalar for chrome or the reserved caret cell.
     * Once PointerRouter capture owns a word gesture, however, dragging beyond visible text needs a
     * stable semantic endpoint. The captured variant therefore clamps horizontally to the first/last
     * completely painted scalar and ignores y. No new text identity is invented for the blank cell.
     */
    CHECK(TerminalTextFieldHitTest::scalarIndexForDrag(field, {-1000, -1000}) ==
          std::optional<std::size_t>{0});
    CHECK(TerminalTextFieldHitTest::scalarIndexForDrag(field, {2, 1000}) ==
          std::optional<std::size_t>{0}); // left chrome
    CHECK(TerminalTextFieldHitTest::scalarIndexForDrag(field, {6, -1000}) ==
          std::optional<std::size_t>{2}); // reserved caret cell
    CHECK(TerminalTextFieldHitTest::scalarIndexForDrag(field, {7, 1000}) ==
          std::optional<std::size_t>{2}); // right chrome
    CHECK(TerminalTextFieldHitTest::scalarIndexForDrag(field, {1000, 1000}) ==
          std::optional<std::size_t>{2});
}

TEST_CASE("Terminal TextField stateful double click drags right by complete semantic words") {
    Window window;
    window.arrange({0, 0, 20, 3});

    auto& field = window.emplace<TextField>("one two three");
    field.arrange({0, 0, 16, 1}); // chrome + 13 one-cell scalars + reserved caret room
    field.setCursorPosition(0);

    PointerRouter pointer_router;
    TerminalTextFieldPointerSelection::GestureState gesture;

    /*
     * Content starts at x=1, so x=2 is scalar index 1 ('n') inside "one". The stateful overload selects
     * the full [0,3) origin word exactly like the stateless path, but deliberately keeps the ordinary
     * TextField capture alive and remembers only that scalar-domain origin range.
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{2, 0}, PointerAction::press, PointerButton::primary, 2},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(gesture.hasWordGesture());
    CHECK(field.selectionAnchor() == 0);
    CHECK(field.cursorPosition() == 3);
    CHECK(field.selectedText() == "one");

    /*
     * Move far beyond both the TextField and the root. Capture keeps the gesture owned by this field,
     * scalarIndexForDrag() clamps to the last painted scalar ('e' in "three"), and Core word boundaries
     * expand that scalar to [8,13). Rightward word dragging keeps the origin's left edge as anchor and
     * moves the active cursor to the complete target word's right edge, naturally including separators.
     */
    const auto move = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{1000, 1000}, PointerAction::move, PointerButton::none, 0},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(move.handled);
    CHECK(move.capture_active);
    CHECK(gesture.hasWordGesture());
    CHECK(field.selectionAnchor() == 0);
    CHECK(field.cursorPosition() == 13);
    CHECK(field.selectedText() == "one two three");

    const auto release = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{1000, 1000}, PointerAction::release, PointerButton::primary, 2},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(release.handled);
    CHECK(!release.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(!gesture.hasWordGesture());
    CHECK(field.selectionAnchor() == 0);
    CHECK(field.cursorPosition() == 13);
    CHECK(field.selectedText() == "one two three");
}

TEST_CASE("Terminal TextField word drag left preserves origin word and restores it on return") {
    Window window;
    window.arrange({0, 0, 20, 3});

    auto& field = window.emplace<TextField>("one two three");
    field.arrange({0, 0, 16, 1});
    field.setCursorPosition(0);

    PointerRouter pointer_router;
    TerminalTextFieldPointerSelection::GestureState gesture;

    /*
     * x=6 is scalar index 5 ('w') inside the middle word "two" -> origin [4,7). The initial word is
     * selected left-to-right, but the stored GestureState is a range rather than an orientation.
     */
    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{6, 0}, PointerAction::press, PointerButton::primary, 2},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(gesture.hasWordGesture());
    CHECK(field.selectionAnchor() == 4);
    CHECK(field.cursorPosition() == 7);
    CHECK(field.selectedText() == "two");

    /*
     * Drag into "one". A leftward word gesture anchors at the origin word's RIGHT boundary (7) and
     * moves the cursor to the target word's LEFT boundary (0). Checking anchor/cursor direction here is
     * important: selectedText() alone would not prove that reversing across the origin remains stable.
     */
    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{2, 0}, PointerAction::move, PointerButton::none, 0},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(field.selectionAnchor() == 7);
    CHECK(field.cursorPosition() == 0);
    CHECK(field.selectedText() == "one two");

    /*
     * Returning anywhere into the origin run must restore exactly [4,7), not collapse to whichever
     * character cell happens to be under the pointer. This is the key distinction between semantic
     * word drag and the older character-granular caret drag path.
     */
    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{6, 0}, PointerAction::move, PointerButton::none, 0},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(field.selectionAnchor() == 4);
    CHECK(field.cursorPosition() == 7);
    CHECK(field.selectedText() == "two");

    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{6, 0}, PointerAction::release, PointerButton::primary, 2},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(!pointer_router.hasCapture());
    CHECK(!gesture.hasWordGesture());
    CHECK(field.selectedText() == "two");
}

TEST_CASE("Terminal TextField word drag over whitespace keeps the last semantic word range") {
    Window window;
    window.arrange({0, 0, 20, 3});

    auto& field = window.emplace<TextField>("one two three");
    field.arrange({0, 0, 16, 1});
    field.setCursorPosition(0);

    PointerRouter pointer_router;
    TerminalTextFieldPointerSelection::GestureState gesture;

    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{6, 0}, PointerAction::press, PointerButton::primary, 2},
        AmbiguousWidthMode::narrow,
        gesture); // origin "two" [4,7)

    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{10, 0}, PointerAction::move, PointerButton::none, 0},
        AmbiguousWidthMode::narrow,
        gesture); // target "three" [8,13)

    CHECK(field.selectionAnchor() == 4);
    CHECK(field.cursorPosition() == 13);
    CHECK(field.selectedText() == "two three");

    /*
     * x=8 is scalar index 7, the separating whitespace before "three". basicWordRangeAt() intentionally
     * returns nullopt for whitespace. A word gesture must therefore hold its last semantic range rather
     * than silently switching back to character granularity at the separator.
     */
    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{8, 0}, PointerAction::move, PointerButton::none, 0},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(field.selectionAnchor() == 4);
    CHECK(field.cursorPosition() == 13);
    CHECK(field.selectedText() == "two three");

    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{8, 0}, PointerAction::release, PointerButton::primary, 2},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(!pointer_router.hasCapture());
    CHECK(!gesture.hasWordGesture());
    CHECK(field.selectedText() == "two three");
}

TEST_CASE("Terminal TextField word gesture state synchronizes after external capture retirement") {
    Window window;
    window.arrange({0, 0, 16, 2});

    auto& field = window.emplace<TextField>("alpha beta");
    field.arrange({0, 0, 13, 1});
    field.setCursorPosition(0);

    PointerRouter pointer_router;
    TerminalTextFieldPointerSelection::GestureState gesture;

    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{3, 0}, PointerAction::press, PointerButton::primary, 2},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(pointer_router.hasCapture());
    CHECK(gesture.hasWordGesture());

    /*
     * Hosts may explicitly retire capture when switching interaction scopes (for example opening the
     * terminal menu). GestureState intentionally has no observer callback into PointerRouter. The next
     * non-press event is therefore the synchronization seam that notices missing capture and discards
     * the stale word origin before any selection policy can reuse it.
     */
    pointer_router.releaseCapture();
    CHECK(!pointer_router.hasCapture());
    CHECK(gesture.hasWordGesture());

    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{4, 0}, PointerAction::move, PointerButton::none, 0},
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(!gesture.hasWordGesture());
}
