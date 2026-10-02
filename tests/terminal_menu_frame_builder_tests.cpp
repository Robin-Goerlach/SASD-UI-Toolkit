#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_composition.hpp>
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
     * The root popup is nine cells wide and naturally starts at x=10. In a 20-cell viewport the child
     * command popup is eight cells wide ("Action" plus the two outer padding cells), so it cannot fit at
     * the natural right-hand x=19. The side-preserving fallback therefore opens it immediately to the
     * parent's left at x=10-8=2. The builder must keep that placement decision beside the parent snapshot
     * so frame rendering can draw a '<' marker later.
     *
     * Keeping this expectation derived from the measurement contract matters: popup width belongs to the
     * presentation snapshot, not to the submenu title that opened it. The earlier expectation of x=4
     * accidentally used the wrong child width and therefore tested arithmetic that the renderer never uses.
     */
    const auto frame = buildMenuPresentationFrame(bar, controller, {10, 0}, {20, 8});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 2U);
    CHECK(frame->popups[0].origin == Point{10, 1});
    CHECK(frame->popups[1].origin == Point{2, 1});
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

TEST_CASE("Terminal menu composition preserves the immutable base outside current menu chrome") {
    ScreenBuffer base{{20, 6}};
    base.clear(Cell{U'.'});

    MenuBarModel bar;
    (void)bar.appendMenu("File");
    (void)bar.appendMenu("Help");
    MenuInteractionController controller;

    const auto composed = composeMenuInteractionPresentation(base, bar, controller);
    CHECK(composed.has_value());

    /*
     * The persistent bar overlays row zero, while unrelated application cells remain exactly as supplied
     * by the explicit base frame. The source buffer is const input and must not become hidden mutable state.
     */
    CHECK(composed->at({0, 0}).code_point == U' ');
    CHECK(composed->at({1, 0}).code_point == U'F');
    CHECK(composed->at({0, 2}).code_point == U'.');
    CHECK(base.at({1, 0}).code_point == U'.');
    CHECK(base.at({0, 2}).code_point == U'.');
}

TEST_CASE("Terminal menu composition restores covered base cells when a popup closes") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);

    ScreenBuffer base{{20, 6}};
    base.clear(Cell{U'.'});

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});

    const auto open_frame = composeMenuInteractionPresentation(base, bar, controller);
    CHECK(open_frame.has_value());
    CHECK(open_frame->at({1, 1}).code_point == U'O');

    /*
     * Closing the popup removes its geometry from current interaction state. Composition starts from base
     * again, so the old popup cells are restored without an erase pass or retained damage rectangle.
     */
    controller.reset();
    const auto closed_frame = composeMenuInteractionPresentation(base, bar, controller);
    CHECK(closed_frame.has_value());
    CHECK(closed_frame->at({1, 1}).code_point == U'.');
    CHECK(closed_frame->at({1, 0}).code_point == U'F');
}

TEST_CASE("Terminal menu composition fails closed without modifying the base frame") {
    ScreenBuffer base{{12, 4}};
    base.clear(Cell{U'#'});

    MenuBarModel bar;
    /* Combining marks are intentionally outside the current simple-cell rendering contract. */
    (void)bar.appendMenu("A\xCC\x81");
    MenuInteractionController controller;

    const auto composed = composeMenuInteractionPresentation(base, bar, controller);
    CHECK(!composed.has_value());

    CHECK(base.at({0, 0}).code_point == U'#');
    CHECK(base.at({5, 2}).code_point == U'#');
}

TEST_CASE("Terminal menu composition uses the base size as the popup fitting viewport") {
    Command action{"Action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(action);

    ScreenBuffer base{{20, 8}};
    base.clear(Cell{U'.'});

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});

    const auto composed = composeMenuInteractionPresentation(base, bar, controller, {10, 0});
    CHECK(composed.has_value());

    /*
     * The child cannot fit to the right of the root inside this 20-cell base/viewport, so the existing
     * direction-aware placement policy opens it to the left and the composed parent row carries '<'.
     */
    CHECK(composed->at({17, 1}).code_point == U'<');
    CHECK(composed->at({3, 1}).code_point == U'A');
}

TEST_CASE("Terminal menu composed frame preserves the application caret while menus are inactive") {
    ScreenBuffer base{{20, 6}};
    base.clear(Cell{U'.'});

    MenuBarModel bar;
    (void)bar.appendMenu("File");
    MenuInteractionController controller;

    const std::optional<Point> base_caret{Point{7, 3}};
    const auto composed = composeMenuInteractionFrame(base, base_caret, bar, controller);
    CHECK(composed.has_value());
    CHECK(composed->caret == base_caret);

    /*
     * Cell composition and caret composition are one frame-level result, but an inactive menu must not
     * interfere with the caret request produced by the underlying application presentation.
     */
    CHECK(composed->buffer.at({1, 0}).code_point == U'F');
    CHECK(base.at({7, 3}).code_point == U'.');
}

TEST_CASE("Terminal menu composed frame suppresses and later restores the application caret") {
    ScreenBuffer base{{20, 6}};
    base.clear(Cell{U'.'});

    MenuBarModel bar;
    (void)bar.appendMenu("File");
    MenuInteractionController controller;
    const std::optional<Point> base_caret{Point{5, 2}};

    CHECK(controller.begin(bar));
    const auto active_frame = composeMenuInteractionFrame(base, base_caret, bar, controller);
    CHECK(active_frame.has_value());
    CHECK(!active_frame->caret.has_value());

    /*
     * Menu activation owns keyboard attention only at the presentation level. Resetting the transient
     * controller must therefore reveal the same base caret without any FocusManager/TextField round trip.
     */
    controller.reset();
    const auto restored_frame = composeMenuInteractionFrame(base, base_caret, bar, controller);
    CHECK(restored_frame.has_value());
    CHECK(restored_frame->caret == base_caret);
}

TEST_CASE("Terminal menu composed frame fails closed before publishing caret metadata") {
    ScreenBuffer base{{12, 4}};
    base.clear(Cell{U'#'});

    MenuBarModel bar;
    (void)bar.appendMenu("A\xCC\x81");
    MenuInteractionController controller;
    const std::optional<Point> base_caret{Point{2, 2}};

    const auto composed = composeMenuInteractionFrame(base, base_caret, bar, controller);
    CHECK(!composed.has_value());

    /* The rejected composition remains observational: neither source cells nor source metadata are mutable. */
    CHECK(base.at({2, 2}).code_point == U'#');
    CHECK(base_caret == std::optional<Point>{Point{2, 2}});
}
