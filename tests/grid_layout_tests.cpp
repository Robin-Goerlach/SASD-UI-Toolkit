#include "test_framework.hpp"

#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/measurement_context.hpp>

#include <cstdint>
#include <stdexcept>
#include <string_view>

using namespace sasd::ui;

namespace {

class FixedGridTextContext final : public MeasurementContext {
public:
    explicit FixedGridTextContext(Size size) : size_{size} {}

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

TEST_CASE("GridLayout measures per-column maxima and per-row maxima") {
    GridLayout grid{2};
    grid.setColumnSpacing(1);
    grid.setRowSpacing(2);

    auto& a = grid.emplace<Widget>();
    auto& b = grid.emplace<Widget>();
    auto& c = grid.emplace<Widget>();
    setPreferred(a, {3, 2});
    setPreferred(b, {5, 1});
    setPreferred(c, {4, 6});

    /*
     * column widths: max(3,4)=4 and 5 => 4 + 1 + 5 = 10
     * row heights: max(2,1)=2 and 6 => 2 + 2 + 6 = 10
     */
    CHECK(grid.measure() == Size{10, 10});
}

TEST_CASE("GridLayout assigns visible children row-major and collapses hidden slots") {
    GridLayout grid{2};

    auto& first = grid.emplace<Widget>();
    auto& hidden = grid.emplace<Widget>();
    auto& second = grid.emplace<Widget>();
    setPreferred(first, {2, 1});
    setPreferred(hidden, {50, 50});
    setPreferred(second, {4, 3});
    hidden.setVisible(false);

    CHECK(grid.measure() == Size{6, 3});
    grid.arrange({7, 9, 10, 5});

    CHECK(first.bounds() == Rect{0, 0, 2, 3});
    CHECK(second.bounds() == Rect{2, 0, 4, 3});
}

TEST_CASE("GridLayout stretches children to complete intrinsic cells") {
    GridLayout grid{2};
    grid.setColumnSpacing(1);
    grid.setRowSpacing(1);

    auto& a = grid.emplace<Widget>();
    auto& b = grid.emplace<Widget>();
    auto& c = grid.emplace<Widget>();
    auto& d = grid.emplace<Widget>();

    setPreferred(a, {2, 1});
    setPreferred(b, {5, 3});
    setPreferred(c, {4, 2});
    setPreferred(d, {1, 6});

    (void)grid.measure();
    grid.arrange({20, 30, 20, 20});

    // Tracks are [4,5] x [3,6]; children stretch inside their complete cell.
    CHECK(a.bounds() == Rect{0, 0, 4, 3});
    CHECK(b.bounds() == Rect{5, 0, 5, 3});
    CHECK(c.bounds() == Rect{0, 4, 4, 6});
    CHECK(d.bounds() == Rect{5, 4, 5, 6});
}

TEST_CASE("GridLayout clips later tracks deterministically when final space is small") {
    GridLayout grid{2};
    grid.setColumnSpacing(1);
    grid.setRowSpacing(1);

    auto& a = grid.emplace<Widget>();
    auto& b = grid.emplace<Widget>();
    auto& c = grid.emplace<Widget>();
    auto& d = grid.emplace<Widget>();

    setPreferred(a, {4, 3});
    setPreferred(b, {5, 3});
    setPreferred(c, {4, 6});
    setPreferred(d, {5, 6});

    (void)grid.measure();
    grid.arrange({0, 0, 7, 6});

    // Earlier tracks retain their intrinsic allocation first, like the initial Box layout contract.
    CHECK(a.bounds() == Rect{0, 0, 4, 3});
    CHECK(b.bounds() == Rect{5, 0, 2, 3});
    CHECK(c.bounds() == Rect{0, 4, 4, 2});
    CHECK(d.bounds() == Rect{5, 4, 2, 2});
}

TEST_CASE("GridLayout arranges fully exhausted later rows to zero extent") {
    GridLayout grid{1};
    grid.setRowSpacing(1);

    auto& first = grid.emplace<Widget>();
    auto& second = grid.emplace<Widget>();
    setPreferred(first, {3, 5});
    setPreferred(second, {3, 5});

    (void)grid.measure();
    grid.arrange({0, 0, 8, 4});

    CHECK(first.bounds() == Rect{0, 0, 3, 4});
    CHECK(second.bounds() == Rect{0, 4, 3, 0});
}

TEST_CASE("GridLayout forwards MeasurementContext to visible children") {
    GridLayout grid{2};
    grid.emplace<Label>("A");
    grid.emplace<Label>("B");
    grid.emplace<Label>("C");

    FixedGridTextContext context{{6, 2}};

    CHECK(grid.measure(context) == Size{12, 4});
    CHECK(context.calls() == 3);
}

TEST_CASE("GridLayout property changes invalidate measurement") {
    GridLayout grid{2};
    auto& a = grid.emplace<Widget>();
    auto& b = grid.emplace<Widget>();
    setPreferred(a, {2, 1});
    setPreferred(b, {2, 1});

    CHECK(grid.measure() == Size{4, 1});
    CHECK(grid.isMeasureValid());

    grid.setColumnCount(1);
    CHECK(!grid.isMeasureValid());
    CHECK(grid.measure() == Size{2, 2});

    grid.setColumnSpacing(3);
    CHECK(!grid.isMeasureValid());
    (void)grid.measure();

    grid.setRowSpacing(4);
    CHECK(!grid.isMeasureValid());
}

TEST_CASE("GridLayout rejects invalid column count and negative spacing") {
    bool constructor_threw = false;
    try {
        GridLayout invalid{0};
    } catch (const std::invalid_argument&) {
        constructor_threw = true;
    }
    CHECK(constructor_threw);

    GridLayout grid{2};
    grid.setColumnSpacing(2);
    grid.setRowSpacing(3);

    bool columns_threw = false;
    try {
        grid.setColumnCount(0);
    } catch (const std::invalid_argument&) {
        columns_threw = true;
    }

    bool column_spacing_threw = false;
    try {
        grid.setColumnSpacing(-1);
    } catch (const std::invalid_argument&) {
        column_spacing_threw = true;
    }

    bool row_spacing_threw = false;
    try {
        grid.setRowSpacing(-1);
    } catch (const std::invalid_argument&) {
        row_spacing_threw = true;
    }

    CHECK(columns_threw);
    CHECK(column_spacing_threw);
    CHECK(row_spacing_threw);
    CHECK(grid.columnCount() == 2);
    CHECK(grid.columnSpacing() == 2);
    CHECK(grid.rowSpacing() == 3);
}

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

    FixedGridTextContext context{{4, 1}};
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
