#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_frame_builder.hpp>
#include <sasd/ui/terminal/menu_hit_test.hpp>
#include <sasd/ui/terminal/menu_pointer_intent.hpp>
#include <sasd/ui/terminal/menu_presentation.hpp>

#include <cstdint>
#include <optional>

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

[[nodiscard]] Coordinate midpoint(Coordinate a, Coordinate b) {
    return static_cast<Coordinate>(
        (static_cast<std::int64_t>(a) + static_cast<std::int64_t>(b)) / 2);
}

void openFirstRootSubmenu(const MenuBarModel& bar, MenuInteractionController& controller) {
    CHECK(controller.begin(bar, 0U));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(controller.selectPopupItem(bar, 0U, 0U).action ==
          MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 0U, 0U).action ==
          MenuInteractionAction::state_changed);
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
}

} // namespace

TEST_CASE("Terminal menu pointer intent classifies a sibling-row crossing toward the open child without mutating Core") {
    Command child0{"Child 0"};
    Command child1{"Child 1"};
    Command child2{"Child 2"};
    Command child3{"Child 3"};
    Command child4{"Child 4"};
    Command child5{"Child 5"};
    Command child6{"Child 6"};
    Command child7{"Child 7"};
    Command other{"Other"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(child0);
    tools.appendCommand(child1);
    tools.appendCommand(child2);
    tools.appendCommand(child3);
    tools.appendCommand(child4);
    tools.appendCommand(child5);
    tools.appendCommand(child6);
    tools.appendCommand(child7);
    file.appendCommand(other);

    MenuInteractionController controller;
    openFirstRootSubmenu(bar, controller);

    auto frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {100, 30});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 2U);

    const auto parent_size =
        measureMenuPopupPresentation(frame->popups[0].snapshot);
    CHECK(parent_size.has_value());

    /*
     * Widen the parent-to-child gap deliberately. Real frame placement normally keeps popups close, but
     * a synthetic gap makes the safe-triangle classification observable at several terminal cells without
     * changing semantic menu state or relying on one platform's viewport-fitting accident.
     */
    frame->popups[1].origin.x = static_cast<Coordinate>(
        frame->popups[0].origin.x + parent_size->size.width + 10);

    TerminalMenuPointerIntent intent;
    const Point parent_anchor = popupRowPoint(*frame, 0U, 0U);
    CHECK(intent.observe(controller, *frame, pointerMove(parent_anchor)) ==
          TerminalMenuPointerIntentKind::none);
    CHECK(intent.hasTransferAnchor());

    /*
     * The physical point is on sibling row 1 of the parent popup, close to its child-facing edge. Because
     * the child is tall, that cell lies inside the triangle from the last Tools cell to the near edge of
     * the open child. The classifier reports intent only; it does NOT call selectPopupItem(), so Tools and
     * its child remain the current semantic route. A later host policy can use this exact fact to defer the
     * sibling selection briefly rather than closing the child prematurely.
     */
    const Coordinate sibling_x = static_cast<Coordinate>(
        frame->popups[0].origin.x + parent_size->size.width - 1);
    const Point sibling_point{
        sibling_x,
        static_cast<Coordinate>(frame->popups[0].origin.y + 1),
    };
    const auto sibling_hit = TerminalMenuHitTest::popupItemAt(*frame, sibling_point);
    CHECK(sibling_hit == std::optional<TerminalMenuPopupHit>{TerminalMenuPopupHit{0U, 1U}});

    CHECK(intent.observe(controller, *frame, pointerMove(sibling_point)) ==
          TerminalMenuPointerIntentKind::toward_open_submenu);
    CHECK(intent.hasTransferAnchor());
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
}

TEST_CASE("Terminal menu pointer intent accepts the same corridor geometry when viewport fitting puts the child on the left") {
    Command child0{"Child 0"};
    Command child1{"Child 1"};
    Command child2{"Child 2"};
    Command child3{"Child 3"};
    Command child4{"Child 4"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(child0);
    tools.appendCommand(child1);
    tools.appendCommand(child2);
    tools.appendCommand(child3);
    tools.appendCommand(child4);

    MenuInteractionController controller;
    openFirstRootSubmenu(bar, controller);

    auto frame = buildMenuPresentationFrame(bar, controller, {20, 0}, {100, 30});
    CHECK(frame.has_value());
    CHECK(frame->popups.size() == 2U);

    const auto child_size = measureMenuPopupPresentation(frame->popups[1].snapshot);
    CHECK(child_size.has_value());

    /*
     * Force only the presentation origin to the left of the parent. Pointer intent must derive the near edge
     * from final geometry rather than hard-code the conventional right-opening direction. This mirrors the
     * frame builder's real ability to flip nested popups when the right side of the viewport has no room.
     */
    frame->popups[1].origin.x = static_cast<Coordinate>(
        frame->popups[0].origin.x - child_size->size.width - 8);

    TerminalMenuPointerIntent intent;
    const Point parent_anchor = popupRowPoint(*frame, 0U, 0U);
    CHECK(intent.observe(controller, *frame, pointerMove(parent_anchor)) ==
          TerminalMenuPointerIntentKind::none);
    CHECK(intent.hasTransferAnchor());

    const Coordinate child_right = static_cast<Coordinate>(
        frame->popups[1].origin.x + child_size->size.width - 1);
    const Point transfer_point{
        midpoint(parent_anchor.x, child_right),
        static_cast<Coordinate>(frame->popups[1].origin.y + 1),
    };

    CHECK(!TerminalMenuHitTest::popupItemAt(*frame, transfer_point).has_value());
    CHECK(intent.observe(controller, *frame, pointerMove(transfer_point)) ==
          TerminalMenuPointerIntentKind::toward_open_submenu);
    CHECK(controller.popupDepth() == 2U);
}

TEST_CASE("Terminal menu pointer intent retires the transfer anchor as soon as motion leaves the child corridor") {
    Command child0{"Child 0"};
    Command child1{"Child 1"};
    Command child2{"Child 2"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& tools = file.appendSubmenu("Tools");
    tools.appendCommand(child0);
    tools.appendCommand(child1);
    tools.appendCommand(child2);

    MenuInteractionController controller;
    openFirstRootSubmenu(bar, controller);

    auto frame = buildMenuPresentationFrame(bar, controller, {10, 0}, {80, 20});
    CHECK(frame.has_value());

    const auto parent_size = measureMenuPopupPresentation(frame->popups[0].snapshot);
    CHECK(parent_size.has_value());
    frame->popups[1].origin.x = static_cast<Coordinate>(
        frame->popups[0].origin.x + parent_size->size.width + 8);

    TerminalMenuPointerIntent intent;
    const Point parent_anchor = popupRowPoint(*frame, 0U, 0U);
    CHECK(intent.observe(controller, *frame, pointerMove(parent_anchor)) ==
          TerminalMenuPointerIntentKind::none);
    CHECK(intent.hasTransferAnchor());

    /*
     * Moving horizontally away from a right-opening child cannot be part of the finite parent-to-child
     * triangle. The classifier fails closed and drops the anchor immediately; unrelated later motion cannot
     * revive the old transfer without first re-entering the owning parent row.
     */
    const Point away{
        static_cast<Coordinate>(frame->popups[0].origin.x - 2),
        parent_anchor.y,
    };
    CHECK(intent.observe(controller, *frame, pointerMove(away)) ==
          TerminalMenuPointerIntentKind::none);
    CHECK(!intent.hasTransferAnchor());

    const Point later_gap{
        midpoint(parent_anchor.x, frame->popups[1].origin.x),
        parent_anchor.y,
    };
    CHECK(intent.observe(controller, *frame, pointerMove(later_gap)) ==
          TerminalMenuPointerIntentKind::none);
    CHECK(!intent.hasTransferAnchor());
}

TEST_CASE("Terminal menu pointer intent cannot reuse an anchor after switching to another top-level menu with the same numeric path") {
    Command file_child0{"File child 0"};
    Command file_child1{"File child 1"};
    Command file_child2{"File child 2"};
    Command edit_child0{"Edit child 0"};
    Command edit_child1{"Edit child 1"};
    Command edit_child2{"Edit child 2"};

    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    MenuModel& file_tools = file.appendSubmenu("Tools");
    file_tools.appendCommand(file_child0);
    file_tools.appendCommand(file_child1);
    file_tools.appendCommand(file_child2);

    MenuModel& edit = bar.appendMenu("Edit");
    MenuModel& edit_tools = edit.appendSubmenu("Tools");
    edit_tools.appendCommand(edit_child0);
    edit_tools.appendCommand(edit_child1);
    edit_tools.appendCommand(edit_child2);

    MenuInteractionController controller;
    openFirstRootSubmenu(bar, controller);

    auto file_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {100, 24});
    CHECK(file_frame.has_value());
    const auto file_parent_size =
        measureMenuPopupPresentation(file_frame->popups[0].snapshot);
    CHECK(file_parent_size.has_value());
    file_frame->popups[1].origin.x = static_cast<Coordinate>(
        file_frame->popups[0].origin.x + file_parent_size->size.width + 8);

    TerminalMenuPointerIntent intent;
    const Point file_anchor = popupRowPoint(*file_frame, 0U, 0U);
    CHECK(intent.observe(controller, *file_frame, pointerMove(file_anchor)) ==
          TerminalMenuPointerIntentKind::none);
    CHECK(intent.hasTransferAnchor());

    /*
     * Edit/Tools has exactly the same {level=0,item=0} and MenuPath {0} numeric shape as File/Tools.
     * The selected top-level index is therefore part of anchor identity. Switching roots must make the old
     * anchor unusable even if the replacement frame happens to put a point inside an otherwise similar triangle.
     */
    CHECK(controller.begin(bar, 1U));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(controller.selectPopupItem(bar, 0U, 0U).action ==
          MenuInteractionAction::state_changed);
    CHECK(controller.openPopupSubmenu(bar, 0U, 0U).action ==
          MenuInteractionAction::state_changed);
    CHECK(controller.menuBarSelection() == std::optional<std::size_t>{1U});
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});

    auto edit_frame = buildMenuPresentationFrame(bar, controller, {0, 0}, {100, 24});
    CHECK(edit_frame.has_value());
    const auto edit_parent_size =
        measureMenuPopupPresentation(edit_frame->popups[0].snapshot);
    CHECK(edit_parent_size.has_value());
    edit_frame->popups[1].origin.x = static_cast<Coordinate>(
        edit_frame->popups[0].origin.x + edit_parent_size->size.width + 8);

    const Point edit_gap{
        midpoint(popupRowPoint(*edit_frame, 0U, 0U).x,
                 edit_frame->popups[1].origin.x),
        static_cast<Coordinate>(edit_frame->popups[1].origin.y + 1),
    };
    CHECK(intent.observe(controller, *edit_frame, pointerMove(edit_gap)) ==
          TerminalMenuPointerIntentKind::none);
    CHECK(!intent.hasTransferAnchor());
    CHECK(controller.popupDepth() == 2U);
    CHECK(controller.popupPath() == std::optional<MenuPath>{MenuPath{0U}});
}
