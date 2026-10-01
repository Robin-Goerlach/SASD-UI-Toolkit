#include "test_framework.hpp"

#include <sasd/ui/menu_model.hpp>

#include <memory>
#include <stdexcept>

using namespace sasd::ui;

TEST_CASE("MenuModel preserves title and insertion order") {
    Command open{"Open"};
    Command save{"Save"};
    MenuModel menu{"File"};

    menu.appendCommand(open);
    menu.appendSeparator();
    menu.appendCommand(save);

    CHECK(menu.title() == "File");
    CHECK(menu.itemCount() == 3);
    CHECK(menu.itemAt(0).kind() == MenuItemKind::command);
    CHECK(menu.itemAt(0).command() == &open);
    CHECK(menu.itemAt(1).kind() == MenuItemKind::separator);
    CHECK(menu.itemAt(2).command() == &save);
}

TEST_CASE("Menu command item reads live Command text and enabled state") {
    Command command{"Initial"};
    MenuModel menu{"Tools"};
    menu.appendCommand(command);

    const MenuItem& item = menu.itemAt(0);
    CHECK(item.text() == "Initial");
    CHECK(item.isEnabled());

    /*
     * MenuItem intentionally does not duplicate command state. A menu presenter sees current semantic
     * state whenever it queries the model, avoiding a second cache and another observer lifetime to
     * keep synchronized merely for this first menu-model slice.
     */
    command.setText("Updated");
    command.setEnabled(false);

    CHECK(item.text() == "Updated");
    CHECK(!item.isEnabled());
}

TEST_CASE("Menu command item delegates activation to Command") {
    Command command{"Run"};
    MenuModel menu{"Actions"};
    int executions = 0;
    command.setOnExecuted([&executions] { ++executions; });
    menu.appendCommand(command);

    CHECK(menu.itemAt(0).activate());
    CHECK(executions == 1);

    command.setEnabled(false);
    CHECK(!menu.itemAt(0).activate());
    CHECK(executions == 1);
}

TEST_CASE("Menu item shortcut is display metadata only") {
    Command help{"Help"};
    MenuModel menu{"Help"};
    const Shortcut shortcut{Key::f1, KeyModifier::none};

    menu.appendCommand(help, shortcut);

    CHECK(menu.itemAt(0).shortcut().has_value());
    CHECK(*menu.itemAt(0).shortcut() == shortcut);

    /*
     * No ShortcutMap is involved here. The menu model can advertise the gesture, but routing remains
     * an explicit scope policy rather than a side effect of adding a visible/semantic menu entry.
     */
    CHECK(menu.itemAt(0).command() == &help);
}

TEST_CASE("Menu separator is non-interactive and carries no command state") {
    MenuModel menu{"Edit"};
    menu.appendSeparator();

    const MenuItem& separator = menu.itemAt(0);
    CHECK(separator.kind() == MenuItemKind::separator);
    CHECK(separator.command() == nullptr);
    CHECK(separator.submenu() == nullptr);
    CHECK(separator.text().empty());
    CHECK(!separator.isEnabled());
    CHECK(!separator.shortcut().has_value());
    CHECK(!separator.activate());
}

TEST_CASE("Menu command item becomes safely unavailable after Command destruction") {
    MenuModel menu{"Document"};
    auto command = std::make_unique<Command>("Close");
    menu.appendCommand(*command, Shortcut{Key::f10, KeyModifier::none});

    CHECK(menu.itemAt(0).command() == command.get());
    CHECK(menu.itemAt(0).isEnabled());

    command.reset();

    /*
     * Menu structure survives independently from semantic ownership. Expiration is represented by an
     * unavailable item instead of a dangling raw pointer; shortcut-display metadata remains structural
     * metadata and therefore survives as well.
     */
    CHECK(menu.itemAt(0).command() == nullptr);
    CHECK(menu.itemAt(0).text().empty());
    CHECK(!menu.itemAt(0).isEnabled());
    CHECK(menu.itemAt(0).shortcut().has_value());
    CHECK(!menu.itemAt(0).activate());
}

TEST_CASE("Menu activation tolerates Command self-destruction") {
    MenuModel menu{"Dangerous"};
    auto command = std::make_unique<Command>("Destroy command");
    Command* const original = command.get();

    command->setOnExecuted([&command] { command.reset(); });
    menu.appendCommand(*command);

    CHECK(menu.itemAt(0).command() == original);
    CHECK(menu.itemAt(0).activate());
    CHECK(command == nullptr);

    // The same structural item is now expired and must reject further activation safely.
    CHECK(menu.itemAt(0).command() == nullptr);
    CHECK(!menu.itemAt(0).activate());
}

TEST_CASE("MenuModel owns nested submenu structure recursively") {
    Command recent{"Recent file"};
    MenuModel file{"File"};

    MenuModel& recent_menu = file.appendSubmenu("Recent");
    recent_menu.appendCommand(recent);

    const MenuItem& item = file.itemAt(0);
    CHECK(item.kind() == MenuItemKind::submenu);
    CHECK(item.command() == nullptr);
    CHECK(item.submenu() == &recent_menu);
    CHECK(item.text() == "Recent");
    CHECK(item.isEnabled());
    CHECK(!item.shortcut().has_value());
    CHECK(!item.activate());
    CHECK(recent_menu.itemCount() == 1);
    CHECK(recent_menu.itemAt(0).command() == &recent);
}

TEST_CASE("Submenu builder references survive sibling item growth") {
    MenuModel file{"File"};
    MenuModel& tools = file.appendSubmenu("Tools");

    /*
     * The parent stores MenuItem values in a vector, so those values may move as siblings are appended.
     * The nested MenuModel itself is individually allocated and owned by its submenu item. Its address
     * must therefore remain stable even when the owning MenuItem moves during vector reallocation.
     */
    for (int index = 0; index < 64; ++index) {
        file.appendSeparator();
    }

    CHECK(file.itemAt(0).submenu() == &tools);
    tools.setTitle("Utilities");
    CHECK(file.itemAt(0).text() == "Utilities");
}

TEST_CASE("MenuModel supports recursively nested submenus") {
    Command about{"About"};
    MenuModel root{"Root"};

    MenuModel& first = root.appendSubmenu("First");
    MenuModel& second = first.appendSubmenu("Second");
    second.appendCommand(about);

    const MenuModel* first_from_item = root.itemAt(0).submenu();
    CHECK(first_from_item == &first);
    CHECK(first_from_item->itemAt(0).submenu() == &second);
    CHECK(second.itemAt(0).text() == "About");
}

TEST_CASE("MenuModel clear preserves title and itemAt checks bounds") {
    Command command{"One"};
    MenuModel menu{"Stable title"};
    menu.appendCommand(command);
    menu.appendSeparator();
    (void)menu.appendSubmenu("Nested");

    menu.clear();
    CHECK(menu.title() == "Stable title");
    CHECK(menu.itemCount() == 0);

    bool threw = false;
    try {
        (void)menu.itemAt(0);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}
