#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_frame_builder.hpp>
#include <sasd/ui/terminal/menu_interaction_presentation.hpp>

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

TEST_CASE("Terminal menu interaction presentation renders the persistent bar without active interaction") {
    MenuBarModel bar;
    (void)bar.appendMenu("File");
    (void)bar.appendMenu("Help");
    MenuInteractionController controller;

    ScreenBuffer buffer{{24, 5}};
    buffer.clear(Cell{U'.'});

    CHECK(renderMenuInteractionPresentation(buffer, bar, controller, {1, 1}));

    /*
     * The convenience boundary must not invent a separate layout contract. It uses the same one-cell
     * padding on each side of a title as the standalone menu-bar renderer, so "File" starts at x=2 when
     * the bar origin is x=1. Cells outside the painted title span remain untouched.
     */
    CHECK(buffer.at({1, 1}).code_point == U' ');
    CHECK(buffer.at({2, 1}).code_point == U'F');
    CHECK(buffer.at({5, 1}).code_point == U'e');
    CHECK(buffer.at({6, 1}).code_point == U' ');
    CHECK(buffer.at({0, 1}).code_point == U'.');
}

TEST_CASE("Terminal menu interaction presentation carries fitted submenu direction through to cells") {
    Command action{"Action"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});

    ScreenBuffer buffer{{20, 8}};
    buffer.clear(Cell{U'.'});

    CHECK(renderMenuInteractionPresentation(buffer, bar, controller, {10, 0}));

    /*
     * This is intentionally an end-to-end presentation assertion rather than another placement-unit test.
     * The builder must choose the left side, attach that decision to the parent frame layer, and the frame
     * renderer must finally turn it into '<'. The child command itself begins at x=3 because its popup is
     * fitted to x=2 and owns one leading padding cell.
     */
    CHECK(buffer.at({17, 1}).code_point == U'<');
    CHECK(buffer.at({17, 1}).style.inverse);
    CHECK(buffer.at({3, 1}).code_point == U'A');
}

TEST_CASE("Terminal menu interaction presentation leaves the previous frame intact for stale interaction") {
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

    ScreenBuffer buffer{{20, 6}};
    buffer.clear(Cell{U'Z'});

    /*
     * Presentation can race semantically with ordinary application-side menu rebuilding even in a single-
     * threaded event loop: mutation may happen after one input transaction and before the next controller
     * normalization. The orchestration helper must therefore fail before painting any part of the new frame.
     */
    file.clear();

    CHECK(!renderMenuInteractionPresentation(buffer, bar, controller, {0, 0}));
    CHECK(buffer.at({0, 0}).code_point == U'Z');
    CHECK(buffer.at({19, 5}).code_point == U'Z');
    CHECK(controller.popupDepth() == 2U);
}

TEST_CASE("Terminal menu interaction presentation preserves renderer fail-closed text semantics") {
    MenuBarModel bar;
    (void)bar.appendMenu("A\xCC\x81");
    MenuInteractionController controller;

    ScreenBuffer buffer{{12, 3}};
    buffer.clear(Cell{U'Q'});

    /*
     * A bar-only frame can be constructed without needing popup geometry, so unsupported combining text is
     * finally rejected by renderMenuPresentationFrame(). The convenience boundary must preserve that
     * transaction: no title cell may be cleared before the renderer has completed its representability
     * preflight.
     */
    CHECK(!renderMenuInteractionPresentation(buffer, bar, controller));
    CHECK(buffer.at({0, 0}).code_point == U'Q');
    CHECK(buffer.at({11, 2}).code_point == U'Q');
}
