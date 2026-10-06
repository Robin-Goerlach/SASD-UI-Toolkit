#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_close_interaction.hpp>
#include <sasd/ui/terminal/menu_frame_builder.hpp>
#include <sasd/ui/terminal/menu_hover_interaction.hpp>
#include <sasd/ui/terminal/menu_pointer_deferral_interaction.hpp>
#include <sasd/ui/terminal/menu_pointer_intent.hpp>
#include <sasd/ui/terminal/menu_pointer_interaction.hpp>
#include <sasd/ui/terminal/menu_presentation.hpp>

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
                                  std::size_t row,
                                  Coordinate x_offset = 1) {
    return Point{
        static_cast<Coordinate>(frame.popups.at(level).origin.x + x_offset),
        static_cast<Coordinate>(frame.popups.at(level).origin.y +
                                static_cast<Coordinate>(row)),
    };
}

void makeChildTall(MenuModel& child, Command& first_item, std::size_t height) {
    child.appendCommand(first_item);
    for (std::size_t row = 1U; row < height; ++row) {
        child.appendSeparator();
    }
}

void openToolsSubmenu(const MenuBarModel& bar, MenuInteractionController& controller) {
    CHECK(controller.begin(bar, 0U));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(controller.selectPopupItem(bar, 0U, 0U).action ==
          MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 0U, 0U).action ==
          MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
}

[[nodiscard]] MenuFramePresentationSnapshot
widenedRightOpeningFrame(const MenuBarModel& bar,
                         const MenuInteractionController& controller) {
    auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {120, 32});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 2U);

    const auto parent_size = measureMenuPopupPresentation(frame->popups[0].snapshot);
    CHECK(parent_size.has_value());

    /*
     * Production placement normally keeps parent and child close. A deliberate ten-cell gap makes the
     * safe-triangle interior easy to probe with integer terminal cells. Only presentation geometry changes;
     * semantic menu state remains the real MenuInteractionController state used by the test.
     */
    frame->popups[1].origin.x = static_cast<Coordinate>(
        frame->popups[0].origin.x + parent_size->size.width + 10);
    frame->popups[1].origin.y = frame->popups[0].origin.y;
    return *frame;
}

[[nodiscard]] Point parentRowNearChildEdge(const MenuFramePresentationSnapshot& frame,
                                           std::size_t row) {
    const auto parent_size = measureMenuPopupPresentation(frame.popups.at(0).snapshot);
    CHECK(parent_size.has_value());
    return Point{
        static_cast<Coordinate>(frame.popups.at(0).origin.x + parent_size->size.width - 1),
        static_cast<Coordinate>(frame.popups.at(0).origin.y + static_cast<Coordinate>(row)),
    };
}

} // namespace

TEST_CASE("Terminal menu pointer deferral delays a sibling row and replays it only at the safe-triangle deadline") {
    Command child_item{"Child"};
    Command sibling{"Sibling row with deliberately wide presentation"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    makeChildTall(tools, child_item, 8U);
    file.appendCommand(sibling);

    MenuInteractionController controller;
    openToolsSubmenu(bar, controller);
    MenuFramePresentationSnapshot frame = widenedRightOpeningFrame(bar, controller);

    TerminalMenuPointerIntent pointer_intent;
    TerminalMenuPointerDeferralOptions options;
    options.submenu_switch_delay = 100ms;
    TerminalMenuPointerDeferralInteraction deferral{options};
    const TerminalMenuPointerDeferralInteraction::TimePoint t0{};

    /* Establish the pre-interaction transfer anchor while the pointer still occupies Tools. */
    const PointerEvent anchor = pointerMove(popupRowPoint(frame, 0U, 0U));
    const auto anchor_intent = pointer_intent.observe(controller, frame, anchor);
    CHECK(anchor_intent == TerminalMenuPointerIntentKind::none);
    CHECK(deferral.observe(controller,
                           frame,
                           anchor,
                           anchor_intent,
                           AmbiguousWidthMode::narrow,
                           t0) == TerminalMenuPointerDeferralDecision::process_now);

    const PointerEvent sibling_move = pointerMove(parentRowNearChildEdge(frame, 1U));
    const auto sibling_intent = pointer_intent.observe(controller, frame, sibling_move);
    CHECK(sibling_intent == TerminalMenuPointerIntentKind::toward_open_submenu);

    /*
     * The point is physically on row 1, yet Core must remain on the Tools path until the bounded grace period
     * expires. The deferral helper only withholds the event; it does not manufacture an alternate selection.
     */
    CHECK(deferral.observe(controller,
                           frame,
                           sibling_move,
                           sibling_intent,
                           AmbiguousWidthMode::narrow,
                           t0) == TerminalMenuPointerDeferralDecision::defer);
    CHECK(deferral.hasPendingEvent());
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
    CHECK(!controller.popupSelection().has_value()); // deepest child opens without an initial row selection

    CHECK(!deferral.advance(controller,
                            frame,
                            AmbiguousWidthMode::narrow,
                            t0 + 99ms).has_value());

    const auto expired = deferral.advance(
        controller,
        frame,
        AmbiguousWidthMode::narrow,
        t0 + 100ms);
    CHECK(expired.has_value());
    CHECK(expired->position == sibling_move.position);
    CHECK(!deferral.hasPendingEvent());

    /*
     * Expiration returns a value event. Replaying it through the ordinary adapter invokes Core's established
     * selectPopupItem() transaction, which closes the old child because row 1 does not own that descendant.
     */
    const auto replay_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        frame,
        *expired);
    CHECK(replay_result.has_value());
    CHECK(replay_result->action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 1U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{}});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{1U});
}

TEST_CASE("Terminal menu pointer deferral keeps the first deadline but replays the latest sibling row") {
    Command child_item{"Child"};
    Command sibling1{"Sibling row one with deliberately wide presentation"};
    Command sibling2{"Sibling row two with deliberately wide presentation"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    makeChildTall(tools, child_item, 10U);
    file.appendCommand(sibling1);
    file.appendCommand(sibling2);

    MenuInteractionController controller;
    openToolsSubmenu(bar, controller);
    MenuFramePresentationSnapshot frame = widenedRightOpeningFrame(bar, controller);

    TerminalMenuPointerIntent pointer_intent;
    TerminalMenuPointerDeferralOptions options;
    options.submenu_switch_delay = 100ms;
    TerminalMenuPointerDeferralInteraction deferral{options};
    const TerminalMenuPointerDeferralInteraction::TimePoint t0{};

    (void)pointer_intent.observe(
        controller,
        frame,
        pointerMove(popupRowPoint(frame, 0U, 0U)));

    const PointerEvent first_sibling = pointerMove(parentRowNearChildEdge(frame, 1U));
    const auto first_intent = pointer_intent.observe(controller, frame, first_sibling);
    CHECK(first_intent == TerminalMenuPointerIntentKind::toward_open_submenu);
    CHECK(deferral.observe(controller,
                           frame,
                           first_sibling,
                           first_intent,
                           AmbiguousWidthMode::narrow,
                           t0) == TerminalMenuPointerDeferralDecision::defer);

    /*
     * Dense all-motion traffic updates only the physical sample. It must not restart the deadline; otherwise a
     * moving pointer could suppress sibling selection indefinitely. The latest row remains the correct replay.
     */
    const PointerEvent second_sibling = pointerMove(parentRowNearChildEdge(frame, 2U));
    const auto second_intent = pointer_intent.observe(controller, frame, second_sibling);
    CHECK(second_intent == TerminalMenuPointerIntentKind::toward_open_submenu);
    CHECK(deferral.observe(controller,
                           frame,
                           second_sibling,
                           second_intent,
                           AmbiguousWidthMode::narrow,
                           t0 + 80ms) == TerminalMenuPointerDeferralDecision::defer);

    const auto expired = deferral.advance(
        controller,
        frame,
        AmbiguousWidthMode::narrow,
        t0 + 100ms);
    CHECK(expired.has_value());
    CHECK(expired->position == second_sibling.position);

    const auto replay_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        frame,
        *expired);
    CHECK(replay_result.has_value());
    CHECK(replay_result->action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 1U);
    CHECK(controller.popupSelection() == std::optional<std::size_t>{2U});
}

TEST_CASE("Terminal menu pointer deferral cancels a sibling candidate when the pointer reaches the child") {
    Command child_item{"Child"};
    Command sibling{"Sibling row with deliberately wide presentation"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    makeChildTall(tools, child_item, 5U);
    file.appendCommand(sibling);

    MenuInteractionController controller;
    openToolsSubmenu(bar, controller);
    MenuFramePresentationSnapshot frame = widenedRightOpeningFrame(bar, controller);

    TerminalMenuPointerIntent pointer_intent;
    TerminalMenuPointerDeferralOptions options;
    options.submenu_switch_delay = 100ms;
    TerminalMenuPointerDeferralInteraction deferral{options};
    const TerminalMenuPointerDeferralInteraction::TimePoint t0{};

    (void)pointer_intent.observe(
        controller,
        frame,
        pointerMove(popupRowPoint(frame, 0U, 0U)));
    const PointerEvent sibling_move = pointerMove(parentRowNearChildEdge(frame, 1U));
    const auto sibling_intent = pointer_intent.observe(controller, frame, sibling_move);
    CHECK(sibling_intent == TerminalMenuPointerIntentKind::toward_open_submenu);
    CHECK(deferral.observe(controller,
                           frame,
                           sibling_move,
                           sibling_intent,
                           AmbiguousWidthMode::narrow,
                           t0) == TerminalMenuPointerDeferralDecision::defer);

    /*
     * Entering the target child completes the trajectory. Pointer intent returns none, which causes the timing
     * policy to retire the old sibling sample and process the child motion normally with no delayed surprise.
     */
    const PointerEvent child_move = pointerMove(popupRowPoint(frame, 1U, 0U));
    const auto child_intent = pointer_intent.observe(controller, frame, child_move);
    CHECK(child_intent == TerminalMenuPointerIntentKind::none);
    CHECK(deferral.observe(controller,
                           frame,
                           child_move,
                           child_intent,
                           AmbiguousWidthMode::narrow,
                           t0 + 60ms) == TerminalMenuPointerDeferralDecision::process_now);
    CHECK(!deferral.hasPendingEvent());

    const auto child_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        frame,
        child_move);
    CHECK(child_result.has_value());
    CHECK(child_result->action == MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});

    CHECK(!deferral.advance(controller,
                            frame,
                            AmbiguousWidthMode::narrow,
                            t0 + 150ms).has_value());
}

TEST_CASE("Terminal menu pointer deferral discards a pending event when semantic popup scope changes") {
    Command child_item{"Child"};
    Command sibling{"Sibling row with deliberately wide presentation"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    makeChildTall(tools, child_item, 4U);
    file.appendCommand(sibling);

    MenuInteractionController controller;
    openToolsSubmenu(bar, controller);
    MenuFramePresentationSnapshot frame = widenedRightOpeningFrame(bar, controller);

    TerminalMenuPointerIntent pointer_intent;
    TerminalMenuPointerDeferralOptions options;
    options.submenu_switch_delay = 50ms;
    TerminalMenuPointerDeferralInteraction deferral{options};
    const TerminalMenuPointerDeferralInteraction::TimePoint t0{};

    (void)pointer_intent.observe(
        controller,
        frame,
        pointerMove(popupRowPoint(frame, 0U, 0U)));
    const PointerEvent sibling_move = pointerMove(parentRowNearChildEdge(frame, 1U));
    const auto sibling_intent = pointer_intent.observe(controller, frame, sibling_move);
    CHECK(sibling_intent == TerminalMenuPointerIntentKind::toward_open_submenu);
    CHECK(deferral.observe(controller,
                           frame,
                           sibling_move,
                           sibling_intent,
                           AmbiguousWidthMode::narrow,
                           t0) == TerminalMenuPointerDeferralDecision::defer);

    /*
     * Another input policy may change Core before the timeout. The pending sample belongs to the old Tools path
     * and must be discarded rather than reinterpreted after that path has already been truncated.
     */
    CHECK(controller.selectPopupItem(bar, 0U, 1U).action ==
          MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 1U);

    const auto rebuilt = buildMenuPresentationFrame(bar, controller, {0, 0}, {120, 32});
    CHECK(rebuilt.has_value());
    CHECK(!deferral.advance(controller,
                            *rebuilt,
                            AmbiguousWidthMode::narrow,
                            t0 + 50ms).has_value());
    CHECK(!deferral.hasPendingEvent());
    CHECK(controller.popupSelection() == std::optional<std::size_t>{1U});
}

TEST_CASE("Terminal menu pointer deferral rejects negative switch delay configuration") {
    TerminalMenuPointerDeferralOptions options;
    options.submenu_switch_delay = -1ms;

    bool threw = false;
    try {
        TerminalMenuPointerDeferralInteraction deferral{options};
        (void)deferral;
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
}

TEST_CASE("Terminal menu pointer pipeline cancels a deferred sibling when the child is reached before deadline") {
    Command child_item{"Child"};
    Command sibling{"Sibling row with deliberately wide presentation"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    makeChildTall(tools, child_item, 8U);
    file.appendCommand(sibling);

    MenuInteractionController controller;
    openToolsSubmenu(bar, controller);
    MenuFramePresentationSnapshot frame = widenedRightOpeningFrame(bar, controller);

    TerminalMenuPointerIntent pointer_intent;

    TerminalMenuPointerDeferralOptions deferral_options;
    deferral_options.submenu_switch_delay = 100ms;
    TerminalMenuPointerDeferralInteraction deferral{deferral_options};

    TerminalMenuHoverOptions hover_options;
    hover_options.submenu_open_delay = 250ms;
    TerminalMenuHoverInteraction hover{hover_options};

    TerminalMenuCloseOptions close_options;
    close_options.submenu_close_delay = 150ms;
    TerminalMenuCloseInteraction close_grace{close_options};

    TerminalMenuPointerInteraction::GestureState gesture;
    const TerminalMenuPointerDeferralInteraction::TimePoint t0{};

    /*
     * The synthetic path deliberately anchors inside the owning row rather than at its final rightmost cell.
     * Terminal intent works with integer cell coordinates: a sibling hit at the same x coordinate as a
     * right-edge anchor is vertically below the triangle apex and therefore correctly lies outside the
     * corridor. Starting from an interior cell gives this test real horizontal progress toward the child while
     * crossing the sibling, which is the diagonal transfer scenario the safe-triangle policy is meant to cover.
     * The sample is then run through the normal immediate adapter and both post-interaction timing policies.
     */
    const PointerEvent anchor = pointerMove(popupRowPoint(frame, 0U, 0U));
    const auto anchor_intent = pointer_intent.observe(controller, frame, anchor);
    CHECK(anchor_intent == TerminalMenuPointerIntentKind::none);
    CHECK(deferral.observe(controller,
                           frame,
                           anchor,
                           anchor_intent,
                           AmbiguousWidthMode::narrow,
                           t0) == TerminalMenuPointerDeferralDecision::process_now);

    const auto anchor_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        frame,
        anchor,
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(anchor_result.has_value());
    hover.observe(controller, frame, anchor, AmbiguousWidthMode::narrow, t0);
    close_grace.observe(controller, frame, anchor, AmbiguousWidthMode::narrow, t0);
    CHECK(controller.popupDepth() == 2U);

    /*
     * The diagonal path now crosses the sibling row. Intent qualifies that geometry and deferral owns the
     * sample, so the host must NOT call TerminalMenuPointerInteraction yet. Just like the demo, clear the
     * unrelated hover-open and gap-close candidates because neither policy should inherit a deliberately
     * suppressed sibling sample.
     */
    const PointerEvent sibling_move = pointerMove(parentRowNearChildEdge(frame, 1U));
    const auto sibling_intent = pointer_intent.observe(controller, frame, sibling_move);
    CHECK(sibling_intent == TerminalMenuPointerIntentKind::toward_open_submenu);
    CHECK(deferral.observe(controller,
                           frame,
                           sibling_move,
                           sibling_intent,
                           AmbiguousWidthMode::narrow,
                           t0 + 10ms) == TerminalMenuPointerDeferralDecision::defer);
    hover.reset();
    close_grace.reset();

    CHECK(deferral.hasPendingEvent());
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});

    /*
     * Reaching the actual child before 100 ms completes the transfer. PointerIntent retires its anchor and the
     * deferral policy discards the withheld sibling. The child motion then flows immediately through the same
     * adapter/timing sequence as any physical event, proving that no replay is required to recover normal input.
     */
    const PointerEvent child_move = pointerMove(popupRowPoint(frame, 1U, 0U));
    const auto child_intent = pointer_intent.observe(controller, frame, child_move);
    CHECK(child_intent == TerminalMenuPointerIntentKind::none);
    CHECK(deferral.observe(controller,
                           frame,
                           child_move,
                           child_intent,
                           AmbiguousWidthMode::narrow,
                           t0 + 60ms) == TerminalMenuPointerDeferralDecision::process_now);
    CHECK(!deferral.hasPendingEvent());

    const auto child_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        frame,
        child_move,
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(child_result.has_value());
    CHECK(child_result->action == MenuInteractionAction::state_changed);
    hover.observe(controller, frame, child_move, AmbiguousWidthMode::narrow, t0 + 60ms);
    close_grace.observe(controller, frame, child_move, AmbiguousWidthMode::narrow, t0 + 60ms);

    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
    CHECK(!close_grace.hasPendingCandidate());

    /*
     * Move the host clock beyond every old deadline. The cancelled sibling must never reappear, close grace has
     * no stale transaction, and hover merely asks Core whether the selected child command is a submenu (it is
     * not). The open Tools route therefore remains stable after all delayed policies have had a chance to run.
     */
    CHECK(!deferral.advance(controller,
                            frame,
                            AmbiguousWidthMode::narrow,
                            t0 + 200ms).has_value());
    CHECK(close_grace.advance(bar, controller, t0 + 220ms).action ==
          MenuInteractionAction::none);
    CHECK(hover.advance(bar, controller, t0 + 400ms).action ==
          MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
}

TEST_CASE("Terminal menu pointer pipeline leaves safe-triangle gap motion immediate so close grace owns it") {
    Command child_item{"Child"};
    Command sibling{"Sibling row with deliberately wide presentation"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    makeChildTall(tools, child_item, 8U);
    file.appendCommand(sibling);

    MenuInteractionController controller;
    openToolsSubmenu(bar, controller);
    MenuFramePresentationSnapshot frame = widenedRightOpeningFrame(bar, controller);

    TerminalMenuPointerIntent pointer_intent;

    TerminalMenuPointerDeferralOptions deferral_options;
    deferral_options.submenu_switch_delay = 100ms;
    TerminalMenuPointerDeferralInteraction deferral{deferral_options};

    TerminalMenuHoverOptions hover_options;
    hover_options.submenu_open_delay = 250ms;
    TerminalMenuHoverInteraction hover{hover_options};

    TerminalMenuCloseOptions close_options;
    close_options.submenu_close_delay = 120ms;
    TerminalMenuCloseInteraction close_grace{close_options};

    TerminalMenuPointerInteraction::GestureState gesture;
    const TerminalMenuPointerDeferralInteraction::TimePoint t0{};

    const PointerEvent anchor = pointerMove(parentRowNearChildEdge(frame, 0U));
    const auto anchor_intent = pointer_intent.observe(controller, frame, anchor);
    CHECK(anchor_intent == TerminalMenuPointerIntentKind::none);
    CHECK(deferral.observe(controller,
                           frame,
                           anchor,
                           anchor_intent,
                           AmbiguousWidthMode::narrow,
                           t0) == TerminalMenuPointerDeferralDecision::process_now);
    (void)TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        frame,
        anchor,
        AmbiguousWidthMode::narrow,
        gesture);
    hover.observe(controller, frame, anchor, AmbiguousWidthMode::narrow, t0);
    close_grace.observe(controller, frame, anchor, AmbiguousWidthMode::narrow, t0);

    const auto parent_size = measureMenuPopupPresentation(frame.popups.at(0).snapshot);
    CHECK(parent_size.has_value());
    const Coordinate parent_right = static_cast<Coordinate>(
        frame.popups.at(0).origin.x + parent_size->size.width - 1);
    const Coordinate child_left = frame.popups.at(1).origin.x;
    CHECK(child_left > parent_right + 1);

    const Point gap_point{
        static_cast<Coordinate>(parent_right + (child_left - parent_right) / 2),
        static_cast<Coordinate>(frame.popups.at(0).origin.y + 1),
    };
    CHECK(!TerminalMenuHitTest::popupItemAt(
               frame,
               gap_point,
               AmbiguousWidthMode::narrow).has_value());

    /*
     * The gap still lies inside the geometric safe triangle, so PointerIntent remains positive. Deferral must
     * nevertheless return process_now because no sibling parent row is threatened. Immediate menu handling then
     * consumes the modal motion without changing Core, hover timing clears, and close grace becomes the sole
     * owner of the short parent-to-child gap.
     */
    const PointerEvent gap_move = pointerMove(gap_point);
    const auto gap_intent = pointer_intent.observe(controller, frame, gap_move);
    CHECK(gap_intent == TerminalMenuPointerIntentKind::toward_open_submenu);
    CHECK(deferral.observe(controller,
                           frame,
                           gap_move,
                           gap_intent,
                           AmbiguousWidthMode::narrow,
                           t0 + 20ms) == TerminalMenuPointerDeferralDecision::process_now);
    CHECK(!deferral.hasPendingEvent());

    const auto gap_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        frame,
        gap_move,
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(gap_result.has_value());
    CHECK(gap_result->action == MenuInteractionAction::none);
    hover.observe(controller, frame, gap_move, AmbiguousWidthMode::narrow, t0 + 20ms);
    close_grace.observe(controller, frame, gap_move, AmbiguousWidthMode::narrow, t0 + 20ms);
    CHECK(!hover.hasPendingCandidate());
    CHECK(close_grace.hasPendingCandidate());
    CHECK(controller.popupDepth() == 2U);

    /*
     * Arriving in the child before the 120 ms close deadline cancels the gap transaction. This verifies the
     * architectural handoff: safe-triangle deferral protects only sibling replacement, while ordinary gap
     * transfer remains immediate and is protected by TerminalMenuCloseInteraction instead.
     */
    const PointerEvent child_move = pointerMove(popupRowPoint(frame, 1U, 0U));
    const auto child_intent = pointer_intent.observe(controller, frame, child_move);
    CHECK(child_intent == TerminalMenuPointerIntentKind::none);
    CHECK(deferral.observe(controller,
                           frame,
                           child_move,
                           child_intent,
                           AmbiguousWidthMode::narrow,
                           t0 + 80ms) == TerminalMenuPointerDeferralDecision::process_now);

    const auto child_result = TerminalMenuPointerInteraction::handle(
        bar,
        controller,
        frame,
        child_move,
        AmbiguousWidthMode::narrow,
        gesture);
    CHECK(child_result.has_value());
    hover.observe(controller, frame, child_move, AmbiguousWidthMode::narrow, t0 + 80ms);
    close_grace.observe(controller, frame, child_move, AmbiguousWidthMode::narrow, t0 + 80ms);
    CHECK(!close_grace.hasPendingCandidate());

    CHECK(close_grace.advance(bar, controller, t0 + 200ms).action ==
          MenuInteractionAction::none);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
    CHECK(controller.popupSelection() == std::optional<std::size_t>{0U});
}
