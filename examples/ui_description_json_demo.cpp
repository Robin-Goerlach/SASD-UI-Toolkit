#include <sasd/ui/ui_description_json.hpp>
#include <sasd/ui/ui_description_reconstruction.hpp>
#include <sasd/ui/button.hpp>
#include <sasd/ui/command.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/resource_key.hpp>
#include <sasd/ui/resource_reference.hpp>
#include <sasd/ui/window.hpp>

#include <algorithm>
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

    // Runtime controls still expose concrete inline values in M8.27. Build the durable authoring
    // document explicitly, replace exactly one inline descriptor with a ResourceReference, and do
    // not claim that parsing the document has established a live runtime binding.
    sasd::ui::Label heading{"Heading"};
    const auto inline_json = sasd::ui::encodeBuiltInUiDescriptionJson(heading);
    if (!inline_json) { return 5; }
    auto v2_parsed = sasd::ui::parseBuiltInUiDescriptionJson(*inline_json);
    if (!v2_parsed) { return 6; }
    auto v2_document = std::move(*v2_parsed.document);
    auto key = sasd::ui::ResourceKey::create("theme.heading");
    if (!key) { return 7; }
    v2_document.text_style_resources.push_back(
        {*key, {sasd::ui::Color::bright_cyan, true, false, false, false}});
    auto& label_node = v2_document.nodes.front();
    const auto inline_style = std::find_if(
        label_node.text_style_properties.begin(), label_node.text_style_properties.end(),
        [](const auto& property) {
            return property.declaring_type_id == "sasd.ui.Label" &&
                   property.property_id == "textStyle";
        });
    if (inline_style == label_node.text_style_properties.end()) { return 8; }
    label_node.text_style_properties.erase(inline_style);
    auto reference = sasd::ui::ResourceReference::create(
        sasd::ui::ResourceKind::text_style, *key);
    if (!reference) { return 9; }
    label_node.resource_reference_properties.push_back(
        {"sasd.ui.Label", "textStyle", std::move(*reference)});
    const auto v2_json = sasd::ui::encodeBuiltInUiDescriptionJsonV2(v2_document);
    if (!v2_json) { return 10; }
    const auto loaded_v2 = sasd::ui::parseBuiltInUiDescriptionJsonV2(*v2_json);
    if (!loaded_v2) { return 11; }
    std::cout << "\nDocument-local resource: theme.heading\n"
              << "Property: sasd.ui.Label.textStyle -> theme.heading\n"
              << "JSON formatVersion: 2\n"
              << "Durable description only; runtime resource reconstruction is not active.\n"
              << *v2_json;
}
