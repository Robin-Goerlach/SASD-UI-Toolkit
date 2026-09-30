#include "test_framework.hpp"

#include <sasd/ui/check_box.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/pointer_router.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

using namespace sasd::ui;

namespace {

class CheckBoxMeasurementContext final : public MeasurementContext {
public:
    Size measureText(std::string_view) const override {
        ++text_calls_;
        return {4, 1};
    }

    Size measureCheckBox(std::string_view) const override {
        ++checkbox_calls_;
        return {12, 2};
    }

    std::uint64_t revision() const noexcept override { return 0; }

    [[nodiscard]] int textCalls() const noexcept { return text_calls_; }
    [[nodiscard]] int checkBoxCalls() const noexcept { return checkbox_calls_; }

private:
    mutable int text_calls_{0};
    mutable int checkbox_calls_{0};
};

} // namespace

TEST_CASE("CheckBox is focusable and stores caption plus initial checked state") {
    CheckBox unchecked{"Feature"};
    CheckBox checked{"Feature", true};

    CHECK(unchecked.isFocusable());
    CHECK(unchecked.canReceiveFocus());
    CHECK(unchecked.text() == "Feature");
    CHECK(!unchecked.isChecked());
    CHECK(checked.isChecked());
}

TEST_CASE("CheckBox caption changes invalidate measurement and presentation") {
    CheckBox check{"Old"};
    CheckBoxMeasurementContext context;
    (void)check.measure(context);
    check.acknowledgeVisualUpdate();

    check.setText("New");

    CHECK(check.text() == "New");
    CHECK(!check.isMeasureValid());
    CHECK(check.isVisualUpdatePending());
}

TEST_CASE("CheckBox checked state invalidates presentation but preserves measurement cache") {
    CheckBox check{"Option"};
    CheckBoxMeasurementContext context;
    (void)check.measure(context);
    check.acknowledgeVisualUpdate();

    CHECK(check.setChecked(true));
    CHECK(check.isChecked());
    CHECK(check.isMeasureValid());
    CHECK(check.isVisualUpdatePending());

    check.acknowledgeVisualUpdate();
    CHECK(!check.setChecked(true));
    CHECK(!check.isVisualUpdatePending());
}

TEST_CASE("CheckBox uses its control-specific measurement hook") {
    CheckBox check{"Measure"};
    CheckBoxMeasurementContext context;

    CHECK(check.measure(context) == Size{12, 2});
    CHECK(context.checkBoxCalls() == 1);
    CHECK(context.textCalls() == 0);
}

TEST_CASE("Programmatic CheckBox state changes notify even while disabled") {
    CheckBox check{"Option"};
    check.setEnabled(false);

    int notifications = 0;
    bool last_value = false;
    check.setOnCheckedChanged([&](bool checked) {
        ++notifications;
        last_value = checked;
    });

    CHECK(check.setChecked(true));
    CHECK(notifications == 1);
    CHECK(last_value);
    CHECK(check.isChecked());

    CHECK(!check.setChecked(true));
    CHECK(notifications == 1);
}

TEST_CASE("Focused CheckBox toggles on unmodified Space press only") {
    CheckBox check{"Option"};
    FocusManager focus;
    int notifications = 0;
    check.setOnCheckedChanged([&](bool) { ++notifications; });
    CHECK(focus.requestFocus(check));

    const auto press =
        EventDispatcher::dispatch(check, KeyEvent{Key::space, true, KeyModifier::none});
    CHECK(press.handled());
    CHECK(check.isChecked());
    CHECK(notifications == 1);

    const auto release =
        EventDispatcher::dispatch(check, KeyEvent{Key::space, false, KeyModifier::none});
    CHECK(release.handled());
    CHECK(check.isChecked());
    CHECK(notifications == 1);

    const auto enter =
        EventDispatcher::dispatch(check, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(!enter.handled());
    CHECK(check.isChecked());
}

TEST_CASE("CheckBox ignores keyboard toggle without focus or with modifiers") {
    CheckBox check{"Option"};
    CHECK(!EventDispatcher::dispatch(
        check, KeyEvent{Key::space, true, KeyModifier::none}).handled());
    CHECK(!check.isChecked());

    FocusManager focus;
    CHECK(focus.requestFocus(check));
    CHECK(!EventDispatcher::dispatch(
        check, KeyEvent{Key::space, true, KeyModifier::control}).handled());
    CHECK(!check.isChecked());
}

TEST_CASE("CheckBox primary pointer gesture toggles only on release inside") {
    Container root;
    root.arrange({0, 0, 200, 100});
    auto& check = root.emplace<CheckBox>("Pointer");
    check.arrange({20, 10, 100, 30});
    PointerRouter router;

    const auto press = router.route(
        root, PointerEvent{{30, 20}, PointerAction::press, PointerButton::primary, 1});
    CHECK(press.handled);
    CHECK(press.capture_active);
    CHECK(check.isPressed());
    CHECK(!check.isChecked());

    const auto release = router.route(
        root, PointerEvent{{40, 20}, PointerAction::release, PointerButton::primary, 1});
    CHECK(release.handled);
    CHECK(!release.capture_active);
    CHECK(!check.isPressed());
    CHECK(check.isChecked());
}

TEST_CASE("CheckBox captured release outside cancels toggle and clears pressed state") {
    Container root;
    root.arrange({0, 0, 200, 100});
    auto& check = root.emplace<CheckBox>("Pointer");
    check.arrange({20, 10, 100, 30});
    PointerRouter router;

    CHECK(router.route(
        root, PointerEvent{{30, 20}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(check.isPressed());

    const auto release = router.route(
        root, PointerEvent{{180, 90}, PointerAction::release, PointerButton::primary, 1});

    CHECK(release.handled);
    CHECK(!router.hasCapture());
    CHECK(!check.isPressed());
    CHECK(!check.isChecked());
}

TEST_CASE("CheckBox pressed state follows captured pointer leave and re-entry") {
    Container root;
    root.arrange({0, 0, 200, 100});
    auto& check = root.emplace<CheckBox>("Pointer");
    check.arrange({20, 10, 100, 30});
    PointerRouter router;

    CHECK(router.route(
        root, PointerEvent{{30, 20}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(check.isPressed());

    CHECK(router.route(
        root, PointerEvent{{180, 90}, PointerAction::move, PointerButton::none, 0}).handled);
    CHECK(!check.isPressed());

    CHECK(router.route(
        root, PointerEvent{{40, 20}, PointerAction::move, PointerButton::none, 0}).handled);
    CHECK(check.isPressed());

    router.releaseCapture();
    CHECK(!check.isPressed());
    CHECK(!check.isChecked());
}

TEST_CASE("Disabled CheckBox rejects user input without changing programmatic state") {
    Container root;
    root.arrange({0, 0, 160, 80});
    auto& check = root.emplace<CheckBox>("Disabled", true);
    check.arrange({10, 10, 100, 30});
    check.setEnabled(false);

    PointerRouter router;
    const auto pointer = router.route(
        root, PointerEvent{{20, 20}, PointerAction::press, PointerButton::primary, 1});
    CHECK(pointer.targeted);
    CHECK(!pointer.handled);
    CHECK(!router.hasCapture());
    CHECK(check.isChecked());

    const auto key =
        EventDispatcher::dispatch(check, KeyEvent{Key::space, true, KeyModifier::none});
    CHECK(!key.handled());
    CHECK(check.isChecked());
}

TEST_CASE("CheckBox checked callback can release the control from its owner") {
    auto owner = std::make_unique<Container>();
    auto& check = owner->emplace<CheckBox>("Self remove");

    std::unique_ptr<Component> released;
    check.setOnCheckedChanged([&](bool checked) {
        CHECK(checked);
        released = owner->release(check);
    });

    CHECK(check.setChecked(true));
    CHECK(released != nullptr);
    CHECK(released->owner() == nullptr);
}
