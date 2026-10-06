#include "test_framework.hpp"

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/events/event_dispatcher.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/measurement_context.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace sasd::ui;

namespace {

class ComboBoxMeasurementContext final : public MeasurementContext {
public:
    Size measureText(std::string_view text) const override {
        ++text_calls_;
        return {static_cast<Coordinate>(text.size()), 1};
    }

    Size measureComboBox(std::string_view text) const override {
        ++combo_calls_;
        /* Three logical units stand in for backend-owned border/drop-indicator chrome. */
        return {static_cast<Coordinate>(text.size() + 3U), 2};
    }

    std::uint64_t revision() const noexcept override { return 0; }

    [[nodiscard]] int textCalls() const noexcept { return text_calls_; }
    [[nodiscard]] int comboCalls() const noexcept { return combo_calls_; }

private:
    mutable int text_calls_{0};
    mutable int combo_calls_{0};
};

} // namespace

TEST_CASE("ComboBox owns items, starts unselected and is focusable") {
    std::vector<std::string> source{"One", "Two"};
    ComboBox combo{source};
    source[0] = "Changed outside";

    CHECK(combo.isFocusable());
    CHECK(combo.canReceiveFocus());
    CHECK(combo.itemCount() == 2U);
    CHECK(combo.itemAt(0U) == "One");
    CHECK(combo.itemAt(1U) == "Two");
    CHECK(!combo.selectedIndex().has_value());
    CHECK(!combo.selectedText().has_value());
}

TEST_CASE("ComboBox selection is optional, validated and notifies only on real changes") {
    ComboBox combo{{"One", "Two", "Three"}};
    std::vector<std::optional<std::size_t>> notifications;
    combo.setOnSelectionChanged(
        [&](std::optional<std::size_t> index) { notifications.push_back(index); });

    CHECK(combo.setSelectedIndex(1U));
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{1U});
    CHECK(combo.selectedText() == std::optional<std::string_view>{"Two"});
    CHECK(notifications.size() == 1U);
    CHECK(notifications.back() == std::optional<std::size_t>{1U});

    CHECK(!combo.setSelectedIndex(1U));
    CHECK(notifications.size() == 1U);

    CHECK(combo.clearSelection());
    CHECK(!combo.selectedIndex().has_value());
    CHECK(notifications.size() == 2U);
    CHECK(!notifications.back().has_value());

    bool threw = false;
    try {
        (void)combo.setSelectedIndex(3U);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(!combo.selectedIndex().has_value());
    CHECK(notifications.size() == 2U);
}

TEST_CASE("Replacing ComboBox items clears old numeric selection before notifying") {
    ComboBox combo{{"Old zero", "Old one"}};
    CHECK(combo.setSelectedIndex(1U));

    bool callback_saw_coherent_replacement = false;
    combo.setOnSelectionChanged([&](std::optional<std::size_t> index) {
        CHECK(!index.has_value());
        CHECK(combo.itemCount() == 1U);
        CHECK(combo.itemAt(0U) == "Replacement");
        CHECK(!combo.selectedText().has_value());
        callback_saw_coherent_replacement = true;
    });

    CHECK(combo.setItems({"Replacement"}));
    CHECK(callback_saw_coherent_replacement);
    CHECK(!combo.selectedIndex().has_value());

    /* Assigning identical replacement content is a structural no-op and must not notify again. */
    callback_saw_coherent_replacement = false;
    CHECK(!combo.setItems({"Replacement"}));
    CHECK(!callback_saw_coherent_replacement);
}

TEST_CASE("ComboBox measures the widest item through its control-specific measurement hook") {
    ComboBox empty;
    ComboBox combo{{"A", "Longest"}};
    ComboBoxMeasurementContext empty_context;
    ComboBoxMeasurementContext context;

    CHECK(empty.measure(empty_context) == Size{3, 2});
    CHECK(empty_context.comboCalls() == 1);
    CHECK(empty_context.textCalls() == 0);

    CHECK(combo.measure(context) == Size{10, 2});
    CHECK(context.comboCalls() == 3); // empty chrome baseline plus two item candidates
    CHECK(context.textCalls() == 0);
}

TEST_CASE("ComboBox selection preserves measurement cache while item append invalidates it") {
    ComboBox combo{{"Short", "Longer"}};
    ComboBoxMeasurementContext context;
    (void)combo.measure(context);
    combo.acknowledgeVisualUpdate();

    CHECK(combo.setSelectedIndex(0U));
    CHECK(combo.isMeasureValid());
    CHECK(combo.isVisualUpdatePending());

    combo.acknowledgeVisualUpdate();
    CHECK(combo.appendItem("A much longer choice") == 2U);
    CHECK(!combo.isMeasureValid());
    CHECK(combo.isVisualUpdatePending());

    CHECK(combo.measure(context).width ==
          static_cast<Coordinate>(std::string_view{"A much longer choice"}.size() + 3U));
}

TEST_CASE("Focused ComboBox navigates selection with non-wrapping arrow Home and End keys") {
    ComboBox combo{{"Zero", "One", "Two"}};
    FocusManager focus;
    std::size_t notifications = 0U;
    combo.setOnSelectionChanged([&](std::optional<std::size_t>) { ++notifications; });
    CHECK(focus.requestFocus(combo));

    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::down, true, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{0U});

    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::down, true, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{1U});

    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::end, true, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{2U});

    /* The end of the list clamps instead of wrapping to the first item. */
    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::down, true, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{2U});
    CHECK(notifications == 3U);

    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::up, true, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{1U});

    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::home, true, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{0U});

    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::up, true, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{0U});
    CHECK(notifications == 5U);

    /* Desktop key-up is consumed but never performs a second navigation step. */
    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::down, false, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{0U});
    CHECK(notifications == 5U);
}

TEST_CASE("ComboBox Up or End chooses the last item when selection is initially empty") {
    ComboBox combo{{"Zero", "One", "Two"}};
    FocusManager focus;
    CHECK(focus.requestFocus(combo));

    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::up, true, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{2U});

    CHECK(combo.clearSelection());
    CHECK(EventDispatcher::dispatch(
              combo, KeyEvent{Key::end, true, KeyModifier::none}).handled());
    CHECK(combo.selectedIndex() == std::optional<std::size_t>{2U});
}

TEST_CASE("ComboBox navigation requires focus visible enabled state and unmodified keys") {
    ComboBox combo{{"Zero", "One"}};

    CHECK(!EventDispatcher::dispatch(
        combo, KeyEvent{Key::down, true, KeyModifier::none}).handled());
    CHECK(!combo.selectedIndex().has_value());

    FocusManager focus;
    CHECK(focus.requestFocus(combo));
    CHECK(!EventDispatcher::dispatch(
        combo, KeyEvent{Key::down, true, KeyModifier::control}).handled());
    CHECK(!combo.selectedIndex().has_value());

    combo.setEnabled(false);
    CHECK(!combo.hasFocus());
    CHECK(!EventDispatcher::dispatch(
        combo, KeyEvent{Key::down, true, KeyModifier::none}).handled());
    CHECK(!combo.selectedIndex().has_value());
}

TEST_CASE("ComboBox selection callback may release the control from its owner") {
    auto owner = std::make_unique<Container>();
    auto& combo = owner->emplace<ComboBox>(std::vector<std::string>{"One", "Two"});

    std::unique_ptr<Component> released;
    combo.setOnSelectionChanged([&](std::optional<std::size_t> index) {
        CHECK(index == std::optional<std::size_t>{0U});
        released = owner->release(combo);
    });

    CHECK(combo.setSelectedIndex(0U));
    CHECK(released != nullptr);
    CHECK(released->owner() == nullptr);
}
