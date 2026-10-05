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
     * Pointer press is a selection transaction only. The exact row identity comes from terminal presentation
     * geometry, but the backend-neutral controller verifies the live MenuModel before retaining index 1.
     * Command activation is intentionally deferred to a later press/release policy, so merely selecting Save
     * cannot enter application callback code.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        primaryPress(popupRowPoint(*frame, 0U, 1U)));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::state_changed);
    CHECK(controller.isActive());
    CHECK(controller.popupOpen());
    CHECK(controller.popupSelection() == std::optional<std::size_t>{1U});
    CHECK(executions == 0);
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
