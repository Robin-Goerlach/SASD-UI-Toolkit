// SPDX-License-Identifier: MIT
//
// Standalone headless consumer. It exercises the installed *public* Core API.
// A real terminal UI also requires TerminalBackend, an event loop and a
// presentation sink; use examples/terminal_form_demo.cpp for those contracts.
#include <sasd/ui/button.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/table_model.hpp>
#include <sasd/ui/table_selection_model.hpp>
#include <sasd/ui/table_view.hpp>
#include <sasd/ui/vbox.hpp>
#include <sasd/ui/window.hpp>

#include <iostream>

int main() {
    using namespace sasd::ui;

    // Models and Commands outlive the Window's references and bindings.
    StringTableModel services{
        {"Service", "Status"},
        {{"dns", "running"}, {"proxy", "stopped"}}};
    TableSelectionModel selection;
    selection.setModel(&services);

    Command reload{"Reload selected service"};
    bool reload_invoked = false;
    reload.setOnExecuted([&reload_invoked] {
        // Pure demonstration: no real services are changed.
        reload_invoked = true;
    });

    Window window;
    auto& column = window.emplace<VBox>();
    column.setSpacing(1);
    auto& message = column.emplace<Label>("Ready");
    auto& table = column.emplace<TableView>();
    table.setModel(&services);
    table.setSelectionModel(&selection);
    table.setViewport(0, 2, 0, 2);

    auto& button = column.emplace<Button>();
    button.bindCommand(reload);

    // A view snapshot is data, not proof that any backend rendered a frame.
    if (!selection.select(0, 1)) {
        return 1;
    }
    const auto rows = table.visibleRows();
    if (rows.size() != 2 || rows.front().cells.size() != 2 ||
        rows.front().cells[0] != "dns" ||
        rows.front().selected_column != 1) {
        return 2;
    }

    if (!button.activate() || !reload_invoked) {
        return 3;
    }
    reload.setEnabled(false);
    if (button.activate()) {
        return 4;
    }

    // Keep user-facing status in an owned semantic widget.
    message.setText("Completed headless API test");
    std::cout << message.text() << '\n';
    return 0;
}
