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

[[nodiscard]] PointerEvent pointerMove(Point position) {
    return PointerEvent{
        position,
        PointerAction::move,
        PointerButton::none,
        0,
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

TEST_CASE("Terminal menu pointer motion selects an enabled popup row without completing it") {
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
    CHECK(controller.popupOpen());
    CHECK(!controller.popupSelection().has_value());

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());

    /*
     * Motion translates geometry into the existing Core selectPopupItem() transaction. The terminal adapter
     * neither executes Commands nor invents submenu-entry semantics here. That separation is important for a
     * passive-hover transport mode: receiving more motion reports must not silently turn mere movement into
     * activation.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        pointerMove(popupRowPoint(*frame, 0U, 1U)));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::state_changed);
    CHECK(controller.isActive());
    CHECK(controller.popupOpen());
    CHECK(controller.popupDepth() == 1U);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{1U});
    CHECK(executions == 0);
}

TEST_CASE("Terminal menu pointer motion over unavailable rows stays consumed without changing selection") {
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
     * Painted separator/disabled rows still belong to the modal menu surface, but Core's selectability
     * contract rejects both identities. Returning an engaged no-op preserves modality while keeping the last
     * valid row selected instead of treating unavailable chrome as either a transparent hole or a new target.
     */
    const auto separator_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        pointerMove(popupRowPoint(*frame, 0U, 1U)));
    CHECK(separator_result.has_value());
    CHECK(separator_result->action == MenuInteractionAction::none);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});

    const auto disabled_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        pointerMove(popupRowPoint(*frame, 0U, 2U)));
    CHECK(disabled_result.has_value());
    CHECK(disabled_result->action == MenuInteractionAction::none);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
}

TEST_CASE("Terminal menu pointer motion to another ancestor row closes descendants it no longer owns") {
    Command open{"Open"};
    Command nested{"Nested"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);                         // root row 0
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
     * Motion itself knows nothing about MenuPath ownership. It supplies root row zero to Core; the existing
     * selectPopupItem() invariant then recognizes that the open child belongs to row one and truncates the
     * descendant path. This keeps the same structural rule for keyboard-, press-, and motion-driven selection.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        pointerMove(popupRowPoint(*frame, 0U, 0U)));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 1U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
}

TEST_CASE("Terminal menu pointer motion does not retarget an armed click completion") {
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

    const auto press_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *press_frame,
        primaryPress(popupRowPoint(*press_frame, 0U, 0U)),
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(press_result.has_value());
    CHECK(press_result->action == MenuInteractionAction::state_changed);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
    CHECK(gesture.hasPressedPopupItem());

    const auto move_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *press_frame,
        pointerMove(popupRowPoint(*press_frame, 0U, 1U)),
        AmbiguousWidthMode::narrow,
        gesture);

    CHECK(move_result.has_value());
    CHECK(move_result->action == MenuInteractionAction::state_changed);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{1U});
    CHECK(gesture.hasPressedPopupItem());

    /*
     * GestureState still remembers the physical press on row zero; motion only changed semantic selection.
     * Releasing back on the geometrically armed row must therefore fail Core's current-selection proof rather
     * than unexpectedly activating either Open or the newly selected Save. The release retires the armed
     * identity, leaving the menu open with Save selected for the next deliberate gesture.
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
    CHECK(release_result->action == MenuInteractionAction::none);
    CHECK(controller.isActive());
    CHECK(controller.popupSelection() == std::optional<std::size_t>{1U});
    CHECK(!gesture.hasPressedPopupItem());
    CHECK(open_executions == 0);
    CHECK(save_executions == 0);
}

TEST_CASE("Terminal menu pointer motion across top-level titles switches the open root popup") {
    Command file_action{"File action"};
    Command edit_action{"Edit action"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(file_action);
    MenuModel& edit = bar.appendMenu("Edit");
    edit.appendCommand(edit_action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar, 0U));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{0U});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());

    /*
     * Each title includes one leading and trailing cell. "File" therefore occupies [0,6) and "Edit"
     * occupies [6,12); x=7 is visibly inside Edit. With all-motion tracking enabled by a host, this ordinary
     * no-button move must replace File's root popup with Edit's root popup immediately. Selection indices from
     * File are deliberately not carried across because the same number would refer to unrelated semantics.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        pointerMove({7, 0}));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::state_changed);
    CHECK(controller.isActive());
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{1U});
    CHECK(controller.popupOpen());
    CHECK(controller.popupDepth() == 1U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(!controller.popupSelection().has_value());
}

TEST_CASE("Terminal menu pointer motion over the already active title preserves open descendants") {
    Command nested{"Nested"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(nested);
    (void)bar.appendMenu("Edit");

    MenuInteractionController controller;
    CHECK(controller.begin(bar, 0U));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});
    (void)controller.handleKey(bar, KeyEvent{Key::right, true, KeyModifier::none});
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {50, 12});
    CHECK(frame.has_value());

    /*
     * Crossing the title that already owns the open popup is not a semantic menu switch. Reopening it through
     * begin()+Enter would discard the valid Tools child merely because the pointer crossed row zero. The motion
     * adapter therefore consumes the title hit as a no-op and preserves the entire existing descendant path.
     */
    const auto result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        pointerMove({1, 0}));

    CHECK(result.has_value());
    CHECK(result->action == MenuInteractionAction::none);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{0U});
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
}

TEST_CASE("Terminal menu pointer title switch retires an armed popup identity before numeric indices can be reused") {
    Command open{"Open"};
    Command save{"Save"};
    int open_executions = 0;
    int save_executions = 0;
    open.setOnExecuted([&] { ++open_executions; });
    save.setOnExecuted([&] { ++save_executions; });

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open); // level 0, row 0 in File
    MenuModel& edit = bar.appendMenu("Edit");
    edit.appendCommand(save); // level 0, row 0 in Edit as well

    MenuInteractionController controller;
    CHECK(controller.begin(bar, 0U));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    TerminalMenuPointerInteraction::GestureState gesture;
    const auto file_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(file_frame.has_value());

    const auto press_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *file_frame,
        primaryPress(popupRowPoint(*file_frame, 0U, 0U)),
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(press_result.has_value());
    CHECK(gesture.hasPressedPopupItem());

    /*
     * Motion to Edit replaces the root MenuModel. The old armed value {level=0,row=0} is numerically valid in
     * Edit too, but it must never be reinterpreted as Save. Switching top-level semantic scope therefore
     * retires the physical press identity before the new root popup is opened.
     */
    const auto switch_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *file_frame,
        pointerMove({7, 0}),
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(switch_result.has_value());
    CHECK(switch_result->action == MenuInteractionAction::state_changed);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{1U});
    CHECK(!gesture.hasPressedPopupItem());

    const auto edit_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(edit_frame.has_value());

    const auto select_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *edit_frame,
        pointerMove(popupRowPoint(*edit_frame, 0U, 0U)),
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(select_result.has_value());
    CHECK(select_result->action == MenuInteractionAction::state_changed);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});

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
    CHECK(release_result->action == MenuInteractionAction::none);
    CHECK(controller.isActive());
    CHECK(open_executions == 0);
    CHECK(save_executions == 0);
}
