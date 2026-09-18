#include "test_framework.hpp"

#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/widget.hpp>

#include <functional>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

using namespace sasd::ui;

namespace {

class FocusRecordingWidget final : public Widget {
public:
    using FocusCallback = std::function<void(bool)>;

    explicit FocusRecordingWidget(FocusCallback callback = {}) : callback_{std::move(callback)} {}

    [[nodiscard]] const std::vector<bool>& focusEvents() const noexcept {
        return focus_events_;
    }

    [[nodiscard]] const std::vector<bool>& observedFocusStates() const noexcept {
        return observed_focus_states_;
    }

protected:
    EventResult onEvent(const Event& event) override {
        if (const auto* focus = std::get_if<FocusEvent>(&event)) {
            focus_events_.push_back(focus->gained);

            // FocusManager promises to update state before notifying the widget. Recording hasFocus()
            // here makes that ordering part of the executable contract rather than documentation only.
            observed_focus_states_.push_back(hasFocus());

            if (callback_) {
                callback_(focus->gained);
            }
        }

        return EventResult::ignored;
    }

private:
    FocusCallback callback_;
    std::vector<bool> focus_events_;
    std::vector<bool> observed_focus_states_;
};

} // namespace

TEST_CASE("Widgets are non-focusable by default and expose explicit local eligibility") {
    Widget widget;

    CHECK(!widget.isFocusable());
    CHECK(!widget.canReceiveFocus());
    CHECK(!widget.hasFocus());

    widget.setFocusable(true);
    CHECK(widget.isFocusable());
    CHECK(widget.canReceiveFocus());

    widget.setEnabled(false);
    CHECK(!widget.canReceiveFocus());

    widget.setEnabled(true);
    widget.setVisible(false);
    CHECK(!widget.canReceiveFocus());

    widget.setVisible(true);
    CHECK(widget.canReceiveFocus());
}

TEST_CASE("FocusManager grants focus and sends state-consistent focus notifications") {
    FocusManager focus;
    FocusRecordingWidget widget;
    widget.setFocusable(true);

    CHECK(focus.requestFocus(widget));
    CHECK(focus.focusedWidget() == &widget);
    CHECK(widget.hasFocus());
    CHECK(widget.focusEvents() == std::vector<bool>{true});
    CHECK(widget.observedFocusStates() == std::vector<bool>{true});

    CHECK(focus.clearFocus());
    CHECK(focus.focusedWidget() == nullptr);
    CHECK(!widget.hasFocus());
    CHECK(widget.focusEvents() == std::vector<bool>({true, false}));
    CHECK(widget.observedFocusStates() == std::vector<bool>({true, false}));

    CHECK(!focus.clearFocus());
}

TEST_CASE("FocusManager moves focus in lost-then-gained order") {
    FocusManager focus;
    std::vector<int> order;

    FocusRecordingWidget first{[&](bool gained) {
        order.push_back(gained ? 1 : -1);
    }};
    FocusRecordingWidget second{[&](bool gained) {
        order.push_back(gained ? 2 : -2);
    }};

    first.setFocusable(true);
    second.setFocusable(true);

    CHECK(focus.requestFocus(first));
    order.clear();

    CHECK(focus.requestFocus(second));

    CHECK(!first.hasFocus());
    CHECK(second.hasFocus());
    CHECK(focus.focusedWidget() == &second);
    CHECK(order == std::vector<int>({-1, 2}));
}

TEST_CASE("FocusManager rejects ineligible requests without disturbing existing focus") {
    FocusManager focus;
    FocusRecordingWidget current;
    FocusRecordingWidget disabled;
    FocusRecordingWidget hidden;

    current.setFocusable(true);
    disabled.setFocusable(true);
    disabled.setEnabled(false);
    hidden.setFocusable(true);
    hidden.setVisible(false);

    CHECK(focus.requestFocus(current));
    CHECK(!focus.requestFocus(disabled));
    CHECK(!focus.requestFocus(hidden));

    CHECK(focus.focusedWidget() == &current);
    CHECK(current.hasFocus());
    CHECK(disabled.focusEvents().empty());
    CHECK(hidden.focusEvents().empty());
}

TEST_CASE("Repeated focus requests are idempotent") {
    FocusManager focus;
    FocusRecordingWidget widget;
    widget.setFocusable(true);

    CHECK(focus.requestFocus(widget));
    CHECK(focus.requestFocus(widget));

    CHECK(widget.focusEvents() == std::vector<bool>{true});
}

TEST_CASE("Losing local focus eligibility clears current focus synchronously") {
    FocusManager focus;
    FocusRecordingWidget widget;
    widget.setFocusable(true);

    CHECK(focus.requestFocus(widget));
    widget.setEnabled(false);

    CHECK(focus.focusedWidget() == nullptr);
    CHECK(!widget.hasFocus());
    CHECK(widget.focusEvents() == std::vector<bool>({true, false}));

    widget.setEnabled(true);
    CHECK(focus.requestFocus(widget));
    widget.setVisible(false);

    CHECK(focus.focusedWidget() == nullptr);
    CHECK(!widget.hasFocus());

    widget.setVisible(true);
    CHECK(focus.requestFocus(widget));
    widget.setFocusable(false);

    CHECK(focus.focusedWidget() == nullptr);
    CHECK(!widget.hasFocus());
}

TEST_CASE("Destroying a focused widget invalidates the managers non-owning pointer") {
    FocusManager focus;
    auto widget = std::make_unique<FocusRecordingWidget>();
    widget->setFocusable(true);

    CHECK(focus.requestFocus(*widget));
    CHECK(focus.focusedWidget() == widget.get());

    widget.reset();

    CHECK(focus.focusedWidget() == nullptr);
}

TEST_CASE("Destroying FocusManager detaches the surviving widget without callbacks") {
    FocusRecordingWidget widget;
    widget.setFocusable(true);

    {
        FocusManager focus;
        CHECK(focus.requestFocus(widget));
        CHECK(widget.hasFocus());
        CHECK(widget.focusEvents() == std::vector<bool>{true});
    }

    CHECK(!widget.hasFocus());

    // Manager destruction is lifetime cleanup, not a semantic transition; no virtual event callback is
    // invoked from the destructor path.
    CHECK(widget.focusEvents() == std::vector<bool>{true});
}

TEST_CASE("A widget cannot be focused by two managers simultaneously") {
    FocusManager first;
    FocusManager second;
    FocusRecordingWidget widget;
    widget.setFocusable(true);

    CHECK(first.requestFocus(widget));
    CHECK(!second.requestFocus(widget));

    CHECK(first.focusedWidget() == &widget);
    CHECK(second.focusedWidget() == nullptr);
    CHECK(widget.hasFocus());
}

TEST_CASE("Nested focus requests from a focus-lost callback supersede the outer request") {
    FocusManager focus;
    FocusRecordingWidget third;
    third.setFocusable(true);

    FocusRecordingWidget first{[&](bool gained) {
        if (!gained) {
            (void)focus.requestFocus(third);
        }
    }};
    FocusRecordingWidget second;

    first.setFocusable(true);
    second.setFocusable(true);

    CHECK(focus.requestFocus(first));

    // Moving toward second first emits loss on first. That callback focuses third. The outer request
    // detects the nested transition and must not overwrite it with second.
    CHECK(!focus.requestFocus(second));
    CHECK(focus.focusedWidget() == &third);
    CHECK(third.hasFocus());
    CHECK(!second.hasFocus());
    CHECK(second.focusEvents().empty());
}
