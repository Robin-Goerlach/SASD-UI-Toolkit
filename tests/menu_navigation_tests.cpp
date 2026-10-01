#include "test_framework.hpp"

#include <sasd/ui/menu_navigation.hpp>

#include <memory>
#include <optional>

using namespace sasd::ui;

TEST_CASE("Menu navigation starts at semantic edges and wraps") {
    Command first{"First"};
    Command second{"Second"};
    MenuModel menu{"Actions"};
    menu.appendCommand(first);
    menu.appendSeparator();
    menu.appendCommand(second);

    CHECK(navigateMenu(menu, std::nullopt, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{0});
    CHECK(navigateMenu(menu, std::nullopt, MenuNavigationDirection::previous) ==
          std::optional<std::size_t>{2});
    CHECK(navigateMenu(menu, std::size_t{0}, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{2});
    CHECK(navigateMenu(menu, std::size_t{2}, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{0});
    CHECK(navigateMenu(menu, std::size_t{0}, MenuNavigationDirection::previous) ==
          std::optional<std::size_t>{2});
}

TEST_CASE("Menu navigation skips disabled and expired commands") {
    Command enabled{"Enabled"};
    Command disabled{"Disabled"};
    disabled.setEnabled(false);

    auto expired = std::make_unique<Command>("Expired");

    MenuModel menu{"Mixed"};
    menu.appendCommand(disabled);
    menu.appendCommand(*expired);
    menu.appendCommand(enabled);
    expired.reset();

    /*
     * The semantic menu model already converts command lifetime/state into MenuItem::isEnabled().
     * Navigation consumes exactly that contract instead of duplicating Command-specific lifetime
     * checks, keeping selection policy independent from how an item obtains its enabled state.
     */
    CHECK(navigateMenu(menu, std::nullopt, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{2});
    CHECK(navigateMenu(menu, std::size_t{2}, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{2});
}

TEST_CASE("Menu navigation treats submenus as selectable structural targets") {
    Command disabled{"Unavailable"};
    disabled.setEnabled(false);

    MenuModel menu{"Root"};
    menu.appendCommand(disabled);
    MenuModel& nested = menu.appendSubmenu("Tools");
    (void)nested;

    CHECK(navigateMenu(menu, std::nullopt, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{1});
    CHECK(menu.itemAt(1).kind() == MenuItemKind::submenu);
}

TEST_CASE("Menu navigation safely recovers from a stale selection index") {
    Command first{"First"};
    Command last{"Last"};
    MenuModel menu{"Mutable"};
    menu.appendCommand(first);
    menu.appendSeparator();
    menu.appendCommand(last);

    /*
     * Presentation code may retain an index while application code rebuilds a menu. An out-of-range
     * index is therefore deliberately interpreted as "no current selection" rather than dereferenced
     * or rejected. The next user gesture deterministically re-enters the valid menu range.
     */
    CHECK(navigateMenu(menu, std::size_t{99}, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{0});
    CHECK(navigateMenu(menu, std::size_t{99}, MenuNavigationDirection::previous) ==
          std::optional<std::size_t>{2});
}

TEST_CASE("Menu navigation returns no selection when no item is selectable") {
    Command disabled{"Disabled"};
    disabled.setEnabled(false);

    MenuModel menu{"Unavailable"};
    menu.appendSeparator();
    menu.appendCommand(disabled);

    CHECK(!navigateMenu(menu, std::nullopt, MenuNavigationDirection::next).has_value());
    CHECK(!navigateMenu(menu, std::nullopt, MenuNavigationDirection::previous).has_value());

    MenuModel empty{"Empty"};
    CHECK(!navigateMenu(empty, std::nullopt, MenuNavigationDirection::next).has_value());
}
