#include <sasd/ui/resource_dictionary.hpp>

#include <utility>

namespace sasd::ui {

bool ResourceDictionary::addTextStyle(ResourceKey key, TextStyle value) {
    if (containsKey(key.value())) {
        // Resource identity is authoring identity. Silently replacing the first value would hide a
        // duplicate declaration or typo and make result order-dependent, so preserve it unchanged.
        return false;
    }

    text_styles_.push_back(TextStyleResource{std::move(key), value});
    return true;
}

const TextStyle* ResourceDictionary::findTextStyle(std::string_view key) const noexcept {
    // Stored keys can only contain the ResourceKey grammar. Consequently any invalid textual input
    // naturally misses without allocation, throwing, normalization or a second grammar copy here.
    for (const auto& resource : text_styles_) {
        if (resource.key.value() == key) {
            return &resource.value;
        }
    }
    return nullptr;
}

const TextStyle* ResourceDictionary::findTextStyle(const ResourceKey& key) const noexcept {
    return findTextStyle(key.value());
}

bool ResourceDictionary::containsKey(std::string_view key) const noexcept {
    for (const auto& resource : text_styles_) {
        if (resource.key.value() == key) {
            return true;
        }
    }
    return false;
}

} // namespace sasd::ui
