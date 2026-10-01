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

TEST_CASE("Popup menu key interpretation moves selection without owning state") {
    Command first{"First"};
    Command second{"Second"};
    MenuModel menu{"Actions"};
    menu.appendCommand(first);
    menu.appendSeparator();
    menu.appendCommand(second);

    const auto down = interpretMenuPopupKey(
        menu, std::nullopt, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(down.action == MenuPopupKeyAction::select);
    CHECK(down.selection == std::optional<std::size_t>{0});

    const auto next = interpretMenuPopupKey(
        menu, down.selection, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(next.action == MenuPopupKeyAction::select);
    CHECK(next.selection == std::optional<std::size_t>{2});

    const auto up = interpretMenuPopupKey(
        menu, next.selection, KeyEvent{Key::up, true, KeyModifier::none});
    CHECK(up.action == MenuPopupKeyAction::select);
    CHECK(up.selection == std::optional<std::size_t>{0});
}

TEST_CASE("Popup menu Home and End select semantic edges") {
    Command first{"First"};
    Command last{"Last"};
    MenuModel menu{"Actions"};
    menu.appendCommand(first);
    menu.appendSeparator();
    menu.appendCommand(last);

    const auto end = interpretMenuPopupKey(
        menu, std::size_t{0}, KeyEvent{Key::end, true, KeyModifier::none});
    CHECK(end.action == MenuPopupKeyAction::select);
    CHECK(end.selection == std::optional<std::size_t>{2});

    const auto home = interpretMenuPopupKey(
        menu, std::size_t{2}, KeyEvent{Key::home, true, KeyModifier::none});
    CHECK(home.action == MenuPopupKeyAction::select);
    CHECK(home.selection == std::optional<std::size_t>{0});
}

TEST_CASE("Popup menu Enter distinguishes commands from submenus without executing") {
    Command run{"Run"};
    int executions = 0;
    run.setOnExecuted([&executions] { ++executions; });

    MenuModel menu{"Actions"};
    menu.appendCommand(run);
    (void)menu.appendSubmenu("More");

    const auto command_result = interpretMenuPopupKey(
        menu, std::size_t{0}, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(command_result.action == MenuPopupKeyAction::activate_command);
    CHECK(command_result.selection == std::optional<std::size_t>{0});
    CHECK(executions == 0);

    /*
     * The interpreter returns semantic intent only. The presenter/controller can first dismiss or
     * stabilize popup state and then call MenuItem::activate(), avoiding client callbacks while it is
     * halfway through its own routing transaction.
     */
    CHECK(menu.itemAt(0).activate());
    CHECK(executions == 1);

    const auto submenu_result = interpretMenuPopupKey(
        menu, std::size_t{1}, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(submenu_result.action == MenuPopupKeyAction::open_submenu);
    CHECK(submenu_result.selection == std::optional<std::size_t>{1});
}

TEST_CASE("Popup menu Right only requests submenu entry") {
    Command run{"Run"};
    MenuModel menu{"Actions"};
    menu.appendCommand(run);
    (void)menu.appendSubmenu("More");

    const auto command_result = interpretMenuPopupKey(
        menu, std::size_t{0}, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(command_result.action == MenuPopupKeyAction::none);
    CHECK(command_result.selection == std::optional<std::size_t>{0});

    const auto submenu_result = interpretMenuPopupKey(
        menu, std::size_t{1}, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(submenu_result.action == MenuPopupKeyAction::open_submenu);
    CHECK(submenu_result.selection == std::optional<std::size_t>{1});
}

TEST_CASE("Popup menu Left and Escape request closing only the current menu level") {
    Command command{"One"};
    MenuModel menu{"Actions"};
    menu.appendCommand(command);

    const auto left = interpretMenuPopupKey(
        menu, std::size_t{0}, KeyEvent{Key::left, true, KeyModifier::none});
    CHECK(left.action == MenuPopupKeyAction::close_menu);
    CHECK(left.selection == std::optional<std::size_t>{0});

    const auto escape = interpretMenuPopupKey(
        menu, std::size_t{0}, KeyEvent{Key::escape, true, KeyModifier::none});
    CHECK(escape.action == MenuPopupKeyAction::close_menu);
    CHECK(escape.selection == std::optional<std::size_t>{0});
}

TEST_CASE("Popup menu key interpretation leaves releases and modified gestures unhandled") {
    Command command{"One"};
    MenuModel menu{"Actions"};
    menu.appendCommand(command);

    const auto release = interpretMenuPopupKey(
        menu, std::size_t{0}, KeyEvent{Key::down, false, KeyModifier::none});
    CHECK(release.action == MenuPopupKeyAction::none);
    CHECK(release.selection == std::optional<std::size_t>{0});

    const auto modified = interpretMenuPopupKey(
        menu, std::size_t{0}, KeyEvent{Key::enter, true, KeyModifier::control});
    CHECK(modified.action == MenuPopupKeyAction::none);
    CHECK(modified.selection == std::optional<std::size_t>{0});
}

TEST_CASE("Popup menu key interpretation sanitizes stale non-navigation selection") {
    Command command{"One"};
    MenuModel menu{"Actions"};
    menu.appendCommand(command);

    const auto stale_enter = interpretMenuPopupKey(
        menu, std::size_t{99}, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(stale_enter.action == MenuPopupKeyAction::none);
    CHECK(!stale_enter.selection.has_value());

    const auto recovered = interpretMenuPopupKey(
        menu, std::size_t{99}, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(recovered.action == MenuPopupKeyAction::select);
    CHECK(recovered.selection == std::optional<std::size_t>{0});
}
