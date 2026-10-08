#include "test_framework.hpp"

#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/terminal/list_view_presentation.hpp>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal ListView presentation materializes visible UTF-8 rows") {
    StringListModel model({"first", "日本語", "third"});
    ListSelectionModel selection;
    selection.setModel(&model);
    CHECK(selection.select(1));

    ListView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(0, 3);

    const auto presentation = TerminalListViewPresentation::snapshot(view, {1, 2, 12, 3});
    CHECK(presentation.has_value());
    CHECK(presentation->rows.size() == 3);
    CHECK(presentation->rows[1].row == 1);
    CHECK(presentation->rows[1].selected);

    ScreenBuffer buffer({20, 6});
    CHECK(TerminalListViewPresentation::render(buffer, *presentation));
    CHECK(buffer.at({1, 2}).code_point == U'f');
    CHECK(buffer.at({1, 3}).code_point == U'\u65e5');
    CHECK(buffer.at({1, 3}).role == CellRole::wide_lead);
    CHECK(buffer.at({2, 3}).role == CellRole::wide_continuation);
    CHECK(buffer.at({1, 3}).style.inverse);
}

TEST_CASE("Terminal ListView hit testing uses final row identity and half-open boundaries") {
    StringListModel model({"a", "b"});
    ListSelectionModel selection;
    selection.setModel(&model);
    ListView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(0, 2);
    const auto presentation = TerminalListViewPresentation::snapshot(view, {4, 5, 8, 2});
    CHECK(presentation.has_value());

    CHECK(TerminalListViewPresentation::rowAt(*presentation, {4, 5}) == std::optional<std::size_t>{0});
    CHECK(TerminalListViewPresentation::rowAt(*presentation, {4, 6}) == std::optional<std::size_t>{1});
    CHECK(!TerminalListViewPresentation::rowAt(*presentation, {4, 7}).has_value());
    CHECK(!TerminalListViewPresentation::rowAt(*presentation, {12, 5}).has_value());
    CHECK(TerminalListViewPresentation::selectAt(*presentation, {4, 6}, selection));
    CHECK(selection.selectedRow() == std::optional<std::size_t>{1});
}

TEST_CASE("Terminal ListView rejects a stale pointer snapshot after model mutation") {
    StringListModel model({"a", "b"});
    ListSelectionModel selection;
    selection.setModel(&model);
    ListView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(0, 2);
    const auto presentation = TerminalListViewPresentation::snapshot(view, {0, 0, 8, 2});
    CHECK(presentation.has_value());

    model.insert(0, "new");
    CHECK(!TerminalListViewPresentation::selectAt(*presentation, {0, 0}, selection));
}

TEST_CASE("Terminal ListView fails closed for an oversized visible range") {
    StringListModel model({"a", "b", "c"});
    ListView view;
    view.setModel(&model);
    view.setViewport(0, 3);
    CHECK(!TerminalListViewPresentation::snapshot(view, {0, 0, 8, 2}).has_value());
}
