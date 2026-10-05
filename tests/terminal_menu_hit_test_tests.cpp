#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_hit_test.hpp>

#include <optional>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal menu hit test maps complete padded menu-bar title spans") {
    MenuFramePresentationSnapshot frame;
    frame.menu_bar_origin = {2, 1};
    frame.menu_bar.titles = {"File", "界"};

    /*
     * "File" occupies four cells plus one presentation padding cell on each side, so its complete
     * clickable span is [2,8). The CJK scalar 界 is two cells wide under the default narrow ambiguous-width
     * policy (it is intrinsically wide, not ambiguous), so its padded span is [8,12).
     *
     * Padding deliberately belongs to each title's hit surface because renderMenuBarPresentation() paints
     * selection inverse-video across those same cells. The geometric and visible selection rectangles must
     * therefore agree exactly.
     */
    CHECK(TerminalMenuHitTest::menuBarIndexAt(frame, {2, 1}) ==
          std::optional<std::size_t>{0U});
    CHECK(TerminalMenuHitTest::menuBarIndexAt(frame, {7, 1}) ==
          std::optional<std::size_t>{0U});
    CHECK(TerminalMenuHitTest::menuBarIndexAt(frame, {8, 1}) ==
          std::optional<std::size_t>{1U});
    CHECK(TerminalMenuHitTest::menuBarIndexAt(frame, {11, 1}) ==
          std::optional<std::size_t>{1U});

    CHECK(!TerminalMenuHitTest::menuBarIndexAt(frame, {12, 1}).has_value());
    CHECK(!TerminalMenuHitTest::menuBarIndexAt(frame, {4, 0}).has_value());
}

TEST_CASE("Terminal menu hit test returns topmost popup rows in frame paint order") {
    MenuFramePresentationSnapshot frame;
    frame.menu_bar.titles = {"File"};

    MenuPopupPresentationSnapshot root;
    root.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Root A", true, std::nullopt});
    root.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::separator, "", false, std::nullopt});
    root.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Root B", false, std::nullopt});

    MenuPopupPresentationSnapshot child;
    child.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Child", true, std::nullopt});

    /*
     * The synthetic child deliberately overlaps the first row of the root. Real frame construction normally
     * fits children beside parents, but paint-order overlap is part of MenuFramePresentationSnapshot's public
     * presentation contract and is worth protecting independently. The later child must win where both
     * rectangles cover the same cell because it is also rendered last.
     */
    frame.popups.push_back(PositionedMenuPopupPresentationSnapshot{{4, 2}, root, std::nullopt});
    frame.popups.push_back(PositionedMenuPopupPresentationSnapshot{{4, 2}, child, std::nullopt});

    CHECK(TerminalMenuHitTest::popupItemAt(frame, {5, 2}) ==
          std::optional<TerminalMenuPopupHit>{TerminalMenuPopupHit{1U, 0U}});

    /*
     * Once the point leaves the one-row child, the root is visible again. Separators and disabled commands
     * remain geometric row hits: whether they may be selected or activated is semantic interaction policy,
     * not terminal rectangle ownership.
     */
    CHECK(TerminalMenuHitTest::popupItemAt(frame, {5, 3}) ==
          std::optional<TerminalMenuPopupHit>{TerminalMenuPopupHit{0U, 1U}});
    CHECK(TerminalMenuHitTest::popupItemAt(frame, {5, 4}) ==
          std::optional<TerminalMenuPopupHit>{TerminalMenuPopupHit{0U, 2U}});

    CHECK(!TerminalMenuHitTest::popupItemAt(frame, {20, 4}).has_value());
}

TEST_CASE("Terminal menu hit test fails closed for an unrepresentable popup frame") {
    MenuFramePresentationSnapshot frame;
    frame.menu_bar.titles = {"File"};

    MenuPopupPresentationSnapshot valid;
    valid.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});

    MenuPopupPresentationSnapshot unsupported;
    /*
     * Combining marks are intentionally outside the current simple terminal Cell contract. Even though the
     * pointer below lands on the otherwise valid first popup, renderMenuPresentationFrame() would reject the
     * complete transaction because another layer is unrepresentable. Hit testing mirrors that fail-closed
     * boundary instead of returning an identity for a frame that cannot truthfully be on screen.
     */
    unsupported.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "A\xCC\x81", true, std::nullopt});

    frame.popups.push_back(PositionedMenuPopupPresentationSnapshot{{1, 1}, valid, std::nullopt});
    frame.popups.push_back(PositionedMenuPopupPresentationSnapshot{{12, 1}, unsupported, std::nullopt});

    CHECK(!TerminalMenuHitTest::popupItemAt(frame, {2, 1}).has_value());
}
