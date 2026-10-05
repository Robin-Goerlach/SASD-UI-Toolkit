#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_frame_builder.hpp>
#include <sasd/ui/terminal/menu_pointer_interaction.hpp>

#include <optional>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

namespace {

[[nodiscard]] PointerEvent primaryPress(Point position) {
    return PointerEvent{
        position,
        PointerAction::press,
        PointerButton::primary,
        1,
        KeyModifier::none,
    };
}

} // namespace

TEST_CASE("Terminal menu pointer press opens a top-level root popup without preselecting a row") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);
    (void)bar.appendMenu("Help");

    MenuInteractionController controller;
    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());
    CHECK(!controller.isActive());

    /*
     * The first title is rendered as " File ", so x=1 lies on its visible title surface. Pointer entry is
     * intentionally equivalent to selecting the title and pressing Enter: the root popup opens, but no row
     * is selected yet. Down/hover/click row selection remains a later semantic layer.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress({1, 0}));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::state_changed);
    CHECK(controller.isActive());
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{0U});
    CHECK(controller.popupOpen());
    CHECK(controller.popupDepth() == 1U);
    CHECK(!controller.popupSelection().has_value());
}

TEST_CASE("Terminal menu pointer press on another title collapses nested popup state to that root") {
    Command nested_action{"Nested"};
    Command edit_action{"Edit action"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(nested_action);
    MenuModel& edit = bar.appendMenu("Edit");
    edit.appendCommand(edit_action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar, 0U));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(controller.popupDepth() == 2U);

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());

    /*
     * "File" owns six cells [0,6), and "Edit" owns the next six [6,12). A press on Edit starts a fresh
     * top-level transaction: any old submenu path and popup selection belong to File and must not be carried
     * across by index into an unrelated menu.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress({7, 0}));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::state_changed);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{1U});
    CHECK(controller.popupOpen());
    CHECK(controller.popupDepth() == 1U);
    CHECK(!controller.popupSelection().has_value());
}

TEST_CASE("Terminal menu pointer press on an active popup row is consumed without click-through") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 1U);

    const Point row_point{
        frame->popups[0].origin.x + 1,
        frame->popups[0].origin.y,
    };

    /*
     * Popup-row activation is deliberately deferred. Even so, the menu overlay already owns this cell, so
     * the press must be consumed rather than falling through to an application Button/TextField underneath.
     * No semantic state changes in this slice.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress(row_point));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::none);
    CHECK(controller.isActive());
    CHECK(controller.popupOpen());
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
}

TEST_CASE("Terminal menu pointer outside press dismisses an active menu and remains consumed") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());

    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress({30, 8}));

    /*
     * The same press both dismisses the transient menu and terminates at the menu layer. Returning an
     * engaged result is what lets a host distinguish this from an ordinary application-surface press and
     * prevents one physical click from also activating the control that was covered by the menu.
     */
    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::closed);
    CHECK(!controller.isActive());
    CHECK(!controller.popupOpen());
}

TEST_CASE("Terminal menu pointer outside press stays available when menu interaction is inactive") {
    MenuBarModel bar;
    (void)bar.appendMenu("File");
    MenuInteractionController controller;

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());

    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress({30, 8}));

    CHECK(!result.has_value());
    CHECK(!controller.isActive());
}

TEST_CASE("Terminal menu pointer keeps active motion and release inside the modal menu scope") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());

    const PointerEvent move{{25, 7}, PointerAction::move, PointerButton::none, 0, KeyModifier::none};
    const PointerEvent release{{25, 7}, PointerAction::release, PointerButton::primary, 1, KeyModifier::none};

    const auto move_result =
        TerminalMenuPointerInteraction::handle(bar, controller, *frame, move);
    const auto release_result =
        TerminalMenuPointerInteraction::handle(bar, controller, *frame, release);

    CHECK(move_result.has_value());
    CHECK(move_result->action == MenuInteractionAction::none);
    CHECK(release_result.has_value());
    CHECK(release_result->action == MenuInteractionAction::none);
    CHECK(controller.isActive());
}

TEST_CASE("Terminal menu pointer gives painted popup rows precedence over overlapping menu-bar titles") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);
    (void)bar.appendMenu("Edit");

    MenuInteractionController controller;
    CHECK(controller.begin(bar, 0U));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 1U);

    /*
     * Normal placement starts the root below the bar. Move the owned presentation snapshot upward only for
     * this regression so its first popup row overlaps the File title. Rendering paints popups after the bar;
     * pointer ownership must follow that same z-order and therefore consume the popup row without reopening
     * or otherwise changing the top-level title state.
     */
    frame->popups[0].origin = {0, 0};

    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress({1, 0}));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::none);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{0U});
    CHECK(controller.popupOpen());
}
