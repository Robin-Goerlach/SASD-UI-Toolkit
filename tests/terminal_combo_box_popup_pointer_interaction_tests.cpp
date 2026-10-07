#include "test_framework.hpp"

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/terminal/combo_box_popup_pointer_interaction.hpp>
#include <sasd/ui/terminal/combo_box_popup_presentation.hpp>

#include <optional>
#include <string>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

namespace {

struct PointerComboFixture {
    ComboBox combo{{"Zero", "One", "Two"}};
    FocusManager focus;
    ComboBoxPopupPresentationSnapshot snapshot;

    PointerComboFixture() {
        (void)combo.setSelectedIndex(0U);
        (void)focus.requestFocus(combo);
        (void)combo.setDropDownOpen(true);

        const auto built = buildComboBoxPopupPresentation(
            combo,
            {3, 2, 10, 1},
            {0, 0, 30, 12});
        if (built.has_value()) {
            snapshot = *built;
        }
    }
};

[[nodiscard]] PointerEvent motion(Point point) {
    return PointerEvent{
        point,
        PointerAction::move,
        PointerButton::none,
        0,
        KeyModifier::none,
    };
}

} // namespace

TEST_CASE("Terminal ComboBox popup pointer motion previews a painted row without committing selection") {
    PointerComboFixture fixture;
    std::size_t selection_notifications = 0U;
    fixture.combo.setOnSelectionChanged(
        [&](std::optional<std::size_t>) { ++selection_notifications; });

    const auto result = TerminalComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        motion({5, 5})); // popup rows are y=3,4,5 => row two

    CHECK(result.has_value());
    CHECK(result->action == TerminalComboBoxPopupPointerAction::preview_changed);
    CHECK(result->row_index == std::optional<std::size_t>{2U});
    CHECK(fixture.combo.previewIndex() == std::optional<std::size_t>{2U});
    CHECK(fixture.combo.selectedIndex() == std::optional<std::size_t>{0U});
    CHECK(selection_notifications == 0U);
}

TEST_CASE("Terminal ComboBox popup repeated motion over current preview is consumed without extra mutation") {
    PointerComboFixture fixture;

    const auto first = TerminalComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        motion({5, 3})); // opening seeded preview row zero

    CHECK(first.has_value());
    CHECK(first->action == TerminalComboBoxPopupPointerAction::none);
    CHECK(first->row_index == std::optional<std::size_t>{0U});
    CHECK(fixture.combo.previewIndex() == std::optional<std::size_t>{0U});
}

TEST_CASE("Terminal ComboBox popup outside motion remains modal and preserves last preview") {
    PointerComboFixture fixture;
    CHECK(fixture.combo.setPreviewIndex(1U));

    /* Rebuild because input must be interpreted against the presentation currently on screen. */
    const auto current = buildComboBoxPopupPresentation(
        fixture.combo,
        {3, 2, 10, 1},
        {0, 0, 30, 12});
    CHECK(current.has_value());

    const auto result = TerminalComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        *current,
        motion({25, 10}));

    CHECK(result.has_value());
    CHECK(result->action == TerminalComboBoxPopupPointerAction::none);
    CHECK(!result->row_index.has_value());
    CHECK(fixture.combo.previewIndex() == std::optional<std::size_t>{1U});
    CHECK(fixture.combo.isDropDownOpen());
}

TEST_CASE("Terminal ComboBox popup press and release are consumed without premature click semantics") {
    PointerComboFixture fixture;

    const PointerEvent press{
        {5, 4},
        PointerAction::press,
        PointerButton::primary,
        1,
        KeyModifier::none,
    };
    const PointerEvent release{
        {5, 4},
        PointerAction::release,
        PointerButton::primary,
        1,
        KeyModifier::none,
    };

    const auto press_result = TerminalComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        press);
    const auto release_result = TerminalComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        release);

    CHECK(press_result.has_value());
    CHECK(release_result.has_value());
    CHECK(press_result->row_index == std::optional<std::size_t>{1U});
    CHECK(release_result->row_index == std::optional<std::size_t>{1U});

    /* Click commit/dismissal is deliberately not part of this hover-preview slice. */
    CHECK(fixture.combo.previewIndex() == std::optional<std::size_t>{0U});
    CHECK(fixture.combo.selectedIndex() == std::optional<std::size_t>{0U});
    CHECK(fixture.combo.isDropDownOpen());
}

TEST_CASE("Terminal ComboBox popup pointer interaction rejects stale semantic snapshot") {
    PointerComboFixture fixture;

    /* Same row count would be especially dangerous if numeric identity were trusted without content proof. */
    CHECK(fixture.combo.setItems({"New zero", "New one", "New two"}));

    const auto result = TerminalComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        motion({5, 4}));

    CHECK(!result.has_value());
    CHECK(!fixture.combo.previewIndex().has_value());
}

TEST_CASE("Terminal ComboBox popup pointer interaction rejects stale preview snapshot") {
    PointerComboFixture fixture;
    CHECK(fixture.combo.setPreviewIndex(2U));

    /*
     * fixture.snapshot still describes the presentation with preview zero. Input must not be applied to a
     * frame that is no longer the semantic presentation transaction the user is seeing/should be seeing.
     */
    const auto result = TerminalComboBoxPopupPointerInteraction::handle(
        fixture.combo,
        fixture.snapshot,
        motion({5, 4}));

    CHECK(!result.has_value());
    CHECK(fixture.combo.previewIndex() == std::optional<std::size_t>{2U});
}

TEST_CASE("Terminal ComboBox popup pointer interaction leaves inactive ComboBox unclaimed") {
    ComboBox combo{{"One"}};
    ComboBoxPopupPresentationSnapshot snapshot;
    snapshot.bounds = {1, 1, 6, 1};
    snapshot.items = {"One"};

    const auto result = TerminalComboBoxPopupPointerInteraction::handle(
        combo,
        snapshot,
        motion({2, 1}));

    CHECK(!result.has_value());
}
