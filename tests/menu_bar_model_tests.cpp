#include "test_framework.hpp"

#include <sasd/ui/menu_interaction_controller.hpp>
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

TEST_CASE("Menu interaction begins explicitly and opens a root popup without implicit selection") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);
    (void)bar.appendMenu("Help");

    MenuInteractionController controller;
    CHECK(!controller.isActive());
    CHECK(controller.begin(bar));
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{0});
    CHECK(!controller.popupOpen());

    const auto open_result = controller.handleKey(
        bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(open_result.action == MenuInteractionAction::state_changed);
    CHECK(controller.popupOpen());
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(controller.popupDepth() == 1);
    CHECK(!controller.popupSelection().has_value());

    const auto select_result = controller.handleKey(
        bar, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(select_result.action == MenuInteractionAction::state_changed);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0});
}

TEST_CASE("Menu interaction opens root popup at semantic edge for vertical arrows") {
    Command disabled{"Disabled"};
    disabled.setEnabled(false);
    Command first{"First"};
    Command last{"Last"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(disabled);
    file.appendSeparator();
    file.appendCommand(first);
    file.appendSeparator();
    file.appendCommand(last);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));

    /*
     * Down from the active menu bar opens the root popup and immediately lands on the first selectable
     * semantic item. The controller deliberately delegates scanning to navigateMenu(), so disabled
     * commands and separators are skipped by exactly the same rule used after the popup is already open.
     */
    const auto down = controller.handleKey(
        bar, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(down.action == MenuInteractionAction::state_changed);
    CHECK(controller.popupOpen());
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{2});

    controller.reset();
    CHECK(controller.begin(bar));

    /*
     * Up is the symmetric entry gesture: open the same root popup but start at the last selectable item.
     * This behavior intentionally lives in the stateful controller because it coordinates menu-bar open
     * intent with popup selection state; the stateless menu-bar interpreter only reports open_menu.
     */
    const auto up = controller.handleKey(
        bar, KeyEvent{Key::up, true, KeyModifier::none});
    CHECK(up.action == MenuInteractionAction::state_changed);
    CHECK(controller.popupOpen());
    CHECK(controller.popupSelection() == std::optional<std::size_t>{4});

    const auto up_intent = interpretMenuBarKey(
        bar, std::size_t{0}, KeyEvent{Key::up, true, KeyModifier::none});
    CHECK(up_intent.action == MenuBarKeyAction::open_menu);
    CHECK(up_intent.selection == std::optional<std::size_t>{0});
}

TEST_CASE("Menu interaction vertical opening tolerates a popup with no selectable item") {
    Command disabled{"Disabled"};
    disabled.setEnabled(false);

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendSeparator();
    file.appendCommand(disabled);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));

    const auto down = controller.handleKey(
        bar, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(down.action == MenuInteractionAction::state_changed);
    CHECK(controller.popupOpen());
    CHECK(controller.popupDepth() == 1);
    CHECK(!controller.popupSelection().has_value());
}

TEST_CASE("Menu interaction composes popup navigation with nested MenuPath state") {
    Command advanced_action{"Advanced action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(advanced_action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});

    const auto enter_submenu = controller.handleKey(
        bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(enter_submenu.action == MenuInteractionAction::state_changed);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0}});
    CHECK(controller.popupDepth() == 2);
    CHECK(!controller.popupSelection().has_value());

    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0});
}

TEST_CASE("Menu interaction switches top-level menus while the root popup stays open") {
    Command open{"Open"};
    Command copy{"Copy"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& edit = bar.appendMenu("Edit");
    file.appendCommand(open);
    edit.appendCommand(copy);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0});

    /*
     * A root-popup Right gesture on a command crosses to the next top-level menu instead of being
     * swallowed by popup-local navigation. The old item index is deliberately discarded because index
     * zero in File has no semantic relationship to index zero in Edit.
     */
    const auto next_menu = controller.handleKey(
        bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(next_menu.action == MenuInteractionAction::state_changed);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{1});
    CHECK(controller.popupOpen());
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(controller.popupDepth() == 1);
    CHECK(!controller.popupSelection().has_value());

    const auto previous_menu = controller.handleKey(
        bar, KeyEvent{Key::left, true, KeyModifier::none});
    CHECK(previous_menu.action == MenuInteractionAction::state_changed);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{0});
    CHECK(controller.popupOpen());
    CHECK(!controller.popupSelection().has_value());
}

TEST_CASE("Menu interaction gives selected root submenus precedence over Right menu switching") {
    Command action{"Action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(action);
    (void)bar.appendMenu("Help");

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});

    const auto enter_submenu = controller.handleKey(
        bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(enter_submenu.action == MenuInteractionAction::state_changed);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{0});
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0}});
    CHECK(controller.popupDepth() == 2);

    /*
     * Once inside a child popup, Left is again local parent navigation. Only after returning to the
     * root popup does another Left participate in horizontal top-level switching.
     */
    const auto close_child = controller.handleKey(
        bar, KeyEvent{Key::left, true, KeyModifier::none});
    CHECK(close_child.action == MenuInteractionAction::state_changed);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{0});
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(controller.popupDepth() == 1);

    const auto switch_previous = controller.handleKey(
        bar, KeyEvent{Key::left, true, KeyModifier::none});
    CHECK(switch_previous.action == MenuInteractionAction::state_changed);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{1});
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(!controller.popupSelection().has_value());
}

TEST_CASE("Menu interaction closes state before returning a command activation request") {
    Command run{"Run"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(run);

    MenuInteractionController controller;
    bool callback_saw_closed_state = false;
    int executions = 0;
    run.setOnExecuted([&] {
        ++executions;
        callback_saw_closed_state = !controller.isActive();
    });

    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});

    const auto activation = controller.handleKey(
        bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(activation.action == MenuInteractionAction::activate_command);
    CHECK(!controller.isActive());
    CHECK(executions == 0);

    Command* const command = activation.command.get();
    CHECK(command == &run);
    if (command != nullptr) {
        CHECK(command->execute());
    }
    CHECK(executions == 1);
    CHECK(callback_saw_closed_state);
}

TEST_CASE("Menu interaction closes nested popup levels one at a time before leaving the bar") {
    Command action{"Action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(controller.popupDepth() == 2);

    const auto close_child = controller.handleKey(
        bar, KeyEvent{Key::escape, true, KeyModifier::none});
    CHECK(close_child.action == MenuInteractionAction::state_changed);
    CHECK(controller.popupOpen());
    CHECK(controller.popupDepth() == 1);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});

    const auto close_root = controller.handleKey(
        bar, KeyEvent{Key::escape, true, KeyModifier::none});
    CHECK(close_root.action == MenuInteractionAction::state_changed);
    CHECK(controller.isActive());
    CHECK(!controller.popupOpen());

    const auto leave_bar = controller.handleKey(
        bar, KeyEvent{Key::escape, true, KeyModifier::none});
    CHECK(leave_bar.action == MenuInteractionAction::closed);
    CHECK(!controller.isActive());
}

TEST_CASE("Menu interaction conservatively normalizes stale popup state after model mutation") {
    Command action{"Action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0}});

    /*
     * Application code may rebuild a menu while a popup is open. The controller stores only value
     * indices, so the next transaction can truncate the invalid route and clear stale selection rather
     * than dereferencing a destroyed submenu object or guessing a replacement item.
     */
    file.clear();
    const auto normalized = controller.handleKey(
        bar, KeyEvent{Key::space, true, KeyModifier::none});
    CHECK(normalized.action == MenuInteractionAction::state_changed);
    CHECK(controller.isActive());
    CHECK(controller.popupOpen());
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(controller.popupDepth() == 1);
    CHECK(!controller.popupSelection().has_value());

    bar.clear();
    const auto invalid_root = controller.handleKey(
        bar, KeyEvent{Key::space, true, KeyModifier::none});
    CHECK(invalid_root.action == MenuInteractionAction::state_changed);
    CHECK(!controller.isActive());
}
