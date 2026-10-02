#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_placement.hpp>

#include <limits>
#include <optional>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal root popup natural placement follows padded menu title spans") {
    MenuBarPresentationSnapshot bar;
    bar.titles = {"界", "Edit", "Help"};

    const auto origin = naturalMenuBarPopupOrigin(bar, 1, {3, 2});
    CHECK(origin.has_value());

    /*
     * The first title occupies four cells: one leading pad, the two-cell wide glyph and one trailing pad.
     * The second popup therefore starts four cells to the right of the menu-bar origin and one row below.
     */
    CHECK(*origin == Point{7, 3});
}

TEST_CASE("Terminal root popup natural placement rejects invalid or unrenderable bars") {
    MenuBarPresentationSnapshot bar;
    bar.titles = {"File"};

    CHECK(!naturalMenuBarPopupOrigin(bar, 1).has_value());

    bar.titles = {"File\nEdit"};
    CHECK(!naturalMenuBarPopupOrigin(bar, 0).has_value());
}

TEST_CASE("Terminal root popup natural placement fails closed on coordinate overflow") {
    MenuBarPresentationSnapshot bar;
    bar.titles = {"File", "Help"};

    const Point extreme{std::numeric_limits<Coordinate>::max(), 0};
    CHECK(!naturalMenuBarPopupOrigin(bar, 1, extreme).has_value());

    const Point extreme_y{0, std::numeric_limits<Coordinate>::max()};
    CHECK(!naturalMenuBarPopupOrigin(bar, 0, extreme_y).has_value());
}

TEST_CASE("Terminal submenu natural placement uses measured parent width and anchor row") {
    PositionedMenuPopupPresentationSnapshot parent;
    parent.origin = {4, 5};
    parent.snapshot.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});
    parent.snapshot.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::submenu, "Tools", true, std::nullopt});

    const auto measured = measureMenuPopupPresentation(parent.snapshot);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{9, 2});

    const auto child = naturalSubmenuPopupOrigin(parent, 1);
    CHECK(child.has_value());
    CHECK(*child == Point{13, 6});
}

TEST_CASE("Terminal submenu natural placement rejects non-submenu anchors and invalid parents") {
    PositionedMenuPopupPresentationSnapshot parent;
    parent.snapshot.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});
    CHECK(!naturalSubmenuPopupOrigin(parent, 0).has_value());
    CHECK(!naturalSubmenuPopupOrigin(parent, 1).has_value());

    parent.snapshot.items.clear();
    parent.snapshot.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::submenu, "A\xCC\x81", true, std::nullopt});
    CHECK(!naturalSubmenuPopupOrigin(parent, 0).has_value());
}

TEST_CASE("Terminal submenu natural placement fails closed on coordinate overflow") {
    PositionedMenuPopupPresentationSnapshot parent;
    parent.origin = {std::numeric_limits<Coordinate>::max() - 1, 0};
    parent.snapshot.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::submenu, "Tools", true, std::nullopt});

    CHECK(!naturalSubmenuPopupOrigin(parent, 0).has_value());
}
