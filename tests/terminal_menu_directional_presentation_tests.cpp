#include "test_framework.hpp"

#include <sasd/ui/terminal/menu_directional_presentation.hpp>

#include <optional>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal directional popup renderer points active submenu marker to the fitted side") {
    ScreenBuffer buffer{{16, 4}};
    buffer.clear(Cell{U'.'});

    MenuPopupPresentationSnapshot popup;
    popup.selection = std::size_t{1};
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::submenu, "Tools", true, std::nullopt});

    const auto measured = measureMenuPopupPresentation(popup);
    CHECK(measured.has_value());
    CHECK(measured->size == Size{9, 2});

    CHECK(renderDirectionalMenuPopupPresentation(
        buffer,
        {2, 1},
        popup,
        ActiveSubmenuPresentationDirection{1, SubmenuPopupSide::left}));

    /*
     * Direction changes only the reserved marker glyph. The rest of the row remains owned by the ordinary
     * popup renderer, including inverse selection styling across the complete padded span.
     */
    CHECK(buffer.at({9, 2}).code_point == U'<');
    CHECK(buffer.at({9, 2}).style.inverse);
    CHECK(buffer.at({3, 2}).code_point == U'T');
    CHECK(buffer.at({2, 2}).style.inverse);
}

TEST_CASE("Terminal directional popup renderer keeps the normal right marker for right-opening children") {
    ScreenBuffer buffer{{16, 3}};

    MenuPopupPresentationSnapshot popup;
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::submenu, "Tools", true, std::nullopt});

    CHECK(renderDirectionalMenuPopupPresentation(
        buffer,
        {1, 0},
        popup,
        ActiveSubmenuPresentationDirection{0, SubmenuPopupSide::right}));

    const auto measured = measureMenuPopupPresentation(popup);
    CHECK(measured.has_value());
    const Coordinate marker_x = 1 + measured->size.width - 2;
    CHECK(buffer.at({marker_x, 0}).code_point == U'>');
}

TEST_CASE("Terminal directional popup renderer rejects stale direction descriptors transactionally") {
    ScreenBuffer buffer{{12, 3}};
    buffer.clear(Cell{U'Z'});

    MenuPopupPresentationSnapshot popup;
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::command, "Open", true, std::nullopt});

    /* A command row cannot own a child submenu. Rejection must happen before the base renderer clears it. */
    CHECK(!renderDirectionalMenuPopupPresentation(
        buffer,
        {0, 0},
        popup,
        ActiveSubmenuPresentationDirection{0, SubmenuPopupSide::left}));
    CHECK(buffer.at({0, 0}).code_point == U'Z');
    CHECK(buffer.at({11, 2}).code_point == U'Z');

    CHECK(!renderDirectionalMenuPopupPresentation(
        buffer,
        {0, 0},
        popup,
        ActiveSubmenuPresentationDirection{4, SubmenuPopupSide::right}));
    CHECK(buffer.at({0, 0}).code_point == U'Z');
}

TEST_CASE("Terminal directional popup renderer still fails closed for unsupported text") {
    ScreenBuffer buffer{{12, 3}};
    buffer.clear(Cell{U'Q'});

    MenuPopupPresentationSnapshot popup;
    popup.items.push_back(
        MenuItemPresentationSnapshot{MenuItemKind::submenu, "A\xCC\x81", true, std::nullopt});

    CHECK(!renderDirectionalMenuPopupPresentation(
        buffer,
        {0, 0},
        popup,
        ActiveSubmenuPresentationDirection{0, SubmenuPopupSide::left}));
    CHECK(buffer.at({0, 0}).code_point == U'Q');
    CHECK(buffer.at({11, 2}).code_point == U'Q');
}
