#include <sasd/ui/resource_dictionary.hpp>

#include <cstdlib>
#include <iostream>

int main() {
    using sasd::ui::Color;
    using sasd::ui::ResourceDictionary;
    using sasd::ui::ResourceKey;
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
    std::cout << "theme.heading -> "
              << (resolved->foreground == Color::bright_cyan ? "bright-cyan" : "other")
              << (resolved->bold ? "/bold" : "/regular") << '\n';
    return EXIT_SUCCESS;
}
