#include "test_framework.hpp"

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/rendered/combo_box_popup_presentation.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/text/utf8.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

class PopupMetrics final : public RenderedMeasurementContext {
public:
    [[nodiscard]] Size measureText(std::string_view text) const override {
        return {
            static_cast<Coordinate>(utf8::scalarCount(text) * 8U),
            lineHeight(),
        };
    }

    [[nodiscard]] Coordinate lineHeight() const noexcept override {
        return 12;
    }

    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view text,
        std::size_t scalar_index) const override {
        if (scalar_index > utf8::scalarCount(text)) {
            return std::nullopt;
        }
        return static_cast<Coordinate>(scalar_index * 8U);
    }

    [[nodiscard]] std::uint64_t revision() const noexcept override {
        return 1;
    }
};

struct OpenRenderedComboFixture {
    ComboBox combo{{"One", "Longest", "Three"}};
    FocusManager focus;
    PopupMetrics metrics;

    OpenRenderedComboFixture() {
        (void)combo.setSelectedIndex(0U);
        (void)focus.requestFocus(combo);
        (void)combo.setDropDownOpen(true);
    }
};

} // namespace

TEST_CASE("Rendered ComboBox popup builder measures fixed rows and expands to anchor width") {
    OpenRenderedComboFixture fixture;
    CHECK(fixture.combo.setPreviewIndex(1U));

    const auto snapshot = buildComboBoxPopupPresentation(
        fixture.combo,
        {20, 20, 100, 20},
        {0, 0, 300, 200},
        fixture.metrics);

    CHECK(snapshot.has_value());
    CHECK(snapshot->side == presentation::PopupVerticalSide::below);
    CHECK(snapshot->bounds == Rect{20, 40, 100, 44});
    CHECK(snapshot->content_bounds == Rect{21, 41, 98, 42});
    CHECK(snapshot->row_height == 14);
    CHECK(snapshot->padding == 1);
    CHECK(snapshot->border_thickness == 1);
    CHECK(snapshot->items.size() == 3U);
    CHECK(snapshot->items[1].text == "Longest");
    CHECK(snapshot->items[1].text_size == Size{56, 12});
    CHECK(snapshot->preview_index == std::optional<std::size_t>{1U});
}

TEST_CASE("Rendered ComboBox popup builder flips above when complete popup does not fit below") {
    OpenRenderedComboFixture fixture;

    const auto snapshot = buildComboBoxPopupPresentation(
        fixture.combo,
        {20, 150, 100, 20},
        {0, 0, 300, 180},
        fixture.metrics);

    CHECK(snapshot.has_value());
    CHECK(snapshot->side == presentation::PopupVerticalSide::above);
    CHECK(snapshot->bounds == Rect{20, 106, 100, 44});
    CHECK(snapshot->content_bounds == Rect{21, 107, 98, 42});
}

TEST_CASE("Rendered ComboBox popup renderer emits opaque chrome rows and preview geometry") {
    OpenRenderedComboFixture fixture;
    CHECK(fixture.combo.setPreviewIndex(1U));

    TextStyle style;
    style.foreground = Color::bright_cyan;
    fixture.combo.setTextStyle(style);

    const auto snapshot = buildComboBoxPopupPresentation(
        fixture.combo,
        {20, 20, 100, 20},
        {0, 0, 300, 200},
        fixture.metrics);
    CHECK(snapshot.has_value());

    DisplayList list;
    CHECK(renderComboBoxPopupPresentation(
        list,
        *snapshot,
        Color::black));

    const auto commands = list.commands();

    /*
     * Outer background + outer border + row0 text + preview-row outline + row1 text + row2 text.
     * The preview outline spans the complete fixed row while inverse is confined to text styling.
     */
    CHECK(commands.size() == 6U);
    CHECK(std::get<FillRectCommand>(commands[0]) ==
          FillRectCommand{Rect{20, 40, 100, 44}, Color::black});
    CHECK(std::get<StrokeRectCommand>(commands[1]) ==
          StrokeRectCommand{Rect{20, 40, 100, 44}, Color::bright_cyan, 1});

    const auto& row0 = std::get<DrawTextCommand>(commands[2]);
    CHECK(row0.origin == Point{22, 42});
    CHECK(row0.text == "One");
    CHECK(row0.clip_bounds == std::optional<Rect>{Rect{22, 42, 96, 12}});

    CHECK(std::get<StrokeRectCommand>(commands[3]) ==
          StrokeRectCommand{Rect{21, 55, 98, 14}, Color::bright_cyan, 1});

    const auto& preview = std::get<DrawTextCommand>(commands[4]);
    CHECK(preview.origin == Point{22, 56});
    CHECK(preview.text == "Longest");
    CHECK(preview.style.inverse);
    CHECK(preview.clip_bounds == std::optional<Rect>{Rect{22, 56, 96, 12}});

    const auto& row2 = std::get<DrawTextCommand>(commands[5]);
    CHECK(row2.origin == Point{22, 70});
    CHECK(row2.text == "Three");
}

TEST_CASE("Rendered ComboBox popup preview toggles an already inverse base style") {
    OpenRenderedComboFixture fixture;

    /*
     * Opening the fixture seeds preview from committed selection zero by ADR 0121. Re-applying the same
     * preview index is correctly an idempotent no-op returning false, so assert the seeded transaction
     * state directly instead of accidentally treating no change as setup failure.
     */
    CHECK(fixture.combo.previewIndex() == std::optional<std::size_t>{0U});

    TextStyle style;
    style.inverse = true;
    fixture.combo.setTextStyle(style);

    const auto snapshot = buildComboBoxPopupPresentation(
        fixture.combo,
        {10, 10, 100, 20},
        {0, 0, 300, 200},
        fixture.metrics);
    CHECK(snapshot.has_value());

    DisplayList list;
    CHECK(renderComboBoxPopupPresentation(list, *snapshot));

    const auto commands = list.commands();
    const auto& preview = std::get<DrawTextCommand>(commands[3]);
    CHECK(preview.text == "One");
    CHECK(!preview.style.inverse);

    const auto& normal = std::get<DrawTextCommand>(commands[4]);
    CHECK(normal.text == "Longest");
    CHECK(normal.style.inverse);
}

TEST_CASE("Rendered ComboBox popup composition appends to a copied base without mutating source") {
    OpenRenderedComboFixture fixture;
    CHECK(fixture.combo.setPreviewIndex(2U));

    DisplayList base;
    base.fillRect({0, 0, 300, 200}, Color::black);
    base.drawText({5, 5}, "base");

    const auto composed = composeComboBoxPopupDisplayList(
        base,
        fixture.combo,
        {20, 20, 100, 20},
        {0, 0, 300, 200},
        fixture.metrics,
        Color::black);

    CHECK(composed.has_value());
    CHECK(base.size() == 2U);
    CHECK(composed->size() > base.size());
    CHECK(std::get<DrawTextCommand>(base.commands()[1]).text == "base");
    CHECK(std::get<DrawTextCommand>(composed->commands()[1]).text == "base");
}

TEST_CASE("Rendered ComboBox popup renderer rejects malformed snapshot before appending commands") {
    RenderedComboBoxPopupPresentationSnapshot snapshot;
    snapshot.bounds = {10, 10, 80, 30};
    snapshot.content_bounds = {11, 11, 78, 28};
    snapshot.items = {
        {"One", {24, 12}},
        {"Two", {24, 12}},
    };
    snapshot.preview_index = 7U;
    snapshot.row_height = 14;
    snapshot.padding = 1;
    snapshot.border_thickness = 1;

    DisplayList list;
    list.drawText({1, 1}, "previous");

    CHECK(!renderComboBoxPopupPresentation(list, snapshot));
    CHECK(list.size() == 1U);
    CHECK(std::get<DrawTextCommand>(list.commands()[0]).text == "previous");

    snapshot.preview_index = 1U;
    snapshot.content_bounds.height = 14; // two declared rows cannot be silently clipped.
    CHECK(!renderComboBoxPopupPresentation(list, snapshot));
    CHECK(list.size() == 1U);
}

TEST_CASE("Rendered empty open ComboBox produces a valid no-overlay composition") {
    ComboBox combo;
    FocusManager focus;
    PopupMetrics metrics;

    CHECK(focus.requestFocus(combo));
    CHECK(combo.setDropDownOpen(true));

    const auto snapshot = buildComboBoxPopupPresentation(
        combo,
        {10, 10, 80, 20},
        {0, 0, 300, 200},
        metrics);
    CHECK(snapshot.has_value());
    CHECK(snapshot->items.empty());
    CHECK(snapshot->bounds.isEmpty());
    CHECK(snapshot->content_bounds.isEmpty());

    DisplayList base;
    base.drawText({1, 1}, "base");

    const auto composed = composeComboBoxPopupDisplayList(
        base,
        combo,
        {10, 10, 80, 20},
        {0, 0, 300, 200},
        metrics);
    CHECK(composed.has_value());
    CHECK(composed->commands().size() == base.commands().size());
    CHECK(std::get<DrawTextCommand>(composed->commands()[0]).text == "base");
}

TEST_CASE("Rendered ComboBox popup fails closed when complete row stack fits on neither side") {
    ComboBox combo{{"0", "1", "2", "3", "4"}};
    FocusManager focus;
    PopupMetrics metrics;

    CHECK(focus.requestFocus(combo));
    CHECK(combo.setDropDownOpen(true));

    /*
     * Five fourteen-unit rows plus borders need 72 logical units. The anchor splits this viewport so
     * neither side provides enough attached space; generic placement must not clip/overlap implicitly.
     */
    CHECK(!buildComboBoxPopupPresentation(
        combo,
        {20, 50, 100, 20},
        {0, 0, 300, 120},
        metrics).has_value());
}
