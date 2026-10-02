#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_frame_builder.hpp>

#include <optional>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal menu frame builder keeps inactive interaction to the persistent menu bar") {
    MenuBarModel bar;
    (void)bar.appendMenu("File");
    (void)bar.appendMenu("Help");
    MenuInteractionController controller;

    const auto frame = buildMenuPresentationFrame(bar, controller, {2, 1}, {30, 8});
    CHECK(frame.has_value());
    CHECK(frame->menu_bar_origin == Point{2, 1});
    CHECK(frame->menu_bar.titles.size() == 2U);
    CHECK(frame->popups.empty());
}

TEST_CASE("Terminal menu frame builder snapshots and fits one open root popup") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});

    const auto frame = buildMenuPresentationFrame(bar, controller, {3, 1}, {20, 8});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 1U);
    CHECK(frame->popups[0].origin == Point{3, 2});
    CHECK(frame->popups[0].snapshot.selection == std::optional<std::size_t>{0});
    CHECK(!frame->popups[0].active_submenu_direction.has_value());
}

TEST_CASE("Terminal menu frame builder carries flipped child direction into the parent layer") {
    Command action{"Action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(controller.popupDepth() == 2U);

    /*
     * The root popup is nine cells wide and naturally starts at x=10. In a 20-cell viewport the six-cell
     * child cannot fit at x=19, but does fit immediately to the parent's left at x=4. The builder must keep
     * that placement decision beside the parent snapshot so frame rendering can draw a '<' marker later.
     */
    const auto frame = buildMenuPresentationFrame(bar, controller, {10, 0}, {20, 8});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 2U);
    CHECK(frame->popups[0].origin == Point{10, 1});
    CHECK(frame->popups[1].origin == Point{4, 1});
    CHECK(frame->popups[0].active_submenu_direction.has_value());
    CHECK(frame->popups[0].active_submenu_direction->item_index == 0U);
    CHECK(frame->popups[0].active_submenu_direction->side == SubmenuPopupSide::left);
    CHECK(!frame->popups[1].active_submenu_direction.has_value());
}

TEST_CASE("Terminal menu frame builder fails closed when an open submenu path becomes stale") {
    Command action{"Action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(controller.popupDepth() == 2U);

    file.clear();

    CHECK(!buildMenuPresentationFrame(bar, controller, {0, 0}, {20, 8}).has_value());
    CHECK(controller.popupDepth() == 2U);
}

TEST_CASE("Terminal menu frame builder rejects a child that cannot fit beside its parent") {
    Command action{"Action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});

    /*
     * The root can be fitted into this viewport, but the child cannot fit completely on either side of it.
     * The builder must not weaken submenu placement by silently overlapping the parent.
     */
    CHECK(!buildMenuPresentationFrame(bar, controller, {0, 0}, {12, 8}).has_value());
}
