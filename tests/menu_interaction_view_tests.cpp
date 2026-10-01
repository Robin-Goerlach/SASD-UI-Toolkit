#include "test_framework.hpp"

#include <sasd/ui/menu_interaction_view.hpp>

#include <optional>

using namespace sasd::ui;

TEST_CASE("Menu interaction view exposes every open popup level without retaining model pointers") {
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
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});

    CHECK(controller.popupDepth() == 2);

    const auto root = menuPopupLevelView(bar, controller, 0);
    CHECK(root.has_value());
    CHECK(root->menu == &file);
    CHECK(root->selection == std::optional<std::size_t>{0});

    const auto child = menuPopupLevelView(bar, controller, 1);
    CHECK(child.has_value());
    CHECK(child->menu == &tools);
    CHECK(child->selection == std::optional<std::size_t>{0});

    CHECK(!menuPopupLevelView(bar, controller, 2).has_value());
}

TEST_CASE("Menu interaction view fails closed when an open submenu path becomes stale") {
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

    /*
     * Presentation may run after application code has structurally rebuilt a menu but before the next
     * input transaction gives MenuInteractionController a chance to normalize its retained indices.
     * The view helper must therefore re-resolve the path rather than trust the controller's old depth.
     */
    file.clear();

    const auto root = menuPopupLevelView(bar, controller, 0);
    CHECK(root.has_value());
    CHECK(root->menu == &file);
    CHECK(!root->selection.has_value());

    CHECK(!menuPopupLevelView(bar, controller, 1).has_value());

    /* Observation is side-effect free; controller repair remains explicit on its next transaction. */
    CHECK(controller.popupDepth() == 2);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0}});
}

TEST_CASE("Menu interaction view clears a deepest selection that becomes unavailable") {
    Command action{"Action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0});

    action.setEnabled(false);

    const auto view = menuPopupLevelView(bar, controller, 0);
    CHECK(view.has_value());
    CHECK(view->menu == &file);
    CHECK(!view->selection.has_value());

    /* The controller keeps value state until its next explicit normalization transaction. */
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0});
}

TEST_CASE("Menu interaction view rejects inactive and closed popup state") {
    MenuBarModel bar;
    (void)bar.appendMenu("File");

    MenuInteractionController controller;
    CHECK(!menuPopupLevelView(bar, controller, 0).has_value());

    CHECK(controller.begin(bar));
    CHECK(!menuPopupLevelView(bar, controller, 0).has_value());

    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(menuPopupLevelView(bar, controller, 0).has_value());

    controller.reset();
    CHECK(!menuPopupLevelView(bar, controller, 0).has_value());
}
