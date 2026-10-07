#include "test_framework.hpp"

#include <sasd/ui/terminal/combo_box_popup_hit_test.hpp>

#include <optional>
#include <string>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal ComboBox popup hit test maps painted rows with half-open bounds") {
    ComboBoxPopupPresentationSnapshot snapshot;
    snapshot.bounds = {4, 3, 10, 3};
    snapshot.items = {"Zero", "One", "Two"};

    CHECK(
        TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {4, 3}) ==
        std::optional<std::size_t>{0U});
    CHECK(
        TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {13, 4}) ==
        std::optional<std::size_t>{1U});
    CHECK(
        TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {8, 5}) ==
        std::optional<std::size_t>{2U});

    /* Right and bottom edges are outside Rect's half-open geometry. */
    CHECK(!TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {14, 3}).has_value());
    CHECK(!TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {4, 6}).has_value());
    CHECK(!TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {3, 4}).has_value());
}

TEST_CASE("Terminal ComboBox popup hit test follows above-placement coordinates without special cases") {
    ComboBoxPopupPresentationSnapshot snapshot;
    snapshot.bounds = {7, 8, 12, 3};
    snapshot.side = presentation::PopupVerticalSide::above;
    snapshot.items = {"A", "B", "C"};

    CHECK(
        TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {9, 8}) ==
        std::optional<std::size_t>{0U});
    CHECK(
        TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {9, 10}) ==
        std::optional<std::size_t>{2U});
}

TEST_CASE("Terminal ComboBox popup hit test rejects malformed row geometry") {
    ComboBoxPopupPresentationSnapshot snapshot;
    snapshot.bounds = {2, 2, 8, 1};
    snapshot.items = {"One", "Two"};

    CHECK(!TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {3, 2}).has_value());

    snapshot.bounds = {};
    CHECK(!TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {0, 0}).has_value());

    snapshot.items.clear();
    CHECK(!TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, {0, 0}).has_value());
}
