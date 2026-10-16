#include <sasd/ui/native/gtk_backend.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/menu_model.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    using namespace sasd::ui;
    native::gtk::GtkWindowBackend backend{{"SASD UI Toolkit GTK4", {800, 500}, false, true,
                                           12.0, Color::black}};
    backend.initialize();

    bool exit_requested = false;
    bool option_enabled = false;
    Command action{"Action"};
    Command open_file{"Open File"};
    open_file.setOnExecuted([&] {
        if (auto* dialogs = backend.fileDialogs()) {
            (void)dialogs->requestOpenFile({"Open an existing file"});
        }
    });
    Command exit_command{"Exit"};
    exit_command.setOnExecuted([&] { exit_requested = true; });
    Command copy{"Copy"};
    Command paste{"Paste"};
    paste.setEnabled(false);
    Command option_a{"Enable Option A"};
    option_a.setOnExecuted([&] {
        option_enabled = !option_enabled;
        option_a.setText(option_enabled ? "Disable Option A" : "Enable Option A");
    });
    Command about{"About"};
    MenuBarModel menu_bar;
    MenuModel& file = menu_bar.appendMenu("File");
    file.appendCommand(open_file);
    file.appendCommand(action);
    file.appendSeparator();
    file.appendCommand(exit_command);
    MenuModel& edit = menu_bar.appendMenu("Edit");
    edit.appendCommand(copy);
    edit.appendCommand(paste);
    MenuModel& view = menu_bar.appendMenu("View");
    view.appendSubmenu("Options").appendCommand(option_a);
    menu_bar.appendMenu("Help").appendCommand(about);
    backend.syncMenuBar(menu_bar);

    Window window;
    auto& label = window.emplace<Label>("SASD UI Toolkit - GTK4 Rendered Host");
    auto& button = window.emplace<Button>("Button");
    auto& field = window.emplace<TextField>("TextField: committed UTF-8 input");
    window.arrange({0, 0, 800, 500});
    label.arrange({42, 52, 500, 24});
    button.arrange({24, 130, 220, 42});
    field.arrange({24, 190, 420, 42});

    rendered::DisplayList frame;
    rendered::RenderedPresentationSink presentation{frame, backend, Color::black};
    if (presentation.synchronize(window) != PresentationUpdateResult::synchronized) return 1;
    (void)backend.presentFrame(frame);

    for (int ticks = 0; ticks != 600 && !exit_requested; ++ticks) {
        if (const auto event = backend.pollEvent(); event.has_value()) {
            if (std::holds_alternative<CommandActivationEvent>(*event)) {
                const Command::Reference command = std::get<CommandActivationEvent>(*event).command;
                if (Command* const live = command.get()) (void)live->execute();
            } else if (std::holds_alternative<FileDialogResultEvent>(*event)) {
                const auto& result = std::get<FileDialogResultEvent>(*event);
                if (result.status == FileDialogResultStatus::accepted) {
                    std::cout << "Opened: " << result.path->string() << '\n';
                } else if (result.status == FileDialogResultStatus::cancelled) {
                    std::cout << "Open cancelled\n";
                } else {
                    std::cout << "Open failed\n";
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{16});
    }
    backend.clearMenuBar();
    backend.shutdown();
}
