#include <sasd/ui/resource_resolution.hpp>

namespace sasd::ui {

std::optional<TextStyleResourceResolution>
resolveTextStyleResource(const ResourceReference& reference,
                         std::span<const ResourceScope> scopes) noexcept {
    // ResourceReference currently validates construction, but resolution owns this type
    // expectation as well. Keeping the check here prevents a future additional resource family
    // from accidentally being looked up in the TextStyle bucket.
    if (reference.kind() != ResourceKind::text_style) {
        return std::nullopt;
    }

    for (std::size_t scope_index = 0; scope_index < scopes.size(); ++scope_index) {
        const auto* value = scopes[scope_index].get().findTextStyle(reference.key());
        if (value != nullptr) {
            // Copy before returning. ResourceDictionary documents that its pointer may be
            // invalidated by the next mutation, while a resolution is an independent value.
            return TextStyleResourceResolution{*value, scope_index};
        }
    }

    return std::nullopt;
}

} // namespace sasd::ui
