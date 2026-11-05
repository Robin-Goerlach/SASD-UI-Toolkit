#include <sasd/ui/component_property_resource_metadata.hpp>
#include <sasd/ui/resource_dictionary.hpp>
#include <sasd/ui/resource_reference.hpp>
#include <sasd/ui/resource_resolution.hpp>

#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>

int main() {
    using sasd::ui::Color;
    using sasd::ui::ResourceDictionary;
    using sasd::ui::ResourceKey;
    using sasd::ui::ResourceKind;
    using sasd::ui::ResourceReference;
    using sasd::ui::ResourceScope;
    using sasd::ui::TextStyle;

    ResourceDictionary application_resources;
    ResourceDictionary local_resources;
    const auto heading_key = ResourceKey::create("theme.heading");
    const auto error_key = ResourceKey::create("theme.error");
    if (!heading_key || !error_key) {
        return EXIT_FAILURE;
    }

    const TextStyle application_heading{Color::blue, false, false, false, false};
    const TextStyle local_heading{Color::bright_cyan, true, false, false, false};
    const TextStyle error{Color::bright_red, true, false, true, false};
    if (!application_resources.addTextStyle(*heading_key, application_heading) ||
        !application_resources.addTextStyle(*error_key, error) ||
        !local_resources.addTextStyle(*heading_key, local_heading)) {
        return EXIT_FAILURE;
    }

    const auto* label_text_style =
        sasd::ui::findBuiltInComponentProperty("sasd.ui.Label", "textStyle");
    if (label_text_style == nullptr) {
        return EXIT_FAILURE;
    }
    const auto metadata =
        sasd::ui::builtInComponentPropertyResourceMetadata(*label_text_style);
    const auto reference = ResourceReference::create(ResourceKind::text_style, *heading_key);
    if (!metadata || metadata->resource_kind != ResourceKind::text_style || !reference) {
        return EXIT_FAILURE;
    }

    const std::array<ResourceScope, 2> local_then_application{
        std::cref(local_resources), std::cref(application_resources)};
    const std::array<ResourceScope, 2> application_then_local{
        std::cref(application_resources), std::cref(local_resources)};
    const auto local_resolution =
        sasd::ui::resolveTextStyleResource(*reference, local_then_application);
    const auto application_resolution =
        sasd::ui::resolveTextStyleResource(*reference, application_then_local);
    if (!local_resolution || !application_resolution) {
        return EXIT_FAILURE;
    }

    std::cout << "Reference: " << reference->key().value() << '\n'
              << "Local, application order:\n"
              << "  scope " << local_resolution->scope_index << " -> bright-cyan/bold\n"
              << "Application, local order:\n"
              << "  scope " << application_resolution->scope_index << " -> blue/regular\n";

    if (local_resolution->value != local_heading || local_resolution->scope_index != 0 ||
        application_resolution->value != application_heading ||
        application_resolution->scope_index != 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
