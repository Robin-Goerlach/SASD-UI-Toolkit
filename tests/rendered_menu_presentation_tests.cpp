#include "test_framework.hpp"

#include <sasd/ui/rendered/menu_presentation.hpp>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {
class Metrics final : public RenderedMeasurementContext {
public:
    [[nodiscard]] std::uint64_t revision() const noexcept override { return 1; }
    [[nodiscard]] Size measureText(std::string_view text) const override {
        return {static_cast<Coordinate>(text.size()), 1};
    }
    [[nodiscard]] Coordinate lineHeight() const noexcept override { return 2; }
    [[nodiscard]] std::optional<Coordinate> textAdvanceToScalar(std::string_view, std::size_t index) const override {
        return static_cast<Coordinate>(index);
    }
};
}

TEST_CASE("Rendered menu popup owns final geometry for rendering and hit testing") {
    MenuPopupPresentationSnapshot menu;
    menu.items.push_back({MenuItemKind::command, "Open", true, std::nullopt});
    menu.items.push_back({MenuItemKind::separator, "", false, std::nullopt});
    menu.selection = 0;
    Metrics metrics;
    const auto snapshot = buildMenuPopupPresentation(menu, {2, 3}, {0, 0, 40, 30}, metrics);
    CHECK(snapshot.has_value());
    if (!snapshot.has_value()) {
        return;
    }
    CHECK(menuPopupRowAt(*snapshot, {3, 4}) == std::optional<std::size_t>{0});
    CHECK(!menuPopupRowAt(*snapshot, {2, 3}).has_value());
    DisplayList list;
    CHECK(renderMenuPopupPresentation(list, *snapshot));
    CHECK(list.size() == 4);
}

TEST_CASE("Rendered menu popup rejects placement outside viewport before painting") {
    MenuPopupPresentationSnapshot menu;
    menu.items.push_back({MenuItemKind::command, "Too wide", true, std::nullopt});
    Metrics metrics;
    CHECK(!buildMenuPopupPresentation(menu, {5, 5}, {0, 0, 4, 4}, metrics).has_value());
}
