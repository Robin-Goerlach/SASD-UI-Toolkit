#include <sasd/ui/component_property_resource_metadata.hpp>
#include <sasd/ui/resource_dictionary.hpp>
#include <sasd/ui/resource_reference.hpp>

#include <cstdlib>
#include <iostream>

int main() {
    using sasd::ui::Color;
    using sasd::ui::ResourceDictionary;
    using sasd::ui::ResourceKey;
    using sasd::ui::ResourceKind;
    using sasd::ui::ResourceReference;
    using sasd::ui::TextStyle;

    ResourceDictionary resources;
    const auto heading_key = ResourceKey::create("theme.heading");
    const auto error_key = ResourceKey::create("theme.error");
    if (!heading_key || !error_key) {
        return EXIT_FAILURE;
    }

    const TextStyle heading{Color::bright_cyan, true, false, false, false};
    const TextStyle error{Color::bright_red, true, false, true, false};
    if (!resources.addTextStyle(*heading_key, heading) ||
        !resources.addTextStyle(*error_key, error)) {
        return EXIT_FAILURE;
    }

    std::cout << "Resources:\n";
    for (const auto& resource : resources.textStyles()) {
        std::cout << "  " << resource.key.value() << '\n';
    }

    const auto* resolved = resources.findTextStyle("theme.heading");
    if (resolved == nullptr) {
        return EXIT_FAILURE;
    }
    std::cout << "Dictionary lookup (not reference resolution): theme.heading -> "
              << (resolved->foreground == Color::bright_cyan ? "bright-cyan" : "other")
              << (resolved->bold ? "/bold" : "/regular") << '\n';

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

    std::cout << "Resource metadata:\n"
              << "  sasd.ui.Label.textStyle -> text-style resource capable\n"
              << "Reference identity:\n"
              << "  " << reference->key().value() << '\n'
              << "Resolution and scoping: deferred to M8.26\n";
    return EXIT_SUCCESS;
}
