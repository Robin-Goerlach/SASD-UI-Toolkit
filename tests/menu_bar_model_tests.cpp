#include "test_framework.hpp"

#include <sasd/ui/menu_bar_navigation.hpp>
#include <sasd/ui/menu_model.hpp>

#include <optional>
#include <stdexcept>

using namespace sasd::ui;

TEST_CASE("MenuBarModel owns top-level menus in insertion order") {
    MenuBarModel bar;

    MenuModel& file = bar.appendMenu("File");
    MenuModel& edit = bar.appendMenu("Edit");
    MenuModel& help = bar.appendMenu("Help");

    CHECK(bar.menuCount() == 3);
    CHECK(bar.menuAt(0).title() == "File");
    CHECK(bar.menuAt(1).title() == "Edit");
    CHECK(bar.menuAt(2).title() == "Help");
    CHECK(&bar.menuAt(0) == &file);
    CHECK(&bar.menuAt(1) == &edit);
    CHECK(&bar.menuAt(2) == &help);
}

TEST_CASE("MenuBarModel keeps earlier MenuModel references stable while it grows") {
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    Command open{"Open"};
    file.appendCommand(open);

    /*
     * appendMenu() intentionally returns a long-lived builder reference. A vector<MenuModel> would
     * make that reference depend on vector capacity and therefore on an unrelated later append. The
     * model stores individually owned MenuModel objects so extending the bar cannot relocate File.
     */
    for (int index = 0; index < 32; ++index) {
        (void)bar.appendMenu("Extra");
    }

    CHECK(&bar.menuAt(0) == &file);
    CHECK(file.title() == "File");
    CHECK(file.itemCount() == 1);
    CHECK(file.itemAt(0).command() == &open);
}

TEST_CASE("MenuBarModel and Command ownership remain independent") {
    MenuBarModel bar;
    Command save{"Save"};
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(save);

    bar.clear();

    /*
     * Clearing menu structure must not touch application semantics. MenuBarModel owns MenuModel
     * objects only; Command ownership remains with the application's Component/object graph.
     */
    CHECK(bar.menuCount() == 0);
    CHECK(save.text() == "Save");
    CHECK(save.isEnabled());
}

TEST_CASE("MenuBarModel exposes mutable and const indexed access with bounds checking") {
    MenuBarModel bar;
    bar.appendMenu("Original");

    bar.menuAt(0).setTitle("Updated");

    const MenuBarModel& const_bar = bar;
    CHECK(const_bar.menuAt(0).title() == "Updated");

    bool mutable_threw = false;
    try {
        (void)bar.menuAt(1);
    } catch (const std::out_of_range&) {
        mutable_threw = true;
    }
    CHECK(mutable_threw);

    bool const_threw = false;
    try {
        (void)const_bar.menuAt(1);
    } catch (const std::out_of_range&) {
        const_threw = true;
    }
    CHECK(const_threw);
}

TEST_CASE("Menu bar navigation enters from edges and wraps cyclically") {
    MenuBarModel bar;
    bar.appendMenu("File");
    bar.appendMenu("Edit");
    bar.appendMenu("Help");

    CHECK(navigateMenuBar(bar, std::nullopt, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{0});
    CHECK(navigateMenuBar(bar, std::nullopt, MenuNavigationDirection::previous) ==
          std::optional<std::size_t>{2});
    CHECK(navigateMenuBar(bar, std::size_t{2}, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{0});
    CHECK(navigateMenuBar(bar, std::size_t{0}, MenuNavigationDirection::previous) ==
          std::optional<std::size_t>{2});
}

TEST_CASE("Menu bar navigation recovers from stale selection and handles an empty bar") {
    MenuBarModel bar;
    bar.appendMenu("File");
    bar.appendMenu("Help");

    /*
     * A presenter may retain a selected top-level index while application code rebuilds the bar. The
     * helper treats an out-of-range index as no current selection, just like the vertical menu helper,
     * so the next deliberate navigation gesture re-enters a known structural edge.
     */
    CHECK(navigateMenuBar(bar, std::size_t{99}, MenuNavigationDirection::next) ==
          std::optional<std::size_t>{0});
    CHECK(navigateMenuBar(bar, std::size_t{99}, MenuNavigationDirection::previous) ==
          std::optional<std::size_t>{1});

    MenuBarModel empty;
    CHECK(!navigateMenuBar(empty, std::nullopt, MenuNavigationDirection::next).has_value());
}

TEST_CASE("Menu bar key interpretation moves selection without owning popup state") {
    MenuBarModel bar;
    bar.appendMenu("File");
    bar.appendMenu("Edit");
    bar.appendMenu("Help");

    const auto right = interpretMenuBarKey(
        bar, std::nullopt, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(right.action == MenuBarKeyAction::select);
    CHECK(right.selection == std::optional<std::size_t>{0});

    const auto next = interpretMenuBarKey(
        bar, right.selection, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(next.action == MenuBarKeyAction::select);
    CHECK(next.selection == std::optional<std::size_t>{1});

    const auto left = interpretMenuBarKey(
        bar, next.selection, KeyEvent{Key::left, true, KeyModifier::none});
    CHECK(left.action == MenuBarKeyAction::select);
    CHECK(left.selection == std::optional<std::size_t>{0});
}

TEST_CASE("Menu bar Home End and open intents are deterministic") {
    MenuBarModel bar;
    bar.appendMenu("File");
    bar.appendMenu("Edit");
    bar.appendMenu("Help");

    const auto end = interpretMenuBarKey(
        bar, std::size_t{0}, KeyEvent{Key::end, true, KeyModifier::none});
    CHECK(end.action == MenuBarKeyAction::select);
    CHECK(end.selection == std::optional<std::size_t>{2});

    const auto home = interpretMenuBarKey(
        bar, end.selection, KeyEvent{Key::home, true, KeyModifier::none});
    CHECK(home.action == MenuBarKeyAction::select);
    CHECK(home.selection == std::optional<std::size_t>{0});

    const auto down = interpretMenuBarKey(
        bar, home.selection, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(down.action == MenuBarKeyAction::open_menu);
    CHECK(down.selection == std::optional<std::size_t>{0});

    const auto enter = interpretMenuBarKey(
        bar, std::size_t{1}, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(enter.action == MenuBarKeyAction::open_menu);
    CHECK(enter.selection == std::optional<std::size_t>{1});
}

TEST_CASE("Menu bar key interpretation leaves modified input and stale direct opens unhandled") {
    MenuBarModel bar;
    bar.appendMenu("File");

    const auto modified = interpretMenuBarKey(
        bar, std::size_t{0}, KeyEvent{Key::right, true, KeyModifier::alt});
    CHECK(modified.action == MenuBarKeyAction::none);
    CHECK(modified.selection == std::optional<std::size_t>{0});

    const auto release = interpretMenuBarKey(
        bar, std::size_t{0}, KeyEvent{Key::right, false, KeyModifier::none});
    CHECK(release.action == MenuBarKeyAction::none);
    CHECK(release.selection == std::optional<std::size_t>{0});

    const auto stale_open = interpretMenuBarKey(
        bar, std::size_t{99}, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(stale_open.action == MenuBarKeyAction::none);
    CHECK(!stale_open.selection.has_value());

    const auto recovered = interpretMenuBarKey(
        bar, std::size_t{99}, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(recovered.action == MenuBarKeyAction::select);
    CHECK(recovered.selection == std::optional<std::size_t>{0});
}

TEST_CASE("Menu bar Escape requests leaving the bar without rewriting selection") {
    MenuBarModel bar;
    bar.appendMenu("File");

    const auto result = interpretMenuBarKey(
        bar, std::size_t{0}, KeyEvent{Key::escape, true, KeyModifier::none});
    CHECK(result.action == MenuBarKeyAction::close_menu_bar);
    CHECK(result.selection == std::optional<std::size_t>{0});
}
