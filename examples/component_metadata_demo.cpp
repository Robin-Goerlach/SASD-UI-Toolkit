#include <sasd/ui/component_metadata.hpp>

#include <iostream>

namespace {

const char* kindName(sasd::ui::ComponentTypeKind kind) noexcept {
    switch (kind) {
    case sasd::ui::ComponentTypeKind::component:
        return "component";
    case sasd::ui::ComponentTypeKind::widget:
        return "widget";
    case sasd::ui::ComponentTypeKind::container:
        return "container";
    }
    return "unknown";
}

} // namespace

int main() {
    for (const auto& metadata : sasd::ui::builtInComponentTypes()) {
        std::cout << metadata.type_id << " <- "
                  << (metadata.base_type_id.empty() ? "<none>" : metadata.base_type_id)
                  << " [" << kindName(metadata.kind) << "]\n";
    }

    const auto* button = sasd::ui::findBuiltInComponentType("sasd.ui.Button");
    std::cout << "lookup sasd.ui.Button: " << (button != nullptr ? "found" : "not found") << '\n';
    const auto* unknown = sasd::ui::findBuiltInComponentType("sasd.ui.DoesNotExist");
    std::cout << "lookup sasd.ui.DoesNotExist: " << (unknown != nullptr ? "found" : "not found")
              << '\n';
}
