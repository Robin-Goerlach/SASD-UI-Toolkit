#include "test_framework.hpp"

#include <sasd/ui/rendered/menu_pointer_interaction.hpp>

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

PointerEvent press(Point point) {
    return {point, PointerAction::press, PointerButton::primary, 1, KeyModifier::none};
}

PointerEvent release(Point point) {
    return {point, PointerAction::release, PointerButton::primary, 1, KeyModifier::none};
}
}

TEST_CASE("Rendered menu pointer opens a title and consumes outside dismissal") {
    Command open{"Open"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(open);
    (void)bar.appendMenu("Edit");
    MenuInteractionController controller;
    Metrics metrics;
    RenderedMenuPointerInteraction::GestureState gesture;

    auto frame = buildMenuFramePresentation(bar, controller, {0, 0}, {0, 0, 80, 30}, metrics);
    CHECK(frame.has_value());
    if (!frame.has_value()) return;
    const auto opened = RenderedMenuPointerInteraction::handle(
        bar, controller, *frame, press({1, 0}), gesture);
    CHECK(opened.has_value());
    CHECK(controller.popupOpen());

    frame = buildMenuFramePresentation(bar, controller, {0, 0}, {0, 0, 80, 30}, metrics);
    CHECK(frame.has_value());
    const auto dismissed = RenderedMenuPointerInteraction::handle(
        bar, controller, *frame, press({70, 25}), gesture);
    CHECK(dismissed.has_value());
    CHECK(dismissed->action == MenuInteractionAction::closed);
    CHECK(!controller.isActive());
}

TEST_CASE("Rendered menu pointer matching release returns command after closing semantic state") {
    Command save{"Save"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(save);
    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    Metrics metrics;
    RenderedMenuPointerInteraction::GestureState gesture;

    auto frame = buildMenuFramePresentation(bar, controller, {0, 0}, {0, 0, 80, 30}, metrics);
    CHECK(frame.has_value());
    if (!frame.has_value()) return;
    const Point row{frame->popups[0].snapshot.rows[0].bounds.x + 1,
                    frame->popups[0].snapshot.rows[0].bounds.y + 1};
    const auto pressed = RenderedMenuPointerInteraction::handle(
        bar, controller, *frame, press(row), gesture);
    CHECK(pressed.has_value());
    CHECK(gesture.armed());

    frame = buildMenuFramePresentation(bar, controller, {0, 0}, {0, 0, 80, 30}, metrics);
    CHECK(frame.has_value());
    const auto completed = RenderedMenuPointerInteraction::handle(
        bar, controller, *frame, release(row), gesture);
    CHECK(completed.has_value());
    CHECK(completed->action == MenuInteractionAction::activate_command);
    CHECK(!controller.isActive());
    CHECK(!gesture.armed());
    CHECK(completed->command.get() == &save);
}

TEST_CASE("Rendered menu pointer does not activate after mismatched release or surface leave") {
    Command save{"Save"};
    Command other{"Other"};
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(save);
    file.appendCommand(other);
    MenuInteractionController controller;
    CHECK(controller.begin(bar));
    (void)controller.handleKey(bar, KeyEvent{Key::enter, true, KeyModifier::none});
    Metrics metrics;
    RenderedMenuPointerInteraction::GestureState gesture;
    auto frame = buildMenuFramePresentation(bar, controller, {0, 0}, {0, 0, 80, 30}, metrics);
    CHECK(frame.has_value());
    if (!frame.has_value()) return;
    const Point first{frame->popups[0].snapshot.rows[0].bounds.x + 1,
                      frame->popups[0].snapshot.rows[0].bounds.y + 1};
    const Point second{frame->popups[0].snapshot.rows[1].bounds.x + 1,
                       frame->popups[0].snapshot.rows[1].bounds.y + 1};
    (void)RenderedMenuPointerInteraction::handle(bar, controller, *frame, press(first), gesture);
    const auto mismatched = RenderedMenuPointerInteraction::handle(
        bar, controller, *frame, release(second), gesture);
    CHECK(mismatched.has_value());
    CHECK(mismatched->action == MenuInteractionAction::none);
    CHECK(controller.isActive());

    (void)RenderedMenuPointerInteraction::handle(bar, controller, *frame, press(first), gesture);
    RenderedMenuPointerInteraction::handleSurfaceEvent(
        PointerSurfaceEvent{PointerSurfaceAction::left}, gesture);
    CHECK(!gesture.armed());
}
