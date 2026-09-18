#include "test_framework.hpp"

#include <sasd/ui/label.hpp>
#include <sasd/ui/window.hpp>

using namespace sasd::ui;

TEST_CASE("Label text changes invalidate measurement and presentation") {
    Label label{"first"};

    (void)label.measure();
    label.acknowledgeVisualUpdate();

    CHECK(label.isMeasureValid());
    CHECK(!label.isVisualUpdatePending());
    CHECK(label.text() == "first");

    label.setText("second");

    CHECK(label.text() == "second");
    CHECK(!label.isMeasureValid());
    CHECK(label.isVisualUpdatePending());
    CHECK(!label.isFocusable());
}

TEST_CASE("Assigning identical Label text preserves cached state") {
    Label label{"same"};

    (void)label.measure();
    label.acknowledgeVisualUpdate();

    label.setText("same");

    CHECK(label.isMeasureValid());
    CHECK(!label.isVisualUpdatePending());
}

TEST_CASE("Window owns visual children through the existing Container contract") {
    Window window;
    auto& label = window.emplace<Label>("Hello");

    CHECK(window.childCount() == 1);
    CHECK(&window.childAt(0) == &label);
    CHECK(label.parent() == &window);
    CHECK(label.owner() == &window);
    CHECK(!window.isFocusable());
}
