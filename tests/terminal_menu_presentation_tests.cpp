#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_bar_presentation.hpp>
#include <sasd/ui/terminal/menu_frame_presentation.hpp>
#include <sasd/ui/terminal/menu_presentation.hpp>

#include <optional>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal menu bar measurement uses terminal cell widths and per-title padding") {
    MenuBarPresentationSnapshot snapshot;
    snapshot.titles = {"File", "Help"};

    const auto measured = measureMenuBarPresentation(snapshot);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{12, 1});
}

TEST_CASE("Terminal popup measurement reserves shortcut and submenu indicator columns") {
    MenuPopupPresentationSnapshot snapshot;
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true,
                                                           Shortcut{Key::f1, KeyModifier::control}});
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::separator, {}, false, std::nullopt});
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::submenu, "Tools", true, std::nullopt});

    const auto measured = measureMenuPopupPresentation(snapshot);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{15, 3});
}

TEST_CASE("Terminal popup measurement honors wide Unicode cells") {
    MenuPopupPresentationSnapshot snapshot;
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::command, "界", true, std::nullopt});

    const auto measured = measureMenuPopupPresentation(snapshot);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{4, 1});
}

TEST_CASE("Terminal menu measurement fails closed for multiline and zero-width text") {
    MenuBarPresentationSnapshot bar;
    bar.titles = {"File\nEdit"};
    CHECK(!measureMenuBarPresentation(bar).has_value());

    MenuPopupPresentationSnapshot popup;
    popup.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::command, "A\xCC\x81", true, std::nullopt});
    CHECK(!measureMenuPopupPresentation(popup).has_value());
}

TEST_CASE("Terminal empty popup keeps minimal chrome width without inventing rows") {
    MenuPopupPresentationSnapshot snapshot;

    const auto measured = measureMenuPopupPresentation(snapshot);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{3, 0});
}

TEST_CASE("Terminal menu bar renderer paints padded titles and selected span") {
    ScreenBuffer buffer{{24, 3}};
    buffer.clear(Cell{U'.'});

    MenuBarPresentationSnapshot snapshot;
    snapshot.titles = {"File", "Help"};
    snapshot.selection = std::size_t{1};

    CHECK(renderMenuBarPresentation(buffer, {2, 1}, snapshot));

    CHECK(buffer.at({1, 1}).code_point == U'.');
    CHECK(buffer.at({2, 1}).code_point == U' ');
    CHECK(buffer.at({3, 1}).code_point == U'F');
    CHECK(buffer.at({6, 1}).code_point == U'e');
    CHECK(buffer.at({7, 1}).code_point == U' ');
    CHECK(buffer.at({8, 1}).code_point == U' ');
    CHECK(buffer.at({9, 1}).code_point == U'H');
    CHECK(buffer.at({12, 1}).code_point == U'p');
    CHECK(buffer.at({13, 1}).code_point == U' ');
    CHECK(buffer.at({14, 1}).code_point == U'.');

    /* Selection styling covers the complete padded title, not only the glyph cells. */
    CHECK(!buffer.at({3, 1}).style.inverse);
    CHECK(buffer.at({8, 1}).style.inverse);
    CHECK(buffer.at({10, 1}).style.inverse);
    CHECK(buffer.at({13, 1}).style.inverse);
}

TEST_CASE("Terminal menu bar renderer clips wide glyphs without orphaned cells") {
    ScreenBuffer buffer{{2, 2}};
    buffer.clear(Cell{U'.'});

    MenuBarPresentationSnapshot snapshot;
    snapshot.titles = {"界"};

    /*
     * The logical title span is four cells: leading padding, the two-cell glyph and trailing padding.
     * With origin x=-2 the wide glyph would occupy x=-1..0, so it must be omitted atomically. The two
     * visible cells are still part of the successfully rendered title span: x=0 is the cleared glyph
     * continuation slot and x=1 is the trailing padding cell. Both must therefore contain ordinary blanks.
     */
    CHECK(renderMenuBarPresentation(buffer, {-2, 0}, snapshot));
    CHECK(buffer.at({0, 0}).code_point == U' ');
    CHECK(buffer.at({0, 0}).role == CellRole::normal);
    CHECK(buffer.at({1, 0}).code_point == U' ');
    CHECK(buffer.at({1, 0}).role == CellRole::normal);
}

TEST_CASE("Terminal menu bar renderer leaves previous frame untouched when preflight fails") {
    ScreenBuffer buffer{{10, 2}};
    buffer.clear(Cell{U'Z'});

    MenuBarPresentationSnapshot snapshot;
    snapshot.titles = {"A\xCC\x81"};

    CHECK(!renderMenuBarPresentation(buffer, {0, 0}, snapshot));
    CHECK(buffer.at({0, 0}).code_point == U'Z');
    CHECK(buffer.at({9, 1}).code_point == U'Z');
}

TEST_CASE("Terminal popup renderer paints rows separators shortcuts and submenu markers") {
    ScreenBuffer buffer{{24, 6}};
    buffer.clear(Cell{U'.'});

    MenuPopupPresentationSnapshot snapshot;
    snapshot.selection = std::size_t{0};
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true,
                                                           Shortcut{Key::f1, KeyModifier::control}});
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::separator, {}, false, std::nullopt});
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::submenu, "Tools", true, std::nullopt});

    CHECK(renderMenuPopupPresentation(buffer, {2, 1}, snapshot));

    CHECK(buffer.at({1, 1}).code_point == U'.');
    CHECK(buffer.at({2, 1}).code_point == U' ');
    CHECK(buffer.at({3, 1}).code_point == U'O');
    CHECK(buffer.at({6, 1}).code_point == U'n');
    CHECK(buffer.at({9, 1}).code_point == U'C');
    CHECK(buffer.at({15, 1}).code_point == U'1');
    CHECK(buffer.at({16, 1}).code_point == U' ');
    CHECK(buffer.at({17, 1}).code_point == U'.');

    CHECK(buffer.at({2, 2}).code_point == U' ');
    CHECK(buffer.at({3, 2}).code_point == U'-');
    CHECK(buffer.at({15, 2}).code_point == U'-');
    CHECK(buffer.at({16, 2}).code_point == U' ');

    CHECK(buffer.at({3, 3}).code_point == U'T');
    CHECK(buffer.at({7, 3}).code_point == U's');
    CHECK(buffer.at({15, 3}).code_point == U'>');

    CHECK(buffer.at({2, 1}).style.inverse);
    CHECK(buffer.at({8, 1}).style.inverse);
    CHECK(buffer.at({16, 1}).style.inverse);
    CHECK(!buffer.at({3, 3}).style.inverse);
}

TEST_CASE("Terminal popup renderer dims disabled commands without changing semantic text") {
    ScreenBuffer buffer{{20, 3}};
    MenuPopupPresentationSnapshot snapshot;
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::command, "Unavailable", false,
                                                           std::nullopt});

    CHECK(renderMenuPopupPresentation(buffer, {0, 0}, snapshot));
    CHECK(buffer.at({1, 0}).code_point == U'U');
    CHECK(buffer.at({1, 0}).style.dim);
    CHECK(!buffer.at({1, 0}).style.inverse);
}

TEST_CASE("Terminal popup renderer preserves wide-cell invariants under clipping") {
    ScreenBuffer buffer{{3, 2}};
    buffer.clear(Cell{U'.'});
    MenuPopupPresentationSnapshot snapshot;
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::command, "界", true, std::nullopt});

    CHECK(renderMenuPopupPresentation(buffer, {-2, 0}, snapshot));
    CHECK(buffer.at({0, 0}).code_point == U' ');
    CHECK(buffer.at({0, 0}).role == CellRole::normal);
    CHECK(buffer.at({1, 0}).code_point == U' ');
    CHECK(buffer.at({1, 0}).role == CellRole::normal);
}

TEST_CASE("Terminal popup renderer leaves the previous frame untouched when preflight fails") {
    ScreenBuffer buffer{{8, 2}};
    buffer.clear(Cell{U'Z'});
    MenuPopupPresentationSnapshot snapshot;
    snapshot.items.push_back(MenuItemPresentationSnapshot{MenuItemKind::command, "A\xCC\x81", true,
                                                           std::nullopt});

    CHECK(!renderMenuPopupPresentation(buffer, {0, 0}, snapshot));
    CHECK(buffer.at({0, 0}).code_point == U'Z');
    CHECK(buffer.at({7, 1}).code_point == U'Z');
}

TEST_CASE("Terminal menu frame renderer composes bar and popup layers") {
    ScreenBuffer buffer{{24, 5}};
    buffer.clear(Cell{U'.'});

    MenuFramePresentationSnapshot frame;
    frame.menu_bar_origin = {1, 0};
    frame.menu_bar.titles = {"File", "Help"};
    frame.menu_bar.selection = std::size_t{0};

    PositionedMenuPopupPresentationSnapshot popup;
    popup.origin = {1, 1};
    popup.snapshot.selection = std::size_t{0};
    popup.snapshot.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});
    frame.popups.push_back(std::move(popup));

    CHECK(renderMenuPresentationFrame(buffer, frame));

    CHECK(buffer.at({2, 0}).code_point == U'F');
    CHECK(buffer.at({1, 0}).style.inverse);
    CHECK(buffer.at({2, 1}).code_point == U'O');
    CHECK(buffer.at({1, 1}).style.inverse);
    CHECK(buffer.at({0, 0}).code_point == U'.');
}

TEST_CASE("Terminal menu frame renderer preflights every layer before mutating the buffer") {
    ScreenBuffer buffer{{12, 4}};
    buffer.clear(Cell{U'Z'});

    MenuFramePresentationSnapshot frame;
    frame.menu_bar.titles = {"File"};

    PositionedMenuPopupPresentationSnapshot invalid_popup;
    invalid_popup.origin = {0, 1};
    invalid_popup.snapshot.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "A\xCC\x81", true, std::nullopt});
    frame.popups.push_back(std::move(invalid_popup));

    /*
     * The menu bar is individually renderable, but the complete frame is not. Frame-level preflight must
     * therefore reject the transaction before even the valid bar clears cells from the previous frame.
     */
    CHECK(!renderMenuPresentationFrame(buffer, frame));
    CHECK(buffer.at({0, 0}).code_point == U'Z');
    CHECK(buffer.at({0, 1}).code_point == U'Z');
    CHECK(buffer.at({11, 3}).code_point == U'Z');
}

TEST_CASE("Terminal menu frame renderer paints later popup layers last") {
    ScreenBuffer buffer{{12, 4}};
    buffer.clear(Cell{U'.'});

    MenuFramePresentationSnapshot frame;
    frame.menu_bar.titles = {"File"};

    PositionedMenuPopupPresentationSnapshot first;
    first.origin = {0, 1};
    first.snapshot.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "First", true, std::nullopt});
    frame.popups.push_back(std::move(first));

    PositionedMenuPopupPresentationSnapshot second;
    second.origin = {0, 1};
    second.snapshot.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Second", true, std::nullopt});
    frame.popups.push_back(std::move(second));

    CHECK(renderMenuPresentationFrame(buffer, frame));

    /* Later layers define the visible result where transient popup rectangles overlap. */
    CHECK(buffer.at({1, 1}).code_point == U'S');
    CHECK(buffer.at({6, 1}).code_point == U'd');
}
