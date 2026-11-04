#include <sasd/ui/resource_reference.hpp>

#include <utility>

namespace sasd::ui {

bool isBuiltInResourceKind(ResourceKind kind) noexcept {
    switch (kind) {
    case ResourceKind::text_style: return true;
    }

    // Enum values can be fabricated through a cast. Treating such a value as a known family would
    // erase the type-safety that ResourceKind adds to ResourceKey identity, so fail closed.
    return false;
}

std::optional<ResourceReference> ResourceReference::create(ResourceKind kind, ResourceKey key) {
    if (!isBuiltInResourceKind(kind)) {
        return std::nullopt;
    }

    return ResourceReference{kind, std::move(key)};
}

} // namespace sasd::ui
