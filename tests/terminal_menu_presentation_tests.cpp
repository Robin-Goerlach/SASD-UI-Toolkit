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
