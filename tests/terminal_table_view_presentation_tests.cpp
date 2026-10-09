#include "test_framework.hpp"

#include <sasd/ui/table_selection_model.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/terminal/table_view_presentation.hpp>

#include <optional>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal TableView owns UTF-8 cells and preserves scrolled column identity") {
    StringTableModel model({"A", "B", "C"}, {{"eins", "日本", "drei"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 1, 1, 2);

    const auto presentation = TerminalTableViewPresentation::snapshot(view, {2, 3, 12, 3});
    CHECK(presentation.has_value());
    CHECK(presentation->column_indices == std::vector<std::size_t>{1, 2});
    ScreenBuffer buffer({20, 8});
    CHECK(TerminalTableViewPresentation::render(buffer, *presentation));
    CHECK(buffer.at({2, 4}).code_point == U'\u65e5');
    CHECK(buffer.at({2, 4}).role == CellRole::wide_lead);
    CHECK(buffer.at({3, 4}).role == CellRole::wide_continuation);
    CHECK(TerminalTableViewPresentation::hitAt(*presentation, {2, 4}) ==
          std::optional<TerminalTableViewHit>{TerminalTableViewHit{0, 1}});
}

TEST_CASE("Terminal TableView selection is rendered and exact hit boundaries are half-open") {
    StringTableModel model({"A", "B"}, {{"x", "y"}});
    TableSelectionModel selection;
    selection.setModel(&model);
    CHECK(selection.select(0, 1));
    TableView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(0, 1, 0, 2);
    const auto presentation = TerminalTableViewPresentation::snapshot(view, {0, 0, 5, 3});
    CHECK(presentation.has_value());
    ScreenBuffer buffer({8, 5});
    CHECK(TerminalTableViewPresentation::render(buffer, *presentation));
    CHECK(buffer.at({2, 1}).style.inverse);
    CHECK(!TerminalTableViewPresentation::hitAt(*presentation, {0, 0}).has_value());
    CHECK(TerminalTableViewPresentation::hitAt(*presentation, {2, 1}) ==
          std::optional<TerminalTableViewHit>{TerminalTableViewHit{0, 1}});
    CHECK(!TerminalTableViewPresentation::hitAt(*presentation, {4, 1}).has_value());
}

TEST_CASE("Terminal TableView rejects malformed geometry transactionally") {
    StringTableModel model({"A"}, {{"x"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 1, 0, 1);
    const auto original = TerminalTableViewPresentation::snapshot(view, {0, 0, 4, 3});
    CHECK(original.has_value());

    for (const auto malformed : {0, 1}) {
        auto snapshot = *original;
        if (malformed == 0) {
            snapshot.header_bounds.x = -1;
        } else {
            snapshot.row_bounds[0].y = snapshot.header_bounds.y;
        }
        ScreenBuffer buffer({8, 5});
        buffer.set({7, 4}, Cell{U'Q'});
        CHECK(!TerminalTableViewPresentation::render(buffer, snapshot));
        CHECK(buffer.at({7, 4}).code_point == U'Q');
    }
}

TEST_CASE("Terminal TableView rejects stale selection snapshots") {
    StringTableModel model({"A"}, {{"x"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 1, 0, 1);
    const auto presentation = TerminalTableViewPresentation::snapshot(view, {0, 0, 4, 2});
    CHECK(presentation.has_value());
    TableSelectionModel selection;
    selection.setModel(&model);
    model.setCell(0, 0, "changed");
    CHECK(!TerminalTableViewPresentation::selectAt(*presentation, {0, 1}, selection));
    CHECK(!selection.selectedCell().has_value());
}
