#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_placement.hpp>
#include <sasd/ui/terminal/menu_viewport_placement.hpp>

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

TEST_CASE("Terminal viewport fitting preserves a naturally visible popup origin") {
    MenuPopupPresentationSnapshot popup;
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});

    const auto fitted = fitMenuPopupOriginToViewport(popup, {5, 3}, {20, 10});
    CHECK(fitted.has_value());
    CHECK(*fitted == Point{5, 3});
}

TEST_CASE("Terminal viewport fitting shifts right and bottom overflow inward") {
    MenuPopupPresentationSnapshot popup;
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});

    const auto measured = measureMenuPopupPresentation(popup);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{6, 1});

    /*
     * A six-cell popup in a ten-cell viewport can start no farther right than x=4. Its one-row height in
     * a four-row viewport similarly makes y=3 the final fully visible row. The fitting policy moves only
     * the overflowing axes; it does not reinterpret menu ancestry or interaction state.
     */
    const auto fitted = fitMenuPopupOriginToViewport(popup, {8, 4}, {10, 4});
    CHECK(fitted.has_value());
    CHECK(*fitted == Point{4, 3});
}

TEST_CASE("Terminal viewport fitting clamps negative natural origins to the viewport") {
    MenuPopupPresentationSnapshot popup;
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});

    const auto fitted = fitMenuPopupOriginToViewport(popup, {-12, -7}, {20, 10});
    CHECK(fitted.has_value());
    CHECK(*fitted == Point{0, 0});
}

TEST_CASE("Terminal viewport fitting rejects popups that cannot fit completely") {
    MenuPopupPresentationSnapshot too_wide;
    too_wide.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Unavailable", true, std::nullopt});
    CHECK(!fitMenuPopupOriginToViewport(too_wide, {0, 0}, {12, 5}).has_value());

    MenuPopupPresentationSnapshot too_tall;
    too_tall.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "One", true, std::nullopt});
    too_tall.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Two", true, std::nullopt});
    CHECK(!fitMenuPopupOriginToViewport(too_tall, {0, 0}, {20, 1}).has_value());
}

TEST_CASE("Terminal viewport fitting rejects malformed viewports and unrenderable popup text") {
    MenuPopupPresentationSnapshot popup;
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});

    CHECK(!fitMenuPopupOriginToViewport(popup, {0, 0}, {-1, 10}).has_value());
    CHECK(!fitMenuPopupOriginToViewport(popup, {0, 0}, {20, -1}).has_value());

    popup.items.clear();
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "A\xCC\x81", true, std::nullopt});
    CHECK(!fitMenuPopupOriginToViewport(popup, {0, 0}, {20, 10}).has_value());
}

TEST_CASE("Terminal viewport fitting permits an empty popup on the bottom edge") {
    MenuPopupPresentationSnapshot popup;

    const auto measured = measureMenuPopupPresentation(popup);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{3, 0});

    const auto fitted = fitMenuPopupOriginToViewport(popup, {9, 8}, {10, 4});
    CHECK(fitted.has_value());
    CHECK(*fitted == Point{7, 4});
}
