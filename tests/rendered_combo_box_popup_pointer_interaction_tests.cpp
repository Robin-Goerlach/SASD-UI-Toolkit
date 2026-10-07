#include "test_framework.hpp"

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/rendered/combo_box_popup_hit_test.hpp>
#include <sasd/ui/rendered/combo_box_popup_pointer_interaction.hpp>
#include <sasd/ui/rendered/combo_box_popup_presentation.hpp>
#include <sasd/ui/rendered/rendered_measurement_context.hpp>
#include <sasd/ui/text/utf8.hpp>

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

class PointerPopupMetrics final : public RenderedMeasurementContext {
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

struct PointerPopupFixture {
    ComboBox combo{{"Zero", "One", "Two"}};
    FocusManager focus;
    PointerPopupMetrics metrics;
    RenderedComboBoxPopupPresentationSnapshot snapshot;

    PointerPopupFixture() {
        (void)combo.setSelectedIndex(0U);
        (void)focus.requestFocus(combo);
        (void)combo.setDropDownOpen(true);
        const auto built = buildComboBoxPopupPresentation(
            combo,
            {20, 20, 100, 20},
            {0, 0, 300, 200},
            metrics);
        if (built.has_value()) {
            snapshot = *built;
        }
    }

    [[nodiscard]] RenderedComboBoxPopupPresentationSnapshot current() const {
        const auto built = buildComboBoxPopupPresentation(
            combo,
            {20, 20, 100, 20},
            {0, 0, 300, 200},
            metrics);
        return built.value_or(RenderedComboBoxPopupPresentationSnapshot{});
    }
};

[[nodiscard]] PointerEvent pointer(Point point,
                                   PointerAction action,
                                   PointerButton button = PointerButton::none) {
    return PointerEvent{point, action, button, 1, KeyModifier::none};
}

} // namespace

TEST_CASE("Rendered ComboBox popup hit test uses content rows and excludes border edges") {
    PointerPopupFixture fixture;

    CHECK(RenderedComboBoxPopupHitTest::rowIndexAt(fixture.snapshot, {21, 41}) ==
          std::optional<std::size_t>{0U});
    CHECK(RenderedComboBoxPopupHitTest::rowIndexAt(fixture.snapshot, {50, 55}) ==
          std::optional<std::size_t>{1U});
    CHECK(RenderedComboBoxPopupHitTest::rowIndexAt(fixture.snapshot, {118, 82}) ==
          std::optional<std::size_t>{2U});

    CHECK(!RenderedComboBoxPopupHitTest::rowIndexAt(fixture.snapshot, {20, 41}).has_value());
    CHECK(!RenderedComboBoxPopupHitTest::rowIndexAt(fixture.snapshot, {21, 83}).has_value());
    CHECK(!RenderedComboBoxPopupHitTest::rowIndexAt(fixture.snapshot, {119, 41}).has_value());
}

TEST_CASE("Rendered ComboBox popup motion previews a row without committing") {
    PointerPopupFixture fixture;
    RenderedComboBoxPopupPointerInteraction::GestureState gesture;
    std::size_t notifications = 0U;
    fixture.combo.setOnSelectionChanged(
        [&](std::optional<std::size_t>) { ++notifications; });

    const auto result = RenderedComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        pointer({50, 69}, PointerAction::move),
        gesture);

    CHECK(result.has_value());
    CHECK(result->action == RenderedComboBoxPopupPointerAction::preview_changed);
    CHECK(result->row_index == std::optional<std::size_t>{2U});
    CHECK(fixture.combo.previewIndex() == std::optional<std::size_t>{2U});
    CHECK(fixture.combo.selectedIndex() == std::optional<std::size_t>{0U});
    CHECK(notifications == 0U);
}

TEST_CASE("Rendered ComboBox popup matching Primary release commits the armed row") {
    PointerPopupFixture fixture;
    RenderedComboBoxPopupPointerInteraction::GestureState gesture;
    std::size_t notifications = 0U;
    fixture.combo.setOnSelectionChanged(
        [&](std::optional<std::size_t> index) {
            ++notifications;
            CHECK(index == std::optional<std::size_t>{1U});
            CHECK(!fixture.combo.isDropDownOpen());
        });

    const auto press_result = RenderedComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        pointer({50, 55}, PointerAction::press, PointerButton::primary),
        gesture);
    CHECK(press_result.has_value());
    CHECK(gesture.pressedRow() == std::optional<std::size_t>{1U});

    const auto current = fixture.current();
    const auto release_result = RenderedComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        current,
        pointer({50, 55}, PointerAction::release, PointerButton::primary),
        gesture);

    CHECK(release_result.has_value());
    CHECK(release_result->action == RenderedComboBoxPopupPointerAction::committed);
    CHECK(fixture.combo.selectedIndex() == std::optional<std::size_t>{1U});
    CHECK(!gesture.hasPressedRow());
    CHECK(notifications == 1U);
}

TEST_CASE("Rendered ComboBox popup mismatched release retires gesture without commit") {
    PointerPopupFixture fixture;
    RenderedComboBoxPopupPointerInteraction::GestureState gesture;

    CHECK(RenderedComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        pointer({50, 55}, PointerAction::press, PointerButton::primary),
        gesture).has_value());

    const auto result = RenderedComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.current(),
        pointer({50, 69}, PointerAction::release, PointerButton::primary),
        gesture);

    CHECK(result.has_value());
    CHECK(result->action == RenderedComboBoxPopupPointerAction::none);
    CHECK(!gesture.hasPressedRow());
    CHECK(fixture.combo.selectedIndex() == std::optional<std::size_t>{0U});
    CHECK(fixture.combo.isDropDownOpen());
}

TEST_CASE("Rendered ComboBox popup outside Primary press dismisses without click-through") {
    PointerPopupFixture fixture;
    RenderedComboBoxPopupPointerInteraction::GestureState gesture;
    std::size_t selection_notifications = 0U;
    fixture.combo.setOnSelectionChanged(
        [&](std::optional<std::size_t>) { ++selection_notifications; });

    const auto result = RenderedComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        pointer({200, 150}, PointerAction::press, PointerButton::primary),
        gesture);

    CHECK(result.has_value());
    CHECK(result->action == RenderedComboBoxPopupPointerAction::dismissed);
    CHECK(!result->row_index.has_value());
    CHECK(!fixture.combo.isDropDownOpen());
    CHECK(fixture.combo.selectedIndex() == std::optional<std::size_t>{0U});
    CHECK(selection_notifications == 0U);
}

TEST_CASE("Rendered ComboBox popup stale snapshot fails closed and resets armed row") {
    PointerPopupFixture fixture;
    RenderedComboBoxPopupPointerInteraction::GestureState gesture;

    CHECK(RenderedComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        pointer({50, 55}, PointerAction::press, PointerButton::primary),
        gesture).has_value());
    CHECK(gesture.hasPressedRow());

    CHECK(fixture.combo.setItems({"New zero", "New one", "New two"}));
    const auto result = RenderedComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        pointer({50, 55}, PointerAction::release, PointerButton::primary),
        gesture);

    CHECK(!result.has_value());
    CHECK(!gesture.hasPressedRow());
    CHECK(fixture.combo.isDropDownOpen());
}

TEST_CASE("Rendered ComboBox popup commit callback may release the control") {
    auto owner = std::make_unique<Container>();
    auto& combo = owner->emplace<ComboBox>(
        std::vector<std::string>{"Zero", "One"});
    FocusManager focus;
    PointerPopupMetrics metrics;
    RenderedComboBoxPopupPointerInteraction::GestureState gesture;
    bool notified = false;

    (void)combo.setSelectedIndex(0U);
    (void)focus.requestFocus(combo);
    (void)combo.setDropDownOpen(true);
    auto snapshot = buildComboBoxPopupPresentation(
        combo, {20, 20, 100, 20}, {0, 0, 300, 200}, metrics);
    CHECK(snapshot.has_value());

    CHECK(RenderedComboBoxPopupPointerInteraction::handle(
        combo,
        *snapshot,
        pointer({50, 55}, PointerAction::press, PointerButton::primary),
        gesture).has_value());

    snapshot = buildComboBoxPopupPresentation(
        combo, {20, 20, 100, 20}, {0, 0, 300, 200}, metrics);
    CHECK(snapshot.has_value());

    combo.setOnSelectionChanged([&](std::optional<std::size_t>) {
        notified = true;
        owner.reset();
    });

    const auto result = RenderedComboBoxPopupPointerInteraction::handle(
        combo,
        *snapshot,
        pointer({50, 55}, PointerAction::release, PointerButton::primary),
        gesture);

    CHECK(result.has_value());
    CHECK(result->action == RenderedComboBoxPopupPointerAction::committed);
    CHECK(notified);
    CHECK(!owner);
    CHECK(!gesture.hasPressedRow());
}

TEST_CASE("Rendered ComboBox popup malformed fixed-row geometry cannot become input") {
    PointerPopupFixture fixture;
    RenderedComboBoxPopupPointerInteraction::GestureState gesture;
    fixture.snapshot.content_bounds.height = 13;

    const auto result = RenderedComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        pointer({50, 41}, PointerAction::move),
        gesture);

    CHECK(!result.has_value());

    PointerPopupFixture shifted_fixture;
    shifted_fixture.snapshot.content_bounds.x += 1;
    const auto shifted_result = RenderedComboBoxPopupPointerInteraction::handle(
        shifted_fixture.combo,
        shifted_fixture.snapshot,
        pointer({50, 41}, PointerAction::move),
        gesture);
    CHECK(!shifted_result.has_value());

    PointerPopupFixture oversized_text_fixture;
    oversized_text_fixture.snapshot.items.front().text_size.width = 500;
    const auto oversized_text_result =
        RenderedComboBoxPopupPointerInteraction::handle(
            oversized_text_fixture.combo,
            oversized_text_fixture.snapshot,
            pointer({50, 41}, PointerAction::move),
            gesture);
    CHECK(!oversized_text_result.has_value());
}
