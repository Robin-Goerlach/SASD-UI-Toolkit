#include "test_framework.hpp"

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
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::command,
        "Open",
        true,
        Shortcut{Key::f1, KeyModifier::control},
    });
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::separator,
        {},
        false,
        std::nullopt,
    });
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::submenu,
        "Tools",
        true,
        std::nullopt,
    });

    const auto measured = measureMenuPopupPresentation(snapshot);
    CHECK(measured.has_value());

    /*
     * "Open" + outer padding = 6 cells. The two-cell accelerator gap plus "Ctrl+F1" adds nine more,
     * making 15 cells. "Tools" plus outer padding and " >" needs only nine, so the command row wins.
     */
    CHECK(measured->size == Size{15, 3});
}

TEST_CASE("Terminal popup measurement honors wide Unicode cells") {
    MenuPopupPresentationSnapshot snapshot;
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::command,
        "界",
        true,
        std::nullopt,
    });

    const auto measured = measureMenuPopupPresentation(snapshot);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{4, 1});
}

TEST_CASE("Terminal menu measurement fails closed for multiline and zero-width text") {
    MenuBarPresentationSnapshot bar;
    bar.titles = {"File\nEdit"};
    CHECK(!measureMenuBarPresentation(bar).has_value());

    MenuPopupPresentationSnapshot popup;
    popup.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::command,
        "A\xCC\x81",
        true,
        std::nullopt,
    });
    CHECK(!measureMenuPopupPresentation(popup).has_value());
}

TEST_CASE("Terminal empty popup keeps minimal chrome width without inventing rows") {
    MenuPopupPresentationSnapshot snapshot;

    const auto measured = measureMenuPopupPresentation(snapshot);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{3, 0});
}

TEST_CASE("Terminal popup renderer paints rows separators shortcuts and submenu markers") {
    ScreenBuffer buffer{{24, 6}, Cell{U'.'}};

    MenuPopupPresentationSnapshot snapshot;
    snapshot.selection = std::size_t{0};
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::command,
        "Open",
        true,
        Shortcut{Key::f1, KeyModifier::control},
    });
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::separator,
        {},
        false,
        std::nullopt,
    });
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::submenu,
        "Tools",
        true,
        std::nullopt,
    });

    CHECK(renderMenuPopupPresentation(buffer, {2, 1}, snapshot));

    /*
     * The measured popup is 15 cells wide. The command's accelerator is right-aligned before the final
     * padding cell, while the separator expands across the same geometry and the submenu marker occupies
     * the penultimate cell. Nothing outside the popup rectangle is cleared as a side effect.
     */
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

    /* Selection styling covers padding and gap cells, not only glyph cells. */
    CHECK(buffer.at({2, 1}).style.inverse);
    CHECK(buffer.at({8, 1}).style.inverse);
    CHECK(buffer.at({16, 1}).style.inverse);
    CHECK(!buffer.at({3, 3}).style.inverse);
}

TEST_CASE("Terminal popup renderer dims disabled commands without changing semantic text") {
    ScreenBuffer buffer{{20, 3}};
    MenuPopupPresentationSnapshot snapshot;
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::command,
        "Unavailable",
        false,
        std::nullopt,
    });

    CHECK(renderMenuPopupPresentation(buffer, {0, 0}, snapshot));
    CHECK(buffer.at({1, 0}).code_point == U'U');
    CHECK(buffer.at({1, 0}).style.dim);
    CHECK(!buffer.at({1, 0}).style.inverse);
}

TEST_CASE("Terminal popup renderer preserves wide-cell invariants under clipping") {
    ScreenBuffer buffer{{3, 2}, Cell{U'.'}};
    MenuPopupPresentationSnapshot snapshot;
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::command,
        "界",
        true,
        std::nullopt,
    });

    /*
     * The glyph would start at x=-1: its lead is clipped while its continuation would fall at x=0.
     * A partial wide glyph must not be emitted. Row clearing still occurs because the snapshot itself is
     * valid, leaving ordinary blank cells rather than a synthetic wide continuation at the visible edge.
     */
    CHECK(renderMenuPopupPresentation(buffer, {-2, 0}, snapshot));
    CHECK(buffer.at({0, 0}).code_point == U' ');
    CHECK(buffer.at({0, 0}).role == CellRole::normal);
    CHECK(buffer.at({1, 0}).code_point == U' ');
    CHECK(buffer.at({1, 0}).role == CellRole::normal);
}

TEST_CASE("Terminal popup renderer leaves the previous frame untouched when preflight fails") {
    ScreenBuffer buffer{{8, 2}, Cell{U'Z'}};
    MenuPopupPresentationSnapshot snapshot;
    snapshot.items.push_back(MenuItemPresentationSnapshot{
        MenuItemKind::command,
        "A\xCC\x81",
        true,
        std::nullopt,
    });

    CHECK(!renderMenuPopupPresentation(buffer, {0, 0}, snapshot));

    /*
     * Validation happens before any row clear. Unsupported combining semantics therefore defer the new
     * representation without destroying the last successfully rendered terminal frame.
     */
    CHECK(buffer.at({0, 0}).code_point == U'Z');
    CHECK(buffer.at({7, 1}).code_point == U'Z');
}
