#include "test_framework.hpp"

#include <algorithm>

#include <sasd/ui/rendered/tree_view_presentation.hpp>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

TEST_CASE("Rendered TreeView owns final rows and disclosure geometry") {
    StringTreeModel model({"Parent"});
    const auto parent = TreeNodePath{{0}};
    model.append(parent, "Child");
    TreeSelectionModel selection;
    selection.setModel(&model);
    CHECK(selection.select(parent));
    TreeView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    CHECK(view.expand(parent));
    view.setViewport(std::nullopt, 2);

    const auto presentation = RenderedTreeViewPresentation::snapshot(view, {10, 20, 180, 2});
    CHECK(presentation.has_value());
    CHECK(presentation->rows.size() == 2);
    CHECK(presentation->rows[0].indicator_bounds == Rect{26, 20, 12, 1});
    CHECK(presentation->rows[1].node == TreeNodePath{{0, 0}});

    DisplayList display_list;
    CHECK(RenderedTreeViewPresentation::render(display_list, *presentation));
    CHECK(display_list.size() == 4); // background, indicator, parent text and child text
}

TEST_CASE("Rendered TreeView hit testing distinguishes indicator and row") {
    StringTreeModel model({"Parent"});
    const auto parent = TreeNodePath{{0}};
    model.append(parent, "Child");
    TreeView view;
    view.setModel(&model);
    view.setViewport(std::nullopt, 1);
    const auto presentation = RenderedTreeViewPresentation::snapshot(view, {10, 20, 180, 1});
    CHECK(presentation.has_value());

    const auto indicator = RenderedTreeViewPresentation::hitAt(*presentation, {26, 20});
    CHECK(indicator.has_value());
    CHECK(indicator->kind == RenderedTreeViewHitKind::indicator);
    const auto row = RenderedTreeViewPresentation::hitAt(*presentation, {40, 20});
    CHECK(row.has_value());
    CHECK(row->kind == RenderedTreeViewHitKind::row);
    CHECK(!RenderedTreeViewPresentation::hitAt(*presentation, {10, 21}).has_value());
}

TEST_CASE("Rendered TreeView rejects stale pointer transactions") {
    StringTreeModel model({"Parent"});
    const auto parent = TreeNodePath{{0}};
    model.append(parent, "Child");
    TreeSelectionModel selection;
    selection.setModel(&model);
    TreeView view;
    view.setModel(&model);
    view.setSelectionModel(&selection);
    view.setViewport(std::nullopt, 1);
    const auto presentation = RenderedTreeViewPresentation::snapshot(view, {0, 0, 100, 1});
    CHECK(presentation.has_value());

    model.setText(parent, "Renamed");
    CHECK(!RenderedTreeViewPresentation::selectAt(*presentation, {20, 0}, selection));
    CHECK(!RenderedTreeViewPresentation::toggleAt(*presentation, {16, 0}, view));
}

TEST_CASE("Rendered TreeView rendering preserves the list on malformed geometry") {
    RenderedTreeViewPresentationSnapshot snapshot;
    snapshot.bounds = {0, 0, 100, 1};
    snapshot.rows.push_back({TreeNodePath{{0}}, 99, "bad", {0, 0, 100, 1}, {}, false, false, false});
    DisplayList display_list;
    display_list.drawText({1, 1}, "base");
    const auto before = display_list;
    CHECK(!RenderedTreeViewPresentation::render(display_list, snapshot));
    CHECK(std::ranges::equal(display_list.commands(), before.commands()));
}
