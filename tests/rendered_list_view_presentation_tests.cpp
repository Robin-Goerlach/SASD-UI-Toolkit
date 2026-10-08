#include "test_framework.hpp"

#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/list_view.hpp>
#include <sasd/ui/rendered/list_view_presentation.hpp>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

TEST_CASE("Rendered ListView presentation owns visible text and selection") {
    StringListModel model({"one", "two", "three"});
    ListSelectionModel selection;
    selection.setModel(&model);
    CHECK(selection.select(1));
    ListView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(0, 3);

    const auto presentation = RenderedListViewPresentation::snapshot(view, {2, 4, 30, 3});
    CHECK(presentation.has_value());
    DisplayList display_list;
    CHECK(RenderedListViewPresentation::render(display_list, *presentation));
    CHECK(display_list.size() == 4); // background plus one owned text command per visible row
    CHECK(std::get<DrawTextCommand>(display_list.commands()[2]).text == "two");
    CHECK(std::get<DrawTextCommand>(display_list.commands()[2]).style.inverse);
}

TEST_CASE("Rendered ListView hit test uses exact half-open row geometry") {
    StringListModel model({"a", "b"});
    ListSelectionModel selection;
    selection.setModel(&model);
    ListView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(0, 2);
    const auto presentation = RenderedListViewPresentation::snapshot(view, {0, 5, 10, 2});
    CHECK(presentation.has_value());
    CHECK(RenderedListViewPresentation::rowAt(*presentation, {1, 5}) == std::optional<std::size_t>{0});
    CHECK(RenderedListViewPresentation::rowAt(*presentation, {1, 6}) == std::optional<std::size_t>{1});
    CHECK(!RenderedListViewPresentation::rowAt(*presentation, {1, 7}).has_value());
    CHECK(RenderedListViewPresentation::selectAt(*presentation, {1, 6}, selection));
    CHECK(selection.selectedRow() == std::optional<std::size_t>{1});
}

TEST_CASE("Rendered ListView rejects stale snapshots") {
    StringListModel model({"a", "b"});
    ListSelectionModel selection;
    selection.setModel(&model);
    ListView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(0, 2);
    const auto presentation = RenderedListViewPresentation::snapshot(view, {0, 0, 10, 2});
    CHECK(presentation.has_value());
    model.erase(0);
    CHECK(!RenderedListViewPresentation::selectAt(*presentation, {0, 0}, selection));
}
