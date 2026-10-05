#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_frame_builder.hpp>
#include <sasd/ui/terminal/menu_hover_interaction.hpp>
#include <sasd/ui/terminal/menu_pointer_interaction.hpp>

#include <chrono>
#include <optional>
#include <stdexcept>

using namespace std::chrono_literals;
using namespace sasd::ui;
using namespace sasd::ui::terminal;

namespace {

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

TEST_CASE("Terminal submenu hover opens only after one stable row reaches its delay") {
    Command nested{"Nested"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(nested);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    TerminalMenuHoverOptions options;
    options.submenu_open_delay = 100ms;
    TerminalMenuHoverInteraction hover{options};
    const TerminalMenuHoverInteraction::TimePoint t0{};

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {50, 12});
    CHECK(frame.has_value());
    const PointerEvent first_move = pointerMove(popupRowPoint(*frame, 0U, 0U));

    /*
     * Ordinary pointer interaction still owns immediate row selection. The hover companion sees the same
     * already-processed event afterwards and adds only timing; it does not duplicate selectability or path
     * mutation rules. Tools is therefore selected immediately but its child stays closed until the deadline.
     */
    const auto first_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        first_move);
    CHECK(first_result.has_value());
    CHECK(first_result->action == MenuInteractionAction::state_changed);
    hover.observe(controller, *frame, first_move, AmbiguousWidthMode::narrow, t0);
    CHECK(hover.hasPendingCandidate());
    CHECK(controller.popupDepth() == 1U);

    /*
     * A second all-motion report inside the same painted row must not restart the timer. Real terminals can
     * emit many reports while the pointer drifts horizontally; resetting on every report could prevent an
     * otherwise stable hover from ever reaching its deadline.
     */
    const auto same_row_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {50, 12});
    CHECK(same_row_frame.has_value());
    const PointerEvent same_row_move = pointerMove(popupRowPoint(*same_row_frame, 0U, 0U));
    const auto same_row_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *same_row_frame,
        same_row_move);
    CHECK(same_row_result.has_value());
    hover.observe(
        controller,
        *same_row_frame,
        same_row_move,
        AmbiguousWidthMode::narrow,
        t0 + 80ms);

    const MenuInteractionResult early = hover.advance(bar, controller, t0 + 99ms);
    CHECK(early.action == MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 1U);
    CHECK(hover.hasPendingCandidate());

    const MenuInteractionResult opened = hover.advance(bar, controller, t0 + 100ms);
    CHECK(opened.action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
    CHECK(!controller.popupSelection().has_value());
    CHECK(!hover.hasPendingCandidate());
}

TEST_CASE("Terminal submenu hover restarts its delay when the pointer selects another row") {
    Command tool_action{"Tool action"};
    Command view_action{"View action"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(tool_action);
    MenuModel& views = file.appendSubmenu("Views");
    views.appendCommand(view_action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    TerminalMenuHoverOptions options;
    options.submenu_open_delay = 100ms;
    TerminalMenuHoverInteraction hover{options};
    const TerminalMenuHoverInteraction::TimePoint t0{};

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {50, 12});
    CHECK(frame.has_value());

    const PointerEvent tools_move = pointerMove(popupRowPoint(*frame, 0U, 0U));
    (void)TerminalMenuPointerInteraction::handle(bar, controller, *frame, tools_move);
    hover.observe(controller, *frame, tools_move, AmbiguousWidthMode::narrow, t0);

    const PointerEvent views_move = pointerMove(popupRowPoint(*frame, 0U, 1U));
    (void)TerminalMenuPointerInteraction::handle(bar, controller, *frame, views_move);
    hover.observe(controller, *frame, views_move, AmbiguousWidthMode::narrow, t0 + 80ms);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{1U});

    /* The old Tools deadline must no longer be able to open anything after the candidate changed. */
    const MenuInteractionResult old_deadline = hover.advance(bar, controller, t0 + 100ms);
    CHECK(old_deadline.action == MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 1U);

    const MenuInteractionResult new_deadline = hover.advance(bar, controller, t0 + 180ms);
    CHECK(new_deadline.action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{1U}});
}

TEST_CASE("Terminal submenu hover cancels when pointer motion leaves popup rows") {
    Command nested{"Nested"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(nested);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    TerminalMenuHoverOptions options;
    options.submenu_open_delay = 50ms;
    TerminalMenuHoverInteraction hover{options};
    const TerminalMenuHoverInteraction::TimePoint t0{};

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {50, 12});
    CHECK(frame.has_value());
    const PointerEvent inside = pointerMove(popupRowPoint(*frame, 0U, 0U));
    (void)TerminalMenuPointerInteraction::handle(bar, controller, *frame, inside);
    hover.observe(controller, *frame, inside, AmbiguousWidthMode::narrow, t0);
    CHECK(hover.hasPendingCandidate());

    const PointerEvent outside = pointerMove({49, 11});
    (void)TerminalMenuPointerInteraction::handle(bar, controller, *frame, outside);
    hover.observe(controller, *frame, outside, AmbiguousWidthMode::narrow, t0 + 20ms);
    CHECK(!hover.hasPendingCandidate());

    const MenuInteractionResult result = hover.advance(bar, controller, t0 + 100ms);
    CHECK(result.action == MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 1U);
}

TEST_CASE("Terminal submenu hover cannot reinterpret a candidate after top-level menu switch") {
    Command file_action{"File child"};
    Command edit_action{"Edit child"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(file_action);
    MenuModel& edit = bar.appendMenu("Edit");
    MenuModel& options_menu = edit.appendSubmenu("Options");
    options_menu.appendCommand(edit_action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar, 0U));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});

    TerminalMenuHoverOptions options;
    options.submenu_open_delay = 50ms;
    TerminalMenuHoverInteraction hover{options};
    const TerminalMenuHoverInteraction::TimePoint t0{};

    const auto file_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {50, 12});
    CHECK(file_frame.has_value());
    const PointerEvent file_move = pointerMove(popupRowPoint(*file_frame, 0U, 0U));
    (void)TerminalMenuPointerInteraction::handle(bar, controller, *file_frame, file_move);
    hover.observe(controller, *file_frame, file_move, AmbiguousWidthMode::narrow, t0);
    CHECK(hover.hasPendingCandidate());

    /*
     * Both roots deliberately place a submenu at row zero. The pending value {level=0,item=0} is therefore
     * numerically reusable, but its meaning changed from File/Tools to Edit/Options. Root identity stored with
     * the candidate must prevent that collision from opening the new submenu at the old deadline.
     */
    CHECK(controller.begin(bar, 1U));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{1U});
    CHECK(controller.popupDepth() == 1U);

    const MenuInteractionResult result = hover.advance(bar, controller, t0 + 50ms);
    CHECK(result.action == MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 1U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(!hover.hasPendingCandidate());
}

TEST_CASE("Terminal submenu hover cannot cross a changed nested owner path with equal row indices") {
    Command first_leaf{"First leaf"};
    Command second_leaf{"Second leaf"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    MenuModel& advanced = tools.appendSubmenu("Advanced");
    advanced.appendCommand(first_leaf);
    MenuModel& other = file.appendSubmenu("Other");
    MenuModel& other_advanced = other.appendSubmenu("Advanced");
    other_advanced.appendCommand(second_leaf);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(controller.selectPopupItem(bar, 0U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 0U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});

    TerminalMenuHoverOptions options;
    options.submenu_open_delay = 50ms;
    TerminalMenuHoverInteraction hover{options};
    const TerminalMenuHoverInteraction::TimePoint t0{};

    const auto tools_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {70, 14});
    CHECK(tools_frame.has_value());
    CHECK(tools_frame->popups.size() == 2U);
    const PointerEvent advanced_move = pointerMove(popupRowPoint(*tools_frame, 1U, 0U));
    (void)TerminalMenuPointerInteraction::handle(bar, controller, *tools_frame, advanced_move);
    hover.observe(controller, *tools_frame, advanced_move, AmbiguousWidthMode::narrow, t0);
    CHECK(hover.hasPendingCandidate());

    /*
     * Replace the level-one owner from root row zero (Tools) with root row one (Other). Both child menus have
     * a submenu at row zero, so level/item alone still collides. The retained owner-path prefix must reject the
     * stale candidate rather than opening Other/Advanced from a hover that began under Tools.
     */
    CHECK(controller.selectPopupItem(bar, 0U, 1U).action == MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 0U, 1U).action == MenuInteractionAction::state_changed);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{1U}});
    CHECK(controller.popupDepth() == 2U);

    const MenuInteractionResult result = hover.advance(bar, controller, t0 + 50ms);
    CHECK(result.action == MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{1U}});
    CHECK(!hover.hasPendingCandidate());
}

TEST_CASE("Terminal submenu hover rejects negative delay configuration") {
    TerminalMenuHoverOptions options;
    options.submenu_open_delay = -1ms;

    bool threw = false;
    try {
        TerminalMenuHoverInteraction hover{options};
        (void)hover;
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
}
