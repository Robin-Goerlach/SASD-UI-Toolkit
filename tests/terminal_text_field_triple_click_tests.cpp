#include "test_framework.hpp"

#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/terminal/terminal_text_field_pointer_selection.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal TextField unmodified triple click selects all Unicode scalars atomically") {
    Window window;
    window.arrange({0, 0, 16, 2});

    auto& field = window.emplace<TextField>(std::string{"A\xE7\x95\x8C" "B"});
    field.arrange({0, 0, 7, 1}); // chrome + A + two-cell U+754C + B + caret room
    field.setSelection(1, 2);    // start with only the wide scalar selected

    PointerRouter pointer_router;

    /*
     * Triple-click select-all is a semantic TextField operation once normal HitTest has identified the
     * control. It deliberately does not ask scalarIndexAt(): the right chrome at x=6 is part of the
     * TextField hit target but is not text geometry. Desktop-style whole-line/whole-field triple click
     * should therefore still select all content from this position instead of requiring a glyph hit.
     *
     * The UTF-8 payload contains five bytes but only three Unicode scalars. The resulting cursor must be
     * scalar index 3, proving that the interaction uses Core's scalar-domain selection contract rather
     * than std::string byte offsets.
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{6, 0}, PointerAction::press, PointerButton::primary, 3});

    CHECK(press.targeted);
    CHECK(press.handled);
    CHECK(!press.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 0);
    CHECK(field.cursorPosition() == 3);
    CHECK(field.selectedText() == std::string{"A\xE7\x95\x8C" "B"});

    /*
     * Like atomic double-click word selection, triple-click select-all releases the capture briefly
     * acquired through ordinary Core routing. A later physical release therefore cannot route through
     * the character-granular drag path and accidentally shrink the already committed full selection.
     */
    (void)TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{{6, 0}, PointerAction::release, PointerButton::primary, 3});

    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 0);
    CHECK(field.cursorPosition() == 3);
    CHECK(field.selectedText() == std::string{"A\xE7\x95\x8C" "B"});
}

TEST_CASE("Terminal TextField modified triple click keeps existing Shift selection semantics") {
    Window window;
    window.arrange({0, 0, 16, 2});

    auto& field = window.emplace<TextField>("abcdef");
    field.arrange({0, 0, 9, 1}); // chrome + six scalars + reserved caret cell
    field.setSelection(1, 1);

    PointerRouter pointer_router;

    /*
     * Multi-click semantics use an exact modifier contract. Shift+triple-click is therefore not
     * reinterpreted as select-all. Content begins at x=1; x=3 is the 'c' cell and maps to caret
     * boundary 3. Existing Shift behavior keeps anchor 1 and moves only the active end, selecting
     * "bc". Because this is an ordinary character-granular gesture, capture remains active normally.
     */
    const auto press = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {3, 0},
            PointerAction::press,
            PointerButton::primary,
            3,
            KeyModifier::shift});

    CHECK(press.targeted);
    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(pointer_router.capturedWidget() == &field);
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 3);
    CHECK(field.selectedText() == "bc");

    const auto release = TerminalTextFieldPointerSelection::route(
        window,
        pointer_router,
        PointerEvent{
            {3, 0},
            PointerAction::release,
            PointerButton::primary,
            3,
            KeyModifier::shift});

    CHECK(release.handled);
    CHECK(!release.capture_active);
    CHECK(!pointer_router.hasCapture());
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 3);
    CHECK(field.selectedText() == "bc");
}
