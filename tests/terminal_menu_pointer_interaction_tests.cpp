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

[[nodiscard]] PointerEvent primaryRelease(Point position) {
    return PointerEvent{
        position,
        PointerAction::release,
        PointerButton::primary,
        1,
        KeyModifier::none,
    };
}

[[nodiscard]] Point popupRowPoint(const MenuFramePresentationSnapshot& frame,
                                  std::size_t level,
                                  std::size_t row) {
    return Point{
        frame.popups.at(level).origin.x + 1,
        frame.popups.at(level).origin.y + static_cast<Coordinate>(row),
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
     * is selected yet. Direct row selection is a separate semantic transaction below.
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

TEST_CASE("Terminal menu pointer press selects an enabled popup row without activating its command") {
    Command open{"Open"};
    Command save{"Save"};
    int executions = 0;
    save.setOnExecuted([&] { ++executions; });

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);
    file.appendCommand(save);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(!controller.popupSelection().has_value());

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 1U);

    /*
     * The stateless overload intentionally remains a selection transaction only. The exact row identity
     * comes from terminal presentation geometry, but no press identity is retained across calls, so even a
     * later release on Save cannot enter application callback code. Hosts that want click activation opt into
     * the explicit GestureState overload tested below.
     */
    const Point save_point = popupRowPoint(*frame, 0U, 1U);
    const auto press_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress(save_point));

    CHECK(press_result.has_value());
    CHECK(press_result->action == MenuInteractionAction::state_changed);
    CHECK(controller.isActive());
    CHECK(controller.popupOpen());
    CHECK(controller.popupSelection() == std::optional<std::size_t>{1U});
    CHECK(executions == 0);

    const auto release_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(release_frame.has_value());
    const auto release_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *release_frame,
        primaryRelease(popupRowPoint(*release_frame, 0U, 1U)));

    CHECK(release_result.has_value());
    CHECK(release_result->action == MenuInteractionAction::none);
    CHECK(controller.isActive());
    CHECK(executions == 0);
}

TEST_CASE("Terminal menu pointer stateful matching release returns command only after menu state closes") {
    Command save{"Save"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(save);

    MenuInteractionController controller;
    bool callback_saw_closed_menu = false;
    int executions = 0;
    save.setOnExecuted([&] {
        ++executions;
        callback_saw_closed_menu = !controller.isActive();
    });

    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    TerminalMenuPointerInteraction::GestureState gesture;
    const auto press_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(press_frame.has_value());
    const Point press_point = popupRowPoint(*press_frame, 0U, 0U);

    const auto press_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *press_frame,
        primaryPress(press_point),
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(press_result.has_value());
    CHECK(press_result->action == MenuInteractionAction::state_changed);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
    CHECK(gesture.hasPressedPopupItem());
    CHECK(executions == 0);

    /*
     * Rebuild the frame after press selection. This models the host contract: release hit testing uses the
     * presentation state actually visible after the press, not the pre-press snapshot. Geometry is unchanged
     * here, but the selected-row highlight belongs to the newer transaction.
     */
    const auto release_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(release_frame.has_value());
    const auto release_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *release_frame,
        primaryRelease(popupRowPoint(*release_frame, 0U, 0U)),
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(release_result.has_value());
    CHECK(release_result->action == MenuInteractionAction::activate_command);
    CHECK(!controller.isActive());
    CHECK(!controller.popupOpen());
    CHECK(!gesture.hasPressedPopupItem());

    /*
     * The pointer layer never executes application code. MenuInteractionController captures a lifetime-safe
     * Command::Reference and closes transient state first; the host is free to repaint the closed menu before
     * entering arbitrary client code. Execute manually here to prove the callback observes that invariant.
     */
    CHECK(executions == 0);
    Command* const command = release_result->command.get();
    CHECK(command == &save);
    if (command != nullptr) {
        CHECK(command->execute());
    }
    CHECK(executions == 1);
    CHECK(callback_saw_closed_menu);
}

TEST_CASE("Terminal menu pointer stateful release on another row cancels command activation") {
    Command open{"Open"};
    Command save{"Save"};
    int open_executions = 0;
    int save_executions = 0;
    open.setOnExecuted([&] { ++open_executions; });
    save.setOnExecuted([&] { ++save_executions; });

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);
    file.appendCommand(save);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    TerminalMenuPointerInteraction::GestureState gesture;
    const auto press_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(press_frame.has_value());
    (void)TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *press_frame,
        primaryPress(popupRowPoint(*press_frame, 0U, 0U)),
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
    CHECK(gesture.hasPressedPopupItem());

    const auto release_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(release_frame.has_value());
    const auto release_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *release_frame,
        primaryRelease(popupRowPoint(*release_frame, 0U, 1U)),
        AmbiguousWidthMode::narrow,
        gesture);

    /*
     * Release geometry must match the row armed by press. A different row does not become a surprise second
     * selection or activation target; the original press selection remains visible and the physical click
     * simply completes as a cancelled activation inside the modal menu scope.
     */
    CHECK(release_result.has_value());
    CHECK(release_result->action == MenuInteractionAction::none);
    CHECK(controller.isActive());
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
    CHECK(!gesture.hasPressedPopupItem());
    CHECK(open_executions == 0);
    CHECK(save_executions == 0);
}

TEST_CASE("Terminal menu pointer stateful activation fails closed when selected command changes before release") {
    Command save{"Save"};
    int executions = 0;
    save.setOnExecuted([&] { ++executions; });

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(save);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    TerminalMenuPointerInteraction::GestureState gesture;
    const auto press_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(press_frame.has_value());
    (void)TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *press_frame,
        primaryPress(popupRowPoint(*press_frame, 0U, 0U)),
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});

    /*
     * Application semantics may change between physical press and release. Disabling the selected Command
     * makes the retained selection stale. The release still lands on the same painted row, but the Core
     * activation transaction normalizes first and treats any repair as a reason to refuse activation in the
     * same transaction. A second deliberate user gesture would be required against the repaired model.
     */
    save.setEnabled(false);
    const auto release_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(release_frame.has_value());
    const auto release_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *release_frame,
        primaryRelease(popupRowPoint(*release_frame, 0U, 0U)),
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(release_result.has_value());
    CHECK(release_result->action == MenuInteractionAction::state_changed);
    CHECK(!release_result->command);
    CHECK(controller.isActive());
    CHECK(controller.popupOpen());
    CHECK(!controller.popupSelection().has_value());
    CHECK(!gesture.hasPressedPopupItem());
    CHECK(executions == 0);
}

TEST_CASE("Terminal menu pointer stateful submenu release stays selected without opening the child") {
    Command nested{"Nested"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(nested);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    TerminalMenuPointerInteraction::GestureState gesture;
    const auto press_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(press_frame.has_value());
    (void)TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *press_frame,
        primaryPress(popupRowPoint(*press_frame, 0U, 0U)),
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
    CHECK(controller.popupDepth() == 1U);

    const auto release_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(release_frame.has_value());
    const auto release_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *release_frame,
        primaryRelease(popupRowPoint(*release_frame, 0U, 0U)),
        AmbiguousWidthMode::narrow,
        gesture);

    /*
     * Command activation and submenu opening are separate semantic operations. A matching release over a
     * selected submenu therefore remains consumed but does not synthesize Right/Enter and does not open the
     * child yet. The next menu-pointer slice can define submenu gesture policy independently.
     */
    CHECK(release_result.has_value());
    CHECK(release_result->action == MenuInteractionAction::none);
    CHECK(controller.isActive());
    CHECK(controller.popupOpen());
    CHECK(controller.popupDepth() == 1U);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
    CHECK(!gesture.hasPressedPopupItem());
}

TEST_CASE("Terminal menu pointer consumes separator and disabled rows without selecting them") {
    Command open{"Open"};
    Command disabled{"Disabled"};
    disabled.setEnabled(false);

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);
    file.appendSeparator();
    file.appendCommand(disabled);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());

    /*
     * Geometry still identifies unavailable rows because they are painted menu surface. Semantic selection
     * uses MenuItem::isEnabled(), so both a separator and a disabled command are rejected without changing
     * the existing selection. The engaged no-op result is nevertheless essential: unavailable menu chrome
     * is not a transparent hole through which an application Widget may receive the same physical press.
     */
    const auto separator_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress(popupRowPoint(*frame, 0U, 1U)));
    CHECK(separator_result.has_value());
    CHECK(separator_result->action == MenuInteractionAction::none);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});

    const auto disabled_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress(popupRowPoint(*frame, 0U, 2U)));
    CHECK(disabled_result.has_value());
    CHECK(disabled_result->action == MenuInteractionAction::none);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
}

TEST_CASE("Terminal menu pointer ancestor selection closes popup descendants owned by another row") {
    Command open{"Open"};
    Command nested{"Nested"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);            // root row 0
    MenuModel& tools = file.appendSubmenu("Tools"); // root row 1
    tools.appendCommand(nested);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none}); // Open
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none}); // Tools
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{1U}});

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {60, 12});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 2U);

    /*
     * The child popup is structurally owned by root row 1 (Tools). Selecting root row 0 must therefore
     * close that child before row 0 becomes the deepest selection; retaining the old child would describe
     * a popup whose parent is no longer selected. This invariant lives in MenuInteractionController rather
     * than in terminal geometry so every future backend gets the same safe state transition.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress(popupRowPoint(*frame, 0U, 0U)));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 1U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
}

TEST_CASE("Terminal menu pointer reselecting the submenu that owns a child preserves that child") {
    Command nested{"Nested"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(nested);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {60, 12});
    CHECK(frame.has_value());

    /*
     * The same ancestor row still semantically owns the already-open child, so a press that merely reselects
     * it need not destroy valid descendant state. This is also an important distinction from selecting a
     * different ancestor row in the preceding regression.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress(popupRowPoint(*frame, 0U, 0U)));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
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
     * pointer ownership must follow that same z-order. The popup row is therefore selected. If title hit
     * testing incorrectly won instead, begin()+Enter would reopen File and leave the root with no selection.
     */
    frame->popups[0].origin = {0, 0};

    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress({1, 0}));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::state_changed);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{0U});
    CHECK(controller.popupOpen());
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
}
