#include <sasd/ui/ui_description_json.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/window.hpp>

#include <iostream>

int main() {
    sasd::ui::Window root;
    auto& command = root.emplace<sasd::ui::Command>("Save");
    root.emplace<sasd::ui::Button>("Save").bindCommand(command);
    const auto json = sasd::ui::encodeBuiltInUiDescriptionJson(root);
    if (!json) { return 1; }
    std::cout << *json;
}
