#include "test_framework.hpp"

#include <sasd/ui/container.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/pointer_router.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

using namespace sasd::ui;

namespace {

class RadioMeasurementContext final : public MeasurementContext {
public:
    Size measureText(std::string_view) const override {
        ++text_calls_;
        return {4, 1};
    }

    Size measureRadioButton(std::string_view) const override {
        ++radio_calls_;
        return {13, 2};
    }

    std::uint64_t revision() const noexcept override { return 0; }

    [[nodiscard]] int textCalls() const noexcept { return text_calls_; }
    [[nodiscard]] int radioCalls() const noexcept { return radio_calls_; }

private:
    mutable int text_calls_{0};
    mutable int radio_calls_{0};
};

} // namespace

TEST_CASE("RadioButton is focusable and uses radio-specific measurement") {
    RadioButton radio{"Choice"};
    RadioMeasurementContext metrics;

    CHECK(radio.isFocusable());
    CHECK(radio.canReceiveFocus());
    CHECK(radio.text() == "Choice");
    CHECK(!radio.isSelected());

    CHECK(radio.measure(metrics) == Size{13, 2});
    CHECK(metrics.radioCalls() == 1);
    CHECK(metrics.textCalls() == 0);
}

TEST_CASE("RadioGroup enforces at most one selected member") {
    RadioGroup group;
    RadioButton first{group, "First"};
    RadioButton second{group, "Second"};

    CHECK(group.memberCount() == 2);
    CHECK(group.selectedButton() == nullptr);

    CHECK(first.setSelected(true));
    CHECK(first.isSelected());
    CHECK(!second.isSelected());
    CHECK(group.selectedButton() == &first);

    CHECK(second.setSelected(true));
    CHECK(!first.isSelected());
    CHECK(second.isSelected());
    CHECK(group.selectedButton() == &second);

    // Selecting the current choice is a semantic no-op.
    CHECK(!second.setSelected(true));
}

TEST_CASE("RadioGroup permits explicit no-selection state") {
    RadioGroup group;
    RadioButton radio{group, "Choice"};

    CHECK(radio.setSelected(true));
    CHECK(group.selectedButton() == &radio);

    CHECK(radio.setSelected(false));
    CHECK(!radio.isSelected());
    CHECK(group.selectedButton() == nullptr);

    CHECK(!radio.setSelected(false));
}

TEST_CASE("RadioButton selected callback fires only for false-to-true transition") {
    RadioGroup group;
    RadioButton first{group, "First"};
    RadioButton second{group, "Second"};

    int first_selected = 0;
    int second_selected = 0;
    first.setOnSelected([&] { ++first_selected; });
    second.setOnSelected([&] { ++second_selected; });

    CHECK(first.setSelected(true));
    CHECK(first_selected == 1);
    CHECK(second_selected == 0);

    CHECK(second.setSelected(true));
    CHECK(first_selected == 1);
    CHECK(second_selected == 1);

    // Automatic deselection of First is intentionally not another "new choice" callback.
    CHECK(!first.isSelected());
}

TEST_CASE("RadioButton selection callback observes coherent group state") {
    RadioGroup group;
    RadioButton first{group, "First"};
    RadioButton second{group, "Second"};
    CHECK(first.setSelected(true));

    bool callback_saw_coherent_state = false;
    second.setOnSelected([&] {
        callback_saw_coherent_state =
            group.selectedButton() == &second &&
            second.isSelected() &&
            !first.isSelected();
    });

    CHECK(second.setSelected(true));
    CHECK(callback_saw_coherent_state);
}

TEST_CASE("RadioButton selected callback may release itself from visual ownership") {
    RadioGroup group;
    auto owner = std::make_unique<Container>();
    auto& radio = owner->emplace<RadioButton>(group, "Self remove");

    std::unique_ptr<Component> released;
    radio.setOnSelected([&] {
        released = owner->release(radio);
    });

    CHECK(radio.setSelected(true));
    CHECK(released != nullptr);
    CHECK(group.selectedButton() == &radio);
    CHECK(radio.owner() == nullptr);

    /*
     * Visual ownership and radio grouping are independent relationships. Releasing from a Container
     * must not silently alter the semantic choice group.
     */
    released.reset();
    CHECK(group.selectedButton() == nullptr);
    CHECK(group.memberCount() == 0);
}

TEST_CASE("Destroying RadioButton retires membership and selected pointer") {
    RadioGroup group;

    {
        RadioButton radio{group, "Temporary"};
        CHECK(radio.setSelected(true));
        CHECK(group.memberCount() == 1);
        CHECK(group.selectedButton() == &radio);
    }

    CHECK(group.memberCount() == 0);
    CHECK(group.selectedButton() == nullptr);
}

TEST_CASE("Destroying RadioGroup leaves surviving RadioButton safely standalone") {
    auto group = std::make_unique<RadioGroup>();
    auto radio = std::make_unique<RadioButton>(*group, "Survivor");

    CHECK(radio->setSelected(true));
    CHECK(radio->group() == group.get());

    group.reset();

    CHECK(radio->group() == nullptr);
    CHECK(radio->isSelected());

    // The surviving radio continues to support ordinary programmatic state updates.
    CHECK(radio->setSelected(false));
    CHECK(!radio->isSelected());
}

TEST_CASE("Focused RadioButton selects on Space and never toggles off from user input") {
    RadioGroup group;
    RadioButton radio{group, "Choice"};
    FocusManager focus;
    int selected_count = 0;
    radio.setOnSelected([&] { ++selected_count; });

    CHECK(focus.requestFocus(radio));

    CHECK(EventDispatcher::dispatch(
        radio,
        KeyEvent{Key::space, true, KeyModifier::none}).handled());
    CHECK(radio.isSelected());
    CHECK(selected_count == 1);

    CHECK(EventDispatcher::dispatch(
        radio,
        KeyEvent{Key::space, false, KeyModifier::none}).handled());
    CHECK(radio.isSelected());
    CHECK(selected_count == 1);

    CHECK(EventDispatcher::dispatch(
        radio,
        KeyEvent{Key::space, true, KeyModifier::none}).handled());
    CHECK(radio.isSelected());
    CHECK(selected_count == 1);
}

TEST_CASE("RadioButton ignores modified Space and Enter in initial contract") {
    RadioButton radio{"Choice"};
    FocusManager focus;
    CHECK(focus.requestFocus(radio));

    CHECK(!EventDispatcher::dispatch(
        radio,
        KeyEvent{Key::space, true, KeyModifier::control}).handled());
    CHECK(!EventDispatcher::dispatch(
        radio,
        KeyEvent{Key::enter, true, KeyModifier::none}).handled());
    CHECK(!radio.isSelected());
}

TEST_CASE("RadioButton pointer release inside selects once and release outside cancels") {
    RadioGroup group;
    Container root;
    root.arrange({0, 0, 200, 100});

    auto& radio = root.emplace<RadioButton>(group, "Pointer");
    radio.arrange({20, 10, 100, 30});

    PointerRouter router;
    int selected_count = 0;
    radio.setOnSelected([&] { ++selected_count; });

    CHECK(router.route(
        root,
        PointerEvent{{30, 20}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(radio.isPressed());

    CHECK(router.route(
        root,
        PointerEvent{{180, 90}, PointerAction::release, PointerButton::primary, 1}).handled);
    CHECK(!radio.isPressed());
    CHECK(!radio.isSelected());
    CHECK(selected_count == 0);

    CHECK(router.route(
        root,
        PointerEvent{{30, 20}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(router.route(
        root,
        PointerEvent{{40, 20}, PointerAction::release, PointerButton::primary, 1}).handled);

    CHECK(radio.isSelected());
    CHECK(selected_count == 1);

    // A second complete user gesture on an already selected radio remains selected and emits nothing.
    CHECK(router.route(
        root,
        PointerEvent{{30, 20}, PointerAction::press, PointerButton::primary, 1}).handled);
    CHECK(router.route(
        root,
        PointerEvent{{40, 20}, PointerAction::release, PointerButton::primary, 1}).handled);
    CHECK(radio.isSelected());
    CHECK(selected_count == 1);
}

TEST_CASE("Disabled RadioButton rejects user selection but permits programmatic state") {
    RadioButton radio{"Disabled"};
    radio.arrange({0, 0, 100, 30});
    radio.setEnabled(false);

    CHECK(!EventDispatcher::dispatch(
        radio,
        KeyEvent{Key::space, true, KeyModifier::none}).handled());
    CHECK(!radio.isSelected());

    CHECK(radio.setSelected(true));
    CHECK(radio.isSelected());
}

TEST_CASE("RadioButton selection invalidates presentation but preserves measurement cache") {
    RadioButton radio{"Choice"};
    RadioMeasurementContext metrics;

    (void)radio.measure(metrics);
    radio.acknowledgeVisualUpdate();

    CHECK(radio.setSelected(true));
    CHECK(radio.isMeasureValid());
    CHECK(radio.isVisualUpdatePending());
}
