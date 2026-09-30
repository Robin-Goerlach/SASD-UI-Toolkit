#include "test_framework.hpp"

#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/radio_button.hpp>
#include <sasd/ui/radio_group.hpp>
#include <sasd/ui/radio_group_navigation.hpp>
#include <sasd/ui/vbox.hpp>

using namespace sasd::ui;

TEST_CASE("RadioGroupNavigation moves focus and selection forward with wrapping") {
    RadioGroup group;
    VBox root;
    auto& first = root.emplace<RadioButton>(group, "First");
    auto& second = root.emplace<RadioButton>(group, "Second");
    auto& third = root.emplace<RadioButton>(group, "Third");

    FocusManager focus;
    CHECK(first.setSelected(true));
    CHECK(focus.requestFocus(first));

    CHECK(RadioGroupNavigation::move(
        focus, first, RadioGroupNavigationDirection::next));
    CHECK(focus.focusedWidget() == &second);
    CHECK(!first.isSelected());
    CHECK(second.isSelected());

    CHECK(RadioGroupNavigation::move(
        focus, second, RadioGroupNavigationDirection::next));
    CHECK(focus.focusedWidget() == &third);
    CHECK(third.isSelected());

    CHECK(RadioGroupNavigation::move(
        focus, third, RadioGroupNavigationDirection::next));
    CHECK(focus.focusedWidget() == &first);
    CHECK(first.isSelected());
    CHECK(group.selectedButton() == &first);
}

TEST_CASE("RadioGroupNavigation moves backward and skips disabled hidden members") {
    RadioGroup group;
    VBox root;
    auto& first = root.emplace<RadioButton>(group, "First");
    auto& disabled = root.emplace<RadioButton>(group, "Disabled");
    auto& hidden = root.emplace<RadioButton>(group, "Hidden");
    auto& last = root.emplace<RadioButton>(group, "Last");

    disabled.setEnabled(false);
    hidden.setVisible(false);

    FocusManager focus;
    CHECK(focus.requestFocus(first));

    CHECK(RadioGroupNavigation::move(
        focus, first, RadioGroupNavigationDirection::previous));
    CHECK(focus.focusedWidget() == &last);
    CHECK(last.isSelected());

    CHECK(RadioGroupNavigation::move(
        focus, last, RadioGroupNavigationDirection::next));
    CHECK(focus.focusedWidget() == &first);
    CHECK(first.isSelected());
}

TEST_CASE("RadioGroupNavigation skips members below hidden or disabled ancestors") {
    RadioGroup group;
    VBox root;
    auto& first = root.emplace<RadioButton>(group, "First");

    auto& hidden_panel = root.emplace<VBox>();
    auto& hidden_member = hidden_panel.emplace<RadioButton>(group, "Hidden subtree");
    hidden_panel.setVisible(false);

    auto& disabled_panel = root.emplace<VBox>();
    auto& disabled_member = disabled_panel.emplace<RadioButton>(group, "Disabled subtree");
    disabled_panel.setEnabled(false);

    auto& last = root.emplace<RadioButton>(group, "Last");

    FocusManager focus;
    CHECK(focus.requestFocus(first));

    CHECK(RadioGroupNavigation::move(
        focus, first, RadioGroupNavigationDirection::next));
    CHECK(focus.focusedWidget() == &last);
    CHECK(!hidden_member.isSelected());
    CHECK(!disabled_member.isSelected());
}

TEST_CASE("RadioGroupNavigation does not cross top-level visual roots") {
    RadioGroup group;
    VBox first_root;
    VBox second_root;

    auto& first = first_root.emplace<RadioButton>(group, "First window");
    auto& second = second_root.emplace<RadioButton>(group, "Second window");

    FocusManager focus;
    CHECK(focus.requestFocus(first));

    CHECK(!RadioGroupNavigation::move(
        focus, first, RadioGroupNavigationDirection::next));
    CHECK(focus.focusedWidget() == &first);
    CHECK(!second.isSelected());
}

TEST_CASE("RadioGroupNavigation Arrow keys move selection and consume matching release") {
    RadioGroup group;
    VBox root;
    auto& first = root.emplace<RadioButton>(group, "First");
    auto& second = root.emplace<RadioButton>(group, "Second");

    FocusManager focus;
    CHECK(focus.requestFocus(first));

    CHECK(RadioGroupNavigation::handleEvent(
              focus,
              first,
              KeyEvent{Key::right, true, KeyModifier::none}) ==
          EventResult::handled);
    CHECK(focus.focusedWidget() == &second);
    CHECK(second.isSelected());

    /*
     * Key-up is delivered to the now-focused target by a normal event loop, so ask the utility with
     * second rather than first. It is consumed without another move.
     */
    CHECK(RadioGroupNavigation::handleEvent(
              focus,
              second,
              KeyEvent{Key::right, false, KeyModifier::none}) ==
          EventResult::handled);
    CHECK(focus.focusedWidget() == &second);
    CHECK(second.isSelected());
}

TEST_CASE("RadioGroupNavigation maps Up Left backward and Down Right forward") {
    RadioGroup group;
    VBox root;
    auto& first = root.emplace<RadioButton>(group, "First");
    auto& second = root.emplace<RadioButton>(group, "Second");
    auto& third = root.emplace<RadioButton>(group, "Third");

    FocusManager focus;
    CHECK(focus.requestFocus(second));

    CHECK(RadioGroupNavigation::handleEvent(
              focus,
              second,
              KeyEvent{Key::up, true, KeyModifier::none}) ==
          EventResult::handled);
    CHECK(focus.focusedWidget() == &first);

    CHECK(RadioGroupNavigation::handleEvent(
              focus,
              first,
              KeyEvent{Key::down, true, KeyModifier::none}) ==
          EventResult::handled);
    CHECK(focus.focusedWidget() == &second);

    CHECK(RadioGroupNavigation::handleEvent(
              focus,
              second,
              KeyEvent{Key::left, true, KeyModifier::none}) ==
          EventResult::handled);
    CHECK(focus.focusedWidget() == &first);

    CHECK(RadioGroupNavigation::handleEvent(
              focus,
              first,
              KeyEvent{Key::right, true, KeyModifier::none}) ==
          EventResult::handled);
    CHECK(focus.focusedWidget() == &second);

    CHECK(!third.isSelected());
}

TEST_CASE("RadioGroupNavigation ignores modified arrows standalone radios and isolated groups") {
    RadioGroup group;
    VBox root;
    auto& only = root.emplace<RadioButton>(group, "Only");
    RadioButton standalone{"Standalone"};

    FocusManager focus;
    CHECK(focus.requestFocus(only));

    CHECK(RadioGroupNavigation::handleEvent(
              focus,
              only,
              KeyEvent{Key::right, true, KeyModifier::control}) ==
          EventResult::ignored);
    CHECK(RadioGroupNavigation::handleEvent(
              focus,
              only,
              KeyEvent{Key::right, true, KeyModifier::none}) ==
          EventResult::ignored);

    CHECK(focus.requestFocus(standalone));
    CHECK(RadioGroupNavigation::handleEvent(
              focus,
              standalone,
              KeyEvent{Key::right, true, KeyModifier::none}) ==
          EventResult::ignored);
}

TEST_CASE("RadioGroupNavigation selection callback observes focus and coherent group state") {
    RadioGroup group;
    VBox root;
    auto& first = root.emplace<RadioButton>(group, "First");
    auto& second = root.emplace<RadioButton>(group, "Second");

    FocusManager focus;
    CHECK(first.setSelected(true));
    CHECK(focus.requestFocus(first));

    bool callback_saw_final_state = false;
    second.setOnSelected([&] {
        callback_saw_final_state =
            focus.focusedWidget() == &second &&
            group.selectedButton() == &second &&
            second.isSelected() &&
            !first.isSelected();
    });

    CHECK(RadioGroupNavigation::move(
        focus, first, RadioGroupNavigationDirection::next));
    CHECK(callback_saw_final_state);
}
