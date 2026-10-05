#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_close_interaction.hpp>
#include <sasd/ui/terminal/menu_frame_builder.hpp>
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

TEST_CASE("Terminal submenu close grace collapses descendants only after the outside delay") {
    Command leaf{"Leaf"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    MenuModel& advanced = tools.appendSubmenu("Advanced");
    advanced.appendCommand(leaf);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(controller.selectPopupItem(bar, 0U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 0U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.selectPopupItem(bar, 1U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 1U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 3U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U, 0U}});

    TerminalMenuCloseOptions options;
    options.submenu_close_delay = 100ms;
    TerminalMenuCloseInteraction close_grace{options};
    const TerminalMenuCloseInteraction::TimePoint t0{};

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {80, 20});
    CHECK(frame.has_value());

    /*
     * The point is deliberately inside the viewport but outside every popup. Immediate menu interaction keeps
     * the coherent child route open; the close companion adds only a grace period for this geometric gap.
     */
    const PointerEvent outside = pointerMove({79, 19});
    const auto pointer_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        *frame,
        outside);
    CHECK(pointer_result.has_value());
    CHECK(pointer_result->action == MenuInteractionAction::none);
    close_grace.observe(
        controller,
        *frame,
        outside,
        AmbiguousWidthMode::narrow,
        t0);
    CHECK(close_grace.hasPendingCandidate());

    /*
     * A dense second all-motion report in the same outside region must not restart the close deadline. The
     * original t0 remains authoritative so leaving the popup chain continuously cannot keep descendants alive
     * forever merely because the terminal emits more motion packets.
     */
    close_grace.observe(
        controller,
        *frame,
        outside,
        AmbiguousWidthMode::narrow,
        t0 + 80ms);

    const MenuInteractionResult early = close_grace.advance(bar, controller, t0 + 99ms);
    CHECK(early.action == MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 3U);

    const MenuInteractionResult closed = close_grace.advance(bar, controller, t0 + 100ms);
    CHECK(closed.action == MenuInteractionAction::state_changed);
    CHECK(controller.isActive());
    CHECK(controller.popupOpen());
    CHECK(controller.popupDepth() == 1U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
    CHECK(!close_grace.hasPendingCandidate());
}

TEST_CASE("Terminal submenu close grace is cancelled when pointer reaches any popup before deadline") {
    Command leaf{"Leaf"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(leaf);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(controller.selectPopupItem(bar, 0U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 0U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 2U);

    TerminalMenuCloseOptions options;
    options.submenu_close_delay = 100ms;
    TerminalMenuCloseInteraction close_grace{options};
    const TerminalMenuCloseInteraction::TimePoint t0{};

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {60, 14});
    CHECK(frame.has_value());

    const PointerEvent outside = pointerMove({59, 13});
    (void)TerminalMenuPointerInteraction::handle(bar, controller, *frame, outside);
    close_grace.observe(controller, *frame, outside, AmbiguousWidthMode::narrow, t0);
    CHECK(close_grace.hasPendingCandidate());

    /*
     * Entering the child popup proves that the pointer successfully crossed the non-popup transfer region.
     * The pending close must disappear immediately; a later host-loop tick cannot collapse the submenu under
     * the pointer just because the earlier gap deadline has elapsed.
     */
    const PointerEvent child_move = pointerMove(popupRowPoint(*frame, 1U, 0U));
    (void)TerminalMenuPointerInteraction::handle(bar, controller, *frame, child_move);
    close_grace.observe(
        controller,
        *frame,
        child_move,
        AmbiguousWidthMode::narrow,
        t0 + 80ms);
    CHECK(!close_grace.hasPendingCandidate());

    const MenuInteractionResult result = close_grace.advance(bar, controller, t0 + 150ms);
    CHECK(result.action == MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
}

TEST_CASE("Terminal submenu close grace cannot collapse a different popup path that reuses numeric depth") {
    Command tools_leaf{"Tools leaf"};
    Command other_leaf{"Other leaf"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(tools_leaf);
    MenuModel& other = file.appendSubmenu("Other");
    other.appendCommand(other_leaf);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(controller.selectPopupItem(bar, 0U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 0U, 0U).action == MenuInteractionAction::state_changed);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});

    TerminalMenuCloseOptions options;
    options.submenu_close_delay = 50ms;
    TerminalMenuCloseInteraction close_grace{options};
    const TerminalMenuCloseInteraction::TimePoint t0{};

    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {60, 14});
    CHECK(frame.has_value());
    const PointerEvent outside = pointerMove({59, 13});
    close_grace.observe(controller, *frame, outside, AmbiguousWidthMode::narrow, t0);
    CHECK(close_grace.hasPendingCandidate());

    /*
     * Replace File/Tools with the sibling File/Other child before the old deadline. Both routes have depth two,
     * so depth alone would collide. The candidate's complete MenuPath scope must reject the stale timer rather
     * than collapsing a submenu that was opened by a later, unrelated semantic transition.
     */
    CHECK(controller.selectPopupItem(bar, 0U, 1U).action == MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 0U, 1U).action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{1U}});

    const MenuInteractionResult result = close_grace.advance(bar, controller, t0 + 50ms);
    CHECK(result.action == MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{1U}});
    CHECK(!close_grace.hasPendingCandidate());
}

TEST_CASE("Terminal submenu close grace ignores root-only menus") {
    Command action{"Action"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(action);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(controller.popupDepth() == 1U);

    TerminalMenuCloseInteraction close_grace;
    const auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {40, 10});
    CHECK(frame.has_value());

    close_grace.observe(
        controller,
        *frame,
        pointerMove({39, 9}),
        AmbiguousWidthMode::narrow,
        TerminalMenuCloseInteraction::TimePoint{});

    CHECK(!close_grace.hasPendingCandidate());
}

TEST_CASE("Terminal submenu close grace rejects negative delay configuration") {
    TerminalMenuCloseOptions options;
    options.submenu_close_delay = -1ms;

    bool threw = false;
    try {
        TerminalMenuCloseInteraction close_grace{options};
        (void)close_grace;
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
}
