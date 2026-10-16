#include <sasd/ui/native/appkit_backend.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/menu_model.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <chrono>
#include <thread>
#include <iostream>

int main() {
    using namespace sasd::ui;
    native::appkit::AppKitWindowBackend backend{{"SASD UI Toolkit AppKit", {800, 500}, false, true,
                                                  12.0, Color::black}};
    backend.initialize();
    Command exit_command{"Exit"};
    bool exit_requested = false;
    exit_command.setOnExecuted([&] { exit_requested = true; });
    Command open_file_command{"Open File"};
    open_file_command.setOnExecuted([&] {
        if (auto* dialogs = backend.fileDialogs()) (void)dialogs->requestOpenFile({"Open File"});
    });
    Command copy_command{"Copy"};
    Command paste_command{"Paste"};
    Command about_command{"About"};
    Command disabled_command{"Disabled item"};
    disabled_command.setEnabled(false);
    MenuBarModel menu_bar;
    MenuModel& file_menu = menu_bar.appendMenu("File");
    file_menu.appendCommand(open_file_command);
    MenuModel& action_menu = file_menu.appendSubmenu("Action");
    action_menu.appendCommand(disabled_command);
    file_menu.appendSeparator();
    file_menu.appendCommand(exit_command);
    MenuModel& edit_menu = menu_bar.appendMenu("Edit");
    edit_menu.appendCommand(copy_command);
    edit_menu.appendCommand(paste_command);
    MenuModel& help_menu = menu_bar.appendMenu("Help");
    help_menu.appendCommand(about_command);
    backend.syncMenuBar(menu_bar);
    Window window;
    auto& label = window.emplace<Label>("SASD UI Toolkit - AppKit Rendered Host");
    auto& button = window.emplace<Button>("Button");
    auto& field = window.emplace<TextField>("TextField: committed UTF-8 input");
    window.arrange({0, 0, 800, 500});
    label.arrange({42, 52, 500, 24}); button.arrange({24, 130, 220, 42}); field.arrange({24, 190, 420, 42});
    rendered::DisplayList frame;
    rendered::RenderedPresentationSink presentation{frame, backend, Color::black};
    if (presentation.synchronize(window) != PresentationUpdateResult::synchronized) return 1;
    (void)backend.presentFrame(frame);
    for (;;) {
        const auto event = backend.pollEvent();
        if (event && std::holds_alternative<QuitEvent>(*event)) break;
        if (event && std::holds_alternative<CommandActivationEvent>(*event)) {
            if (Command* command = std::get<CommandActivationEvent>(*event).command.get()) {
                (void)command->execute();
            }
        }
        if (event && std::holds_alternative<FileDialogResultEvent>(*event)) {
            const auto& result = std::get<FileDialogResultEvent>(*event);
            if (result.status == FileDialogResultStatus::accepted) {
                std::cout << "Opened: " << result.path->string() << '\n';
            } else if (result.status == FileDialogResultStatus::cancelled) {
                std::cout << "Open cancelled\n";
            } else {
                std::cout << "Open failed\n";
            }
        }
        if (exit_requested) break;
        std::this_thread::sleep_for(std::chrono::milliseconds{16});
    }
    backend.shutdown();
}
