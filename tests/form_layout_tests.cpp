#include "test_framework.hpp"

#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/measurement_context.hpp>

#include <cstdint>
#include <stdexcept>
#include <string_view>

using namespace sasd::ui;

namespace {

class FixedFormTextContext final : public MeasurementContext {
public:
    explicit FixedFormTextContext(Size size) : size_{size} {}

    Size measureText(std::string_view) const override {
        ++calls_;
        return size_;
    }

    std::uint64_t revision() const noexcept override { return 0; }
    [[nodiscard]] int calls() const noexcept { return calls_; }

private:
    Size size_{};
    mutable int calls_{0};
};

void setPreferred(Widget& widget, Size size) {
    widget.setSizeConstraints({{0, 0}, size, {1000, 1000}});
}

} // namespace

TEST_CASE("FormLayout measures shared label and field columns with row maxima") {
    FormLayout form;
    form.setColumnSpacing(2);
    form.setRowSpacing(1);

    auto& label_a = form.emplace<Widget>();
    auto& field_a = form.emplace<Widget>();
    auto& label_b = form.emplace<Widget>();
    auto& field_b = form.emplace<Widget>();

    setPreferred(label_a, {3, 1});
    setPreferred(field_a, {5, 2});
    setPreferred(label_b, {7, 3});
    setPreferred(field_b, {4, 1});

    // label=7, gap=2, field=5; row heights 2 and 3 with one row gap.
    CHECK(form.measure() == Size{14, 6});
    CHECK(form.rowCount() == 2);
}

TEST_CASE("FormLayout keeps label intrinsic width and expands field column") {
    FormLayout form;
    form.setColumnSpacing(2);
    form.setRowSpacing(1);

    auto& label_a = form.emplace<Widget>();
    auto& field_a = form.emplace<Widget>();
    auto& label_b = form.emplace<Widget>();
    auto& field_b = form.emplace<Widget>();

    setPreferred(label_a, {3, 1});
    setPreferred(field_a, {5, 2});
    setPreferred(label_b, {7, 3});
    setPreferred(field_b, {4, 1});

    (void)form.measure();
    form.arrange({11, 13, 20, 10});

    /*
     * The common label track keeps its seven-unit intrinsic width. After the two-unit gap the field
     * track receives all eleven remaining units, which is the form-specific behavior GridLayout does
     * not provide yet.
     */
    CHECK(label_a.bounds() == Rect{0, 0, 7, 2});
    CHECK(field_a.bounds() == Rect{9, 0, 11, 2});
    CHECK(label_b.bounds() == Rect{0, 3, 7, 3});
    CHECK(field_b.bounds() == Rect{9, 3, 11, 3});
}

TEST_CASE("FormLayout pairing remains structural when one cell is hidden") {
    FormLayout form;
    form.setColumnSpacing(1);

    auto& first_label = form.emplace<Widget>();
    auto& first_field = form.emplace<Widget>();
    auto& second_label = form.emplace<Widget>();
    auto& second_field = form.emplace<Widget>();

    setPreferred(first_label, {20, 1});
    setPreferred(first_field, {4, 2});
    setPreferred(second_label, {6, 3});
    setPreferred(second_field, {5, 1});

    first_label.setVisible(false);

    /*
     * Hiding child 0 must not turn child 1 into the next row's label. The first field remains in the
     * field column; label width is determined only by the still-visible second label.
     */
    CHECK(form.measure() == Size{12, 5});
    form.arrange({0, 0, 16, 5});

    CHECK(first_field.bounds() == Rect{7, 0, 9, 2});
    CHECK(second_label.bounds() == Rect{0, 2, 6, 3});
    CHECK(second_field.bounds() == Rect{7, 2, 9, 3});
}

TEST_CASE("FormLayout collapses a row only when both structural cells are hidden") {
    FormLayout form;
    form.setRowSpacing(2);

    auto& a_label = form.emplace<Widget>();
    auto& a_field = form.emplace<Widget>();
    auto& b_label = form.emplace<Widget>();
    auto& b_field = form.emplace<Widget>();
    auto& c_label = form.emplace<Widget>();
    auto& c_field = form.emplace<Widget>();

    for (Widget* widget : {&a_label, &a_field, &b_label, &b_field, &c_label, &c_field}) {
        setPreferred(*widget, {2, 1});
    }

    b_label.setVisible(false);
    b_field.setVisible(false);

    CHECK(form.measure() == Size{5, 4});
    form.arrange({0, 0, 10, 4});

    CHECK(a_label.bounds().y == 0);
    CHECK(c_label.bounds().y == 3);
}

TEST_CASE("FormLayout emplaceRow creates stable Label field pairs") {
    FormLayout form;
    form.setColumnSpacing(1);

    auto first = form.emplaceRow<Widget>("Name");
    auto second = form.emplaceRow<Widget>("Code");
    setPreferred(first.field, {6, 2});
    setPreferred(second.field, {3, 1});

    FixedFormTextContext context{{4, 1}};
    CHECK(form.measure(context) == Size{11, 3});
    CHECK(context.calls() == 2);
    CHECK(form.childCount() == 4);
    CHECK(form.rowCount() == 2);
    CHECK(first.label.text() == "Name");
    CHECK(second.label.text() == "Code");
}

TEST_CASE("FormLayout clips label then field and later rows deterministically") {
    FormLayout form;
    form.setColumnSpacing(1);
    form.setRowSpacing(1);

    auto& label_a = form.emplace<Widget>();
    auto& field_a = form.emplace<Widget>();
    auto& label_b = form.emplace<Widget>();
    auto& field_b = form.emplace<Widget>();

    setPreferred(label_a, {5, 3});
    setPreferred(field_a, {8, 3});
    setPreferred(label_b, {5, 5});
    setPreferred(field_b, {8, 5});

    (void)form.measure();
    form.arrange({0, 0, 7, 5});

    // Label gets five, gap gets one, field gets the one remaining column.
    CHECK(label_a.bounds() == Rect{0, 0, 5, 3});
    CHECK(field_a.bounds() == Rect{6, 0, 1, 3});
    // After first row and spacing, only one line remains for the second row.
    CHECK(label_b.bounds() == Rect{0, 4, 5, 1});
    CHECK(field_b.bounds() == Rect{6, 4, 1, 1});
}

TEST_CASE("FormLayout spacing changes invalidate measurement and reject negatives") {
    FormLayout form;
    form.emplace<Widget>();
    form.emplace<Widget>();

    (void)form.measure();
    CHECK(form.isMeasureValid());

    form.setColumnSpacing(3);
    CHECK(!form.isMeasureValid());
    (void)form.measure();

    form.setRowSpacing(2);
    CHECK(!form.isMeasureValid());

    bool column_threw = false;
    try {
        form.setColumnSpacing(-1);
    } catch (const std::invalid_argument&) {
        column_threw = true;
    }

    bool row_threw = false;
    try {
        form.setRowSpacing(-1);
    } catch (const std::invalid_argument&) {
        row_threw = true;
    }

    CHECK(column_threw);
    CHECK(row_threw);
    CHECK(form.columnSpacing() == 3);
    CHECK(form.rowSpacing() == 2);
}

TEST_CASE("FormLayout supports an odd final label-only row") {
    FormLayout form;
    form.setColumnSpacing(1);
    form.setRowSpacing(2);

    auto& first_label = form.emplace<Widget>();
    auto& first_field = form.emplace<Widget>();
    auto& final_label = form.emplace<Widget>();

    setPreferred(first_label, {3, 1});
    setPreferred(first_field, {4, 2});
    setPreferred(final_label, {7, 3});

    /*
     * The unmatched final child is intentionally a label-only row. It still contributes to the
     * shared label width and row height, but no synthetic field cell or column spacing is invented
     * for that row. Column spacing remains a relation between globally visible label/field tracks.
     */
    CHECK(form.rowCount() == 2);
    CHECK(form.measure() == Size{12, 7});

    form.arrange({0, 0, 20, 7});
    CHECK(first_label.bounds() == Rect{0, 0, 7, 2});
    CHECK(first_field.bounds() == Rect{8, 0, 12, 2});
    CHECK(final_label.bounds() == Rect{0, 4, 7, 3});
}

TEST_CASE("FormLayout MeasurementContext skips hidden text cells without repacking rows") {
    FormLayout form;
    form.setColumnSpacing(1);

    auto first = form.emplaceRow<Widget>("Hidden label");
    auto second = form.emplaceRow<Widget>("Visible label");
    setPreferred(first.field, {5, 2});
    setPreferred(second.field, {3, 1});
    first.label.setVisible(false);

    FixedFormTextContext context{{4, 1}};

    /*
     * A hidden Label must not consume backend text-measurement work. The row itself remains active
     * because its field is visible, and the second structural pair stays the second row rather than
     * being repacked around the hidden cell.
     */
    CHECK(form.measure(context) == Size{10, 3});
    CHECK(context.calls() == 1);
}

TEST_CASE("FormLayout release and re-adopt follow current visual adoption order") {
    FormLayout form;

    auto& first_label = form.emplace<Widget>();
    auto& first_field = form.emplace<Widget>();
    auto& second_label = form.emplace<Widget>();
    auto& second_field = form.emplace<Widget>();

    auto released = form.release(first_label);

    CHECK(released.get() == &first_label);
    CHECK(first_label.owner() == nullptr);
    CHECK(first_label.parent() == nullptr);
    CHECK(form.childCount() == 3);
    CHECK(form.rowCount() == 2);

    /*
     * FormLayout does not maintain hidden row identities outside the Container child order. Removing
     * a visual child is therefore a structural edit: the remaining children close the gap. Re-adopt
     * appends the component at the end, which makes the new order explicit and avoids stale pairing
     * metadata that could disagree with Container ownership.
     */
    CHECK(&form.childAt(0) == &first_field);
    CHECK(&form.childAt(1) == &second_label);
    CHECK(&form.childAt(2) == &second_field);

    form.adopt(std::move(released));

    CHECK(form.childCount() == 4);
    CHECK(form.rowCount() == 2);
    CHECK(&form.childAt(0) == &first_field);
    CHECK(&form.childAt(1) == &second_label);
    CHECK(&form.childAt(2) == &second_field);
    CHECK(&form.childAt(3) == &first_label);
    CHECK(first_label.owner() == &form);
    CHECK(first_label.parent() == &form);
}
