#include <sasd/ui/ui_description_json.hpp>
#include <sasd/ui/ui_description_reconstruction.hpp>
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
    const auto parsed = sasd::ui::parseBuiltInUiDescriptionJson(*json);
    if (!parsed) { return 2; }
    const auto& document = *parsed.document;
    std::cout << "Parsed UI description:\n"
              << "  nodes: " << document.nodes.size() << '\n'
              << "  root: " << document.root_node_id << '\n'
              << "  references: " << document.reference_edges.size() << '\n'
              << "  root type: " << document.nodes.front().type_id << '\n';
    auto reconstructed = sasd::ui::reconstructBuiltInUiDescription(document);
    if (!reconstructed) { return 3; }
    const auto reconstructed_json =
        sasd::ui::encodeBuiltInUiDescriptionJson(*reconstructed.root);
    if (!reconstructed_json || *reconstructed_json != *json) { return 4; }
    std::cout << "Reconstructed canonical UI description matches the original.\n";
}
