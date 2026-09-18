#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/measurement_context.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

using namespace sasd::ui;

namespace {

class ButtonMeasurementContext final : public MeasurementContext {
public:
    Size measureText(std::string_view) const override {
        ++text_calls_;
        return {3, 1};
    }

    Size measureButton(std::string_view) const override {
        ++button_calls_;
        return {9, 2};
    }

    std::uint64_t revision() const noexcept override { return 0; }

    [[nodiscard]] int textCalls() const noexcept { return text_calls_; }
    [[nodiscard]] int buttonCalls() const noexcept { return button_calls_; }

private:
    mutable int text_calls_{0};
    mutable int button_calls_{0};
};

} // namespace

TEST_CASE("Button is focusable by default and stores UTF-8 caption") {
    Button button{"Run"};

    CHECK(button.isFocusable());
    CHECK(button.canReceiveFocus());
    CHECK(button.text() == "Run");
}

TEST_CASE("Button caption changes invalidate measurement and presentation") {
    Button button{"Old"};
    ButtonMeasurementContext context;

    (void)button.measure(context);
    button.acknowledgeVisualUpdate();

    button.setText("New");

    CHECK(button.text() == "New");
    CHECK(!button.isMeasureValid());
    CHECK(button.isVisualUpdatePending());
}

TEST_CASE("Assigning identical Button caption preserves cached state") {
    Button button{"Same"};
    ButtonMeasurementContext context;

    (void)button.measure(context);
    button.acknowledgeVisualUpdate();

    button.setText("Same");

    CHECK(button.isMeasureValid());
    CHECK(!button.isVisualUpdatePending());
}

TEST_CASE("Button text style invalidates presentation without invalidating measurement") {
    Button button{"Styled"};
    ButtonMeasurementContext context;

    (void)button.measure(context);
    button.acknowledgeVisualUpdate();

    TextStyle style;
    style.foreground = Color::cyan;
    style.bold = true;
    button.setTextStyle(style);

    CHECK(button.textStyle() == style);
    CHECK(button.isMeasureValid());
    CHECK(button.isVisualUpdatePending());

    button.acknowledgeVisualUpdate();
    button.setTextStyle(style);
    CHECK(!button.isVisualUpdatePending());
}

TEST_CASE("Button uses control-specific MeasurementContext hook") {
    Button button{"Measure"};
    ButtonMeasurementContext context;

    CHECK(button.measure(context) == Size{9, 2});
    CHECK(context.buttonCalls() == 1);
    CHECK(context.textCalls() == 0);
}

TEST_CASE("Focused Button activates on unmodified Enter press") {
    Button button{"OK"};
    FocusManager focus;
    int activations = 0;
    button.setOnActivated([&] { ++activations; });

    CHECK(focus.requestFocus(button));

    const auto pressed =
        EventDispatcher::dispatch(button, KeyEvent{Key::enter, true, KeyModifier::none});
    CHECK(pressed.handled());
    CHECK(pressed.handler == &button);
    CHECK(activations == 1);

    const auto released =
        EventDispatcher::dispatch(button, KeyEvent{Key::enter, false, KeyModifier::none});
    CHECK(released.handled());
    CHECK(activations == 1);
}

TEST_CASE("Focused Button activates on Space press") {
    Button button{"OK"};
    FocusManager focus;
    int activations = 0;
    button.setOnActivated([&] { ++activations; });

    CHECK(focus.requestFocus(button));

    const auto result =
        EventDispatcher::dispatch(button, KeyEvent{Key::space, true, KeyModifier::none});

    CHECK(result.handled());
    CHECK(activations == 1);
}

TEST_CASE("Button ignores activation keys when it does not own focus") {
    Button button{"OK"};
    int activations = 0;
    button.setOnActivated([&] { ++activations; });

    const auto result =
        EventDispatcher::dispatch(button, KeyEvent{Key::enter, true, KeyModifier::none});

    CHECK(!result.handled());
    CHECK(activations == 0);
}

TEST_CASE("Button leaves modified activation keys available for parent shortcuts") {
    Button button{"OK"};
    FocusManager focus;
    int activations = 0;
    button.setOnActivated([&] { ++activations; });

    CHECK(focus.requestFocus(button));

    const auto result =
        EventDispatcher::dispatch(button, KeyEvent{Key::enter, true, KeyModifier::control});

    CHECK(!result.handled());
    CHECK(activations == 0);
}

TEST_CASE("Disabled Button rejects programmatic and keyboard activation") {
    Button button{"OK"};
    FocusManager focus;
    int activations = 0;
    button.setOnActivated([&] { ++activations; });

    button.setEnabled(false);

    CHECK(!button.activate());
    CHECK(!focus.requestFocus(button));

    const auto result =
        EventDispatcher::dispatch(button, KeyEvent{Key::space, true, KeyModifier::none});

    CHECK(!result.handled());
    CHECK(activations == 0);
}

TEST_CASE("Button activation callback can release the Button from its owner") {
    auto owner = std::make_unique<Container>();
    auto& button = owner->emplace<Button>("Self remove");
    FocusManager focus;

    CHECK(focus.requestFocus(button));

    std::unique_ptr<Component> released;
    button.setOnActivated([&] {
        released = owner->release(button);
    });

    const auto result =
        EventDispatcher::dispatch(button, KeyEvent{Key::enter, true, KeyModifier::none});

    CHECK(result.handled());
    CHECK(released != nullptr);
    CHECK(released->owner() == nullptr);
}
