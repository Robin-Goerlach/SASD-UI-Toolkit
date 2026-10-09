#include "test_framework.hpp"

#include <sasd/ui/rendered/table_view_presentation.hpp>

#include <string_view>
#include <vector>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {
class Metrics final : public RenderedMeasurementContext {
public:
    [[nodiscard]] Size measureText(std::string_view text) const override {
        return {static_cast<Coordinate>(text.size()), 1};
    }
    [[nodiscard]] Coordinate lineHeight() const noexcept override { return 1; }
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(
        std::string_view, std::size_t scalar_index) const override {
        return static_cast<Coordinate>(scalar_index);
    }
    [[nodiscard]] std::uint64_t revision() const noexcept override { return 1; }
};
} // namespace

TEST_CASE("Rendered TableView owns measured visible cells and uses exact hit geometry") {
    StringTableModel model({"Name", "Wert"}, {{"eins", "one"}, {"zwei", "two"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 2, 0, 2);
    Metrics metrics;

    const auto snapshot = RenderedTableViewPresentation::snapshot(view, {2, 3, 20, 4}, metrics);
    CHECK(snapshot.has_value());
    DisplayList list;
    CHECK(RenderedTableViewPresentation::render(list, *snapshot));
    CHECK(list.size() == 7); // background, two headers and four cells
    CHECK(RenderedTableViewPresentation::hitAt(*snapshot, {2, 4}) ==
          std::optional<RenderedTableViewHit>{RenderedTableViewHit{0, 0}});
    CHECK(!RenderedTableViewPresentation::hitAt(*snapshot, {2, 3}).has_value());
}

TEST_CASE("Rendered TableView rejects malformed snapshots transactionally") {
    Metrics metrics;
    StringTableModel model({"A"}, {{"x"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 1, 0, 1);
    auto snapshot = RenderedTableViewPresentation::snapshot(view, {0, 0, 10, 3}, metrics);
    CHECK(snapshot.has_value());
    DisplayList list;
    list.drawText({0, 0}, "base");
    snapshot->column_bounds[0].width = 0;
    CHECK(!RenderedTableViewPresentation::render(list, *snapshot));
    CHECK(list.size() == 1);
}

TEST_CASE("Rendered TableView hit testing preserves scrolled model column identity") {
    StringTableModel model({"A", "B", "C", "D", "E", "F", "G", "H"},
                           {{"0", "1", "2", "3", "4", "5", "6", "7"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 1, 7, 1);
    Metrics metrics;

    const auto snapshot = RenderedTableViewPresentation::snapshot(view, {0, 0, 4, 2}, metrics);
    CHECK(snapshot.has_value());
    CHECK(snapshot->column_indices == std::vector<std::size_t>{7});
    CHECK(RenderedTableViewPresentation::hitAt(*snapshot, {0, 1}) ==
          std::optional<RenderedTableViewHit>{RenderedTableViewHit{0, 7}});
}

TEST_CASE("Rendered TableView rejects malformed header and row bounds transactionally") {
    Metrics metrics;
    StringTableModel model({"A", "B"}, {{"x", "y"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 1, 0, 2);
    const auto original = RenderedTableViewPresentation::snapshot(view, {0, 0, 10, 3}, metrics);
    CHECK(original.has_value());

    for (const auto malformed : {0, 1}) {
        auto snapshot = *original;
        if (malformed == 0) {
            snapshot.header_bounds.x = -1;
        } else {
            snapshot.row_bounds[0].y = snapshot.header_bounds.y;
        }
        DisplayList list;
        list.drawText({3, 4}, "base");
        CHECK(!RenderedTableViewPresentation::render(list, snapshot));
        CHECK(list.size() == 1);
    }
}

TEST_CASE("Rendered TableView uses half-open final cell boundaries") {
    StringTableModel model({"A", "B"}, {{"x", "y"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 1, 0, 2);
    Metrics metrics;
    const auto snapshot = RenderedTableViewPresentation::snapshot(view, {0, 0, 5, 3}, metrics);
    CHECK(snapshot.has_value());

    CHECK(RenderedTableViewPresentation::hitAt(*snapshot, {1, 1}) ==
          std::optional<RenderedTableViewHit>{RenderedTableViewHit{0, 0}});
    CHECK(RenderedTableViewPresentation::hitAt(*snapshot, {2, 1}) ==
          std::optional<RenderedTableViewHit>{RenderedTableViewHit{0, 1}});
    CHECK(!RenderedTableViewPresentation::hitAt(*snapshot, {4, 1}).has_value());
    CHECK(!RenderedTableViewPresentation::hitAt(*snapshot, {1, 2}).has_value());
}

TEST_CASE("Rendered TableView presents the selected semantic cell") {
    StringTableModel model({"A", "B"}, {{"x", "y"}, {"z", "w"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 2, 0, 2);
    TableSelectionModel selection;
    selection.setModel(&model);
    CHECK(selection.select(1, 1));
    view.setSelectionModel(&selection);
    Metrics metrics;
    const auto snapshot = RenderedTableViewPresentation::snapshot(view, {0, 0, 8, 4}, metrics);
    CHECK(snapshot.has_value());

    DisplayList list;
    CHECK(RenderedTableViewPresentation::render(list, *snapshot));
    const auto& selected = std::get<DrawTextCommand>(list.commands()[6]);
    CHECK(selected.style.inverse);
    CHECK(selected.clip_bounds == Rect{2, 2, 2, 1});
}

TEST_CASE("Rendered TableView rejects stale snapshot selection before applying it") {
    StringTableModel model({"A"}, {{"x"}});
    TableView view;
    view.setModel(&model);
    view.setViewport(0, 1, 0, 1);
    Metrics metrics;
    const auto snapshot = RenderedTableViewPresentation::snapshot(view, {0, 0, 4, 2}, metrics);
    CHECK(snapshot.has_value());
    TableSelectionModel selection;
    selection.setModel(&model);
    model.setCell(0, 0, "changed");
    CHECK(!RenderedTableViewPresentation::selectAt(*snapshot, {0, 1}, selection));
    CHECK(!selection.selectedCell().has_value());
}
