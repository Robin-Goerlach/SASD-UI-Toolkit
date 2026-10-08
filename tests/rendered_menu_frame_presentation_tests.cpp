#include "test_framework.hpp"

#include <sasd/ui/rendered/menu_frame_presentation.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {
class Metrics final : public RenderedMeasurementContext {
public:
    [[nodiscard]] std::uint64_t revision() const noexcept override { return 1; }
    [[nodiscard]] Size measureText(std::string_view text) const override {
        return {static_cast<Coordinate>(text.size()), 1};
    }
    [[nodiscard]] Coordinate lineHeight() const noexcept override { return 2; }
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(std::string_view,
                                                                  std::size_t index) const override {
        return static_cast<Coordinate>(index);
    }
};
}

TEST_CASE("Rendered menu frame builds persistent bar and root popup in paint order") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);

    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::down, true, KeyModifier::none});

    Metrics metrics;
    const auto frame = buildMenuFramePresentation(bar, controller, {1, 1}, {0, 0, 80, 30}, metrics);
    CHECK(frame.has_value());
    if (!frame.has_value()) return;
    CHECK(frame->menu_bar.items.size() == 1U);
    CHECK(frame->popups.size() == 1U);
    CHECK(frame->popups[0].snapshot.bounds.y > frame->menu_bar.bounds.y);

    DisplayList base;
    base.drawText({0, 0}, "base");
    const DisplayList before = base;
    CHECK(renderMenuFramePresentation(base, *frame));
    CHECK(base.size() > before.size());
}

TEST_CASE("Rendered menu frame hit testing gives nested popup topmost precedence") {
    RenderedMenuFramePresentationSnapshot frame;
    frame.viewport = {0, 0, 50, 30};
    frame.menu_bar.row_height = 2;
    frame.menu_bar.horizontal_padding = 1;
    frame.menu_bar.bounds = {0, 0, 20, 2};
    frame.menu_bar.items.push_back({"File", {4, 1}, {0, 0, 8, 2}});

    RenderedMenuPopupPresentationSnapshot popup;
    popup.bounds = {2, 2, 12, 8};
    popup.content_bounds = {3, 3, 10, 6};
    popup.row_height = 3;
    popup.padding = 1;
    popup.border_thickness = 1;
    popup.label_lane_width = 4;
    popup.rows.push_back({
        MenuItemPresentationSnapshot{MenuItemKind::command, "One", true, std::nullopt},
        {3, 3, 10, 3}, {3, 1}, std::string{}, {}, {4, 3, 4, 3}, {}, {}});
    popup.rows.push_back({
        MenuItemPresentationSnapshot{MenuItemKind::command, "Two", true, std::nullopt},
        {3, 6, 10, 3}, {3, 1}, std::string{}, {}, {4, 6, 4, 3}, {}, {}});
    frame.popups.push_back({popup, false});

    CHECK(menuFrameHitAt(frame, {5, 7}) ==
          std::optional<RenderedMenuHit>{RenderedMenuHit{RenderedMenuHit::Kind::popup_row, 0, 1}});
    CHECK(menuFrameHitAt(frame, {0, 0}) ==
          std::optional<RenderedMenuHit>{RenderedMenuHit{RenderedMenuHit::Kind::menu_bar, 0, 0}});
}
