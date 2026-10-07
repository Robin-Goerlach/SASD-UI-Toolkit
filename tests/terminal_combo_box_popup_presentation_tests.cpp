#include "test_framework.hpp"

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/terminal/combo_box_popup_presentation.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

namespace {

struct OpenComboFixture {
    ComboBox combo{{"One", "Two", std::string{"A\xE7\x95\x8C"}}};
    FocusManager focus;

    OpenComboFixture() {
        (void)combo.setSelectedIndex(0U);
        (void)focus.requestFocus(combo);
        (void)combo.setDropDownOpen(true);
    }
};

} // namespace

TEST_CASE("Terminal ComboBox popup measurement uses one row per item and terminal cell width") {
    ComboBoxPopupPresentationSnapshot snapshot;
    snapshot.items = {"One", std::string{"A\xE7\x95\x8C"}};

    const auto measured = measureComboBoxPopupPresentation(snapshot);
    CHECK(measured.has_value());

    /*
     * "One" needs three cells and two padding cells => width five. "A界" is also three terminal
     * columns (1 + 2), so both rows share the same five-cell natural popup width.
     */
    CHECK(measured->size == Size{5, 2});
}

TEST_CASE("Terminal ComboBox popup builder aligns below and expands to collapsed anchor width") {
    OpenComboFixture fixture;
    CHECK(fixture.combo.setPreviewIndex(1U));

    const auto snapshot = buildComboBoxPopupPresentation(
        fixture.combo,
        {4, 2, 12, 1},
        {0, 0, 30, 10});

    CHECK(snapshot.has_value());
    CHECK(snapshot->side == presentation::PopupVerticalSide::below);
    CHECK(snapshot->bounds == Rect{4, 3, 12, 3});
    CHECK(snapshot->items.size() == 3U);
    CHECK(snapshot->items[0] == "One");
    CHECK(snapshot->preview_index == std::optional<std::size_t>{1U});
}

TEST_CASE("Terminal ComboBox popup builder flips above near the viewport bottom") {
    OpenComboFixture fixture;

    const auto snapshot = buildComboBoxPopupPresentation(
        fixture.combo,
        {5, 7, 10, 1},
        {0, 0, 30, 9});

    CHECK(snapshot.has_value());
    CHECK(snapshot->side == presentation::PopupVerticalSide::above);
    CHECK(snapshot->bounds == Rect{5, 4, 10, 3});
}

TEST_CASE("Terminal ComboBox popup renderer paints preview style and wide glyph occupancy") {
    OpenComboFixture fixture;
    CHECK(fixture.combo.setPreviewIndex(2U));

    TextStyle style;
    style.foreground = Color::bright_cyan;
    fixture.combo.setTextStyle(style);

    const auto snapshot = buildComboBoxPopupPresentation(
        fixture.combo,
        {2, 1, 10, 1},
        {0, 0, 20, 8});
    CHECK(snapshot.has_value());

    ScreenBuffer buffer{{20, 8}};
    buffer.clear(Cell{U'.'});

    CHECK(renderComboBoxPopupPresentation(buffer, *snapshot));

    CHECK(buffer.at({2, 2}).code_point == U' ');
    CHECK(buffer.at({3, 2}).code_point == U'O');
    CHECK(buffer.at({4, 2}).code_point == U'n');
    CHECK(buffer.at({5, 2}).code_point == U'e');

    /*
     * Preview index two is the "A界" row. The highlight covers the complete popup row, including
     * padding and unused anchor-expanded cells, while the CJK scalar occupies lead+continuation cells.
     */
    TextStyle preview_style = style;
    preview_style.inverse = true;
    CHECK(buffer.at({2, 4}).style == preview_style);
    CHECK(buffer.at({3, 4}).code_point == U'A');
    CHECK(buffer.at({4, 4}).code_point == U'\x754C');
    CHECK(buffer.at({4, 4}).role == CellRole::wide_lead);
    CHECK(buffer.at({5, 4}).role == CellRole::wide_continuation);
    CHECK(buffer.at({11, 4}).style == preview_style);
}

TEST_CASE("Terminal ComboBox popup preview toggles inverse when user base style is already inverse") {
    OpenComboFixture fixture;

    /*
     * OpenComboFixture commits item zero before opening. ADR 0121 requires opening to seed preview from
     * the committed selection, so preview zero is already the coherent initial transaction state here.
     * setPreviewIndex(0) would correctly be an idempotent no-op (return false), not a failed setup.
     */
    CHECK(fixture.combo.previewIndex() == std::optional<std::size_t>{0U});

    TextStyle style;
    style.inverse = true;
    fixture.combo.setTextStyle(style);

    const auto snapshot = buildComboBoxPopupPresentation(
        fixture.combo,
        {1, 1, 8, 1},
        {0, 0, 20, 8});
    CHECK(snapshot.has_value());

    ScreenBuffer buffer{{20, 8}};
    CHECK(renderComboBoxPopupPresentation(buffer, *snapshot));

    CHECK(!buffer.at({1, 2}).style.inverse);
    CHECK(buffer.at({1, 3}).style.inverse);
}

TEST_CASE("Terminal ComboBox popup composition overlays immutable base and suppresses caret") {
    OpenComboFixture fixture;
    CHECK(fixture.combo.setPreviewIndex(1U));

    TerminalPresentationFrame base{ScreenBuffer{{20, 8}}, Point{17, 7}};
    base.buffer.clear(Cell{U'.'});

    const auto composed = composeComboBoxPopupFrame(
        base,
        fixture.combo,
        {3, 1, 10, 1});
    CHECK(composed.has_value());

    CHECK(!composed->caret.has_value());
    CHECK(composed->buffer.at({4, 2}).code_point == U'O');
    CHECK(composed->buffer.at({4, 3}).code_point == U'T');
    CHECK(composed->buffer.at({0, 0}).code_point == U'.');

    /* Composition is value-oriented: neither source cells nor source caret metadata are mutated. */
    CHECK(base.buffer.at({4, 2}).code_point == U'.');
    CHECK(base.caret == std::optional<Point>{Point{17, 7}});
}

TEST_CASE("Terminal ComboBox popup closed composition is a value-preserving pass-through") {
    ComboBox combo{{"One"}};
    TerminalPresentationFrame base{ScreenBuffer{{12, 4}}, Point{3, 2}};
    base.buffer.clear(Cell{U'#'});

    const auto composed = composeComboBoxPopupFrame(
        base,
        combo,
        {1, 1, 8, 1});
    CHECK(composed.has_value());
    CHECK(composed->caret == base.caret);
    CHECK(composed->buffer.at({5, 2}).code_point == U'#');
}

TEST_CASE("Terminal empty open ComboBox composes no invented row but still suppresses caret") {
    ComboBox combo;
    FocusManager focus;
    CHECK(focus.requestFocus(combo));
    CHECK(combo.setDropDownOpen(true));

    TerminalPresentationFrame base{ScreenBuffer{{12, 4}}, Point{3, 2}};
    base.buffer.clear(Cell{U'.'});

    const auto composed = composeComboBoxPopupFrame(
        base,
        combo,
        {1, 1, 8, 1});
    CHECK(composed.has_value());
    CHECK(!composed->caret.has_value());
    CHECK(composed->buffer.at({2, 2}).code_point == U'.');
    CHECK(base.caret == std::optional<Point>{Point{3, 2}});
}

TEST_CASE("Terminal ComboBox popup rejects unsupported item text without modifying base frame") {
    ComboBox combo{{"One", std::string{"A\xCC\x81"}}};
    FocusManager focus;
    CHECK(focus.requestFocus(combo));
    CHECK(combo.setDropDownOpen(true));

    TerminalPresentationFrame base{ScreenBuffer{{20, 8}}, Point{10, 6}};
    base.buffer.clear(Cell{U'#'});

    const auto composed = composeComboBoxPopupFrame(
        base,
        combo,
        {2, 1, 10, 1});
    CHECK(!composed.has_value());

    CHECK(base.buffer.at({2, 2}).code_point == U'#');
    CHECK(base.buffer.at({10, 6}).code_point == U'#');
    CHECK(base.caret == std::optional<Point>{Point{10, 6}});
}

TEST_CASE("Terminal ComboBox popup fails closed rather than clipping a list that fits on neither side") {
    ComboBox combo{{"Zero", "One", "Two", "Three", "Four"}};
    FocusManager focus;
    CHECK(focus.requestFocus(combo));
    CHECK(combo.setDropDownOpen(true));

    /*
     * Five rows fit the six-row viewport by themselves, but an anchor spanning rows 2..3 leaves only
     * two rows above and two rows below. The generic anchored policy must not overlap or clip the anchor.
     */
    CHECK(!buildComboBoxPopupPresentation(
        combo,
        {2, 2, 10, 2},
        {0, 0, 20, 6}).has_value());
}

TEST_CASE("Terminal ComboBox popup renderer rejects stale preview and geometry transactionally") {
    ComboBoxPopupPresentationSnapshot stale;
    stale.bounds = {2, 2, 8, 2};
    stale.items = {"One", "Two"};
    stale.preview_index = 2U;

    ScreenBuffer buffer{{20, 8}};
    buffer.clear(Cell{U'!'});

    CHECK(!renderComboBoxPopupPresentation(buffer, stale));
    CHECK(buffer.at({2, 2}).code_point == U'!');
    CHECK(buffer.at({5, 3}).code_point == U'!');

    stale.preview_index = 1U;
    stale.bounds.height = 1; // two semantic rows cannot be silently clipped to one row
    CHECK(!renderComboBoxPopupPresentation(buffer, stale));
    CHECK(buffer.at({2, 2}).code_point == U'!');
}

TEST_CASE("Terminal ComboBox popup composition honors an explicit host-reserved viewport") {
    OpenComboFixture fixture;
    CHECK(fixture.combo.setPreviewIndex(1U));

    TerminalPresentationFrame base{ScreenBuffer{{20, 8}}, Point{18, 7}};
    base.buffer.clear(Cell{U'.'});

    /*
     * Row zero is reserved for persistent host chrome. Prefer above deliberately: with the complete
     * buffer the three-row popup could use y=0..2 above an anchor at y=3, but the content viewport
     * begins at y=1, so above no longer fits and the shared placement policy must fall back below.
     */
    const auto composed = composeComboBoxPopupFrame(
        base,
        fixture.combo,
        {3, 3, 10, 1},
        {0, 1, 20, 7},
        AmbiguousWidthMode::narrow,
        presentation::PopupVerticalSide::above);

    CHECK(composed.has_value());
    CHECK(!composed->caret.has_value());

    /* Reserved row zero remains byte-for-byte base presentation. */
    CHECK(composed->buffer.at({3, 0}).code_point == U'.');

    /* The popup begins immediately below the anchor at y=4 after above placement is rejected. */
    CHECK(composed->buffer.at({4, 4}).code_point == U'O');
    CHECK(composed->buffer.at({4, 5}).code_point == U'T');
}

TEST_CASE("Terminal ComboBox popup composition rejects a viewport outside the base frame transactionally") {
    OpenComboFixture fixture;
    TerminalPresentationFrame base{ScreenBuffer{{12, 5}}, Point{8, 4}};
    base.buffer.clear(Cell{U'#'});

    const auto composed = composeComboBoxPopupFrame(
        base,
        fixture.combo,
        {1, 1, 8, 1},
        {0, 1, 20, 4});

    CHECK(!composed.has_value());
    CHECK(base.buffer.at({2, 2}).code_point == U'#');
    CHECK(base.caret == std::optional<Point>{Point{8, 4}});
}
