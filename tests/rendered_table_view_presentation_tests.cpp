#include "test_framework.hpp"

#include <sasd/ui/rendered/table_view_presentation.hpp>

#include <string_view>

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
