#include "test_framework.hpp"

#include <sasd/ui/rendered/menu_bar_presentation.hpp>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {
class Metrics final : public RenderedMeasurementContext {
public:
    [[nodiscard]] std::uint64_t revision() const noexcept override { return 1; }
    [[nodiscard]] Size measureText(std::string_view text) const override {
        return {static_cast<Coordinate>(text.size()), 1};
    }
    [[nodiscard]] Coordinate lineHeight() const noexcept override { return 3; }
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(std::string_view,
                                                                  std::size_t index) const override {
        return static_cast<Coordinate>(index);
    }
};
}

TEST_CASE("Rendered menu bar owns final title geometry and exact half-open hit testing") {
    MenuBarPresentationSnapshot menu;
    menu.titles = {"File", "Edit"};
    menu.selection = 1;
    menu.popup_open = true;

    Metrics metrics;
    const auto snapshot = buildMenuBarPresentation(menu, {-3, -2}, {-10, -10, 50, 30}, metrics);
    CHECK(snapshot.has_value());
    if (!snapshot.has_value()) return;

    CHECK(snapshot->items.size() == 2U);
    CHECK(snapshot->items[0].bounds.x == -3);
    CHECK(snapshot->items[1].bounds.x == snapshot->items[0].bounds.x + snapshot->items[0].bounds.width);
    CHECK(menuBarItemAt(*snapshot, snapshot->items[0].bounds.x == -3
                                   ? Point{-3, -2}
                                   : Point{}) == std::optional<std::size_t>{0});
    CHECK(!menuBarItemAt(*snapshot,
                         {snapshot->items[0].bounds.x + snapshot->items[0].bounds.width,
                          snapshot->items[0].bounds.y})
               .has_value() ||
          menuBarItemAt(*snapshot,
                        {snapshot->items[0].bounds.x + snapshot->items[0].bounds.width,
                         snapshot->items[0].bounds.y}) == std::optional<std::size_t>{1});

    DisplayList list;
    CHECK(renderMenuBarPresentation(list, *snapshot));
    CHECK(list.size() == 3U); // selected background plus two owned title commands
}

TEST_CASE("Rendered empty menu bar is a valid persistent no-op") {
    MenuBarPresentationSnapshot menu;
    Metrics metrics;
    const auto snapshot = buildMenuBarPresentation(menu, {2, 3}, {0, 0, 40, 20}, metrics);
    CHECK(snapshot.has_value());
    if (!snapshot.has_value()) return;
    DisplayList list;
    CHECK(renderMenuBarPresentation(list, *snapshot));
    CHECK(list.empty());
    CHECK(!menuBarItemAt(*snapshot, {2, 3}).has_value());
}

TEST_CASE("Rendered menu bar fails closed when title geometry cannot fit the viewport") {
    MenuBarPresentationSnapshot menu;
    menu.titles = {"A very long title"};
    Metrics metrics;
    CHECK(!buildMenuBarPresentation(menu, {5, 5}, {0, 0, 4, 4}, metrics).has_value());
}
