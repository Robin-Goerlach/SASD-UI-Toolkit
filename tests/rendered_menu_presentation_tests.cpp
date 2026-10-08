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

TEST_CASE("Rendered menu popup reserves owned shortcut and submenu lanes") {
    MenuPopupPresentationSnapshot menu;
    menu.items.push_back({MenuItemKind::command, "Open", true,
                          Shortcut{Key::o, KeyModifier::control}});
    menu.items.push_back({MenuItemKind::separator, "", false, std::nullopt});
    menu.items.push_back({MenuItemKind::submenu, "More", true, std::nullopt});

    Metrics metrics;
    const auto snapshot = buildMenuPopupPresentation(menu, {-4, -3}, {-10, -10, 80, 60}, metrics);
    CHECK(snapshot.has_value());
    if (!snapshot.has_value()) {
        return;
    }

    CHECK(snapshot->rows.size() == 3U);
    CHECK(snapshot->rows[0].shortcut_text == "Ctrl+O");
    CHECK(snapshot->shortcut_lane_width > 0);
    CHECK(snapshot->submenu_indicator_lane_width == 1);
    CHECK(snapshot->rows[0].shortcut_bounds.width == snapshot->shortcut_lane_width);
    CHECK(snapshot->rows[1].shortcut_bounds.width == snapshot->shortcut_lane_width);
    CHECK(snapshot->rows[2].submenu_indicator_bounds.width == 1);

    DisplayList list;
    CHECK(renderMenuPopupPresentation(list, *snapshot));
    CHECK(list.size() == 6U); // surface, border, label+shortcut, label, label+indicator
}

TEST_CASE("Rendered menu popup paints disabled rows and selection across the complete row") {
    MenuPopupPresentationSnapshot menu;
    menu.items.push_back({MenuItemKind::command, "Disabled", false, std::nullopt});
    menu.items.push_back({MenuItemKind::command, "Selected", true, std::nullopt});
    menu.selection = 1;

    Metrics metrics;
    const auto snapshot = buildMenuPopupPresentation(menu, {0, 0}, {0, 0, 80, 40}, metrics);
    CHECK(snapshot.has_value());
    if (!snapshot.has_value()) {
        return;
    }

    DisplayList list;
    CHECK(renderMenuPopupPresentation(list, *snapshot));
    CHECK(menuPopupRowAt(*snapshot, {snapshot->rows[1].bounds.x, snapshot->rows[1].bounds.y}) ==
          std::optional<std::size_t>{1});
    CHECK(menuPopupRowAt(*snapshot, {snapshot->rows[1].bounds.x + snapshot->rows[1].bounds.width - 1,
                                     snapshot->rows[1].bounds.y + snapshot->rows[1].bounds.height - 1}) ==
          std::optional<std::size_t>{1});
}

TEST_CASE("Rendered menu popup rejects malformed public snapshots transactionally") {
    DisplayList list;
    list.drawText({1, 1}, "base");
    const DisplayList before = list;

    RenderedMenuPopupPresentationSnapshot malformed;
    malformed.bounds = {0, 0, 20, 10};
    malformed.content_bounds = {1, 1, 18, 8};
    malformed.rows.push_back(RenderedMenuPopupRow{
        MenuItemPresentationSnapshot{MenuItemKind::command, "bad", true, std::nullopt},
        Rect{1, 1, 18, 4},
    });
    malformed.selection = 3;
    malformed.padding = 1;
    malformed.row_height = 4;
    malformed.border_thickness = 1;

    CHECK(!renderMenuPopupPresentation(list, malformed));
    CHECK(list.size() == before.size());
    CHECK(list.commands()[0] == before.commands()[0]);
    CHECK(!menuPopupRowAt(malformed, {2, 2}).has_value());
}
