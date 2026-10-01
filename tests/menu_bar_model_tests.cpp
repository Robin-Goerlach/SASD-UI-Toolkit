#include "test_framework.hpp"

#include <sasd/ui/menu_model.hpp>

#include <stdexcept>

using namespace sasd::ui;

TEST_CASE("MenuBarModel owns top-level menus in insertion order") {
    MenuBarModel bar;

    MenuModel& file = bar.appendMenu("File");
    MenuModel& edit = bar.appendMenu("Edit");
    MenuModel& help = bar.appendMenu("Help");

    CHECK(bar.menuCount() == 3);
    CHECK(bar.menuAt(0).title() == "File");
    CHECK(bar.menuAt(1).title() == "Edit");
    CHECK(bar.menuAt(2).title() == "Help");
    CHECK(&bar.menuAt(0) == &file);
    CHECK(&bar.menuAt(1) == &edit);
    CHECK(&bar.menuAt(2) == &help);
}

TEST_CASE("MenuBarModel keeps earlier MenuModel references stable while it grows") {
    MenuBarModel bar;
    MenuModel& file = bar.appendMenu("File");
    Command open{"Open"};
    file.appendCommand(open);

    /*
     * appendMenu() intentionally returns a long-lived builder reference. A vector<MenuModel> would
     * make that reference depend on vector capacity and therefore on an unrelated later append. The
     * model stores individually owned MenuModel objects so extending the bar cannot relocate File.
     */
    for (int index = 0; index < 32; ++index) {
        (void)bar.appendMenu("Extra");
    }

    CHECK(&bar.menuAt(0) == &file);
    CHECK(file.title() == "File");
    CHECK(file.itemCount() == 1);
    CHECK(file.itemAt(0).command() == &open);
}

TEST_CASE("MenuBarModel and Command ownership remain independent") {
    MenuBarModel bar;
    Command save{"Save"};
    MenuModel& file = bar.appendMenu("File");
    file.appendCommand(save);

    bar.clear();

    /*
     * Clearing menu structure must not touch application semantics. MenuBarModel owns MenuModel
     * objects only; Command ownership remains with the application's Component/object graph.
     */
    CHECK(bar.menuCount() == 0);
    CHECK(save.text() == "Save");
    CHECK(save.isEnabled());
}

TEST_CASE("MenuBarModel exposes mutable and const indexed access with bounds checking") {
    MenuBarModel bar;
    bar.appendMenu("Original");

    bar.menuAt(0).setTitle("Updated");

    const MenuBarModel& const_bar = bar;
    CHECK(const_bar.menuAt(0).title() == "Updated");

    bool mutable_threw = false;
    try {
        (void)bar.menuAt(1);
    } catch (const std::out_of_range&) {
        mutable_threw = true;
    }
    CHECK(mutable_threw);

    bool const_threw = false;
    try {
        (void)const_bar.menuAt(1);
    } catch (const std::out_of_range&) {
        const_threw = true;
    }
    CHECK(const_threw);
}
