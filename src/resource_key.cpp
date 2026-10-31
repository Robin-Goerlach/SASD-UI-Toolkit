#include <sasd/ui/resource_key.hpp>

#include <utility>

namespace sasd::ui {
namespace {

[[nodiscard]] constexpr bool isAsciiResourceKeyByte(unsigned char value) noexcept {
    const bool is_upper = value >= static_cast<unsigned char>('A') &&
                          value <= static_cast<unsigned char>('Z');
    const bool is_lower = value >= static_cast<unsigned char>('a') &&
                          value <= static_cast<unsigned char>('z');
    const bool is_digit = value >= static_cast<unsigned char>('0') &&
                          value <= static_cast<unsigned char>('9');
    return is_upper || is_lower || is_digit || value == static_cast<unsigned char>('_') ||
           value == static_cast<unsigned char>('-') || value == static_cast<unsigned char>('.') ||
           value == static_cast<unsigned char>('/');
}

} // namespace

std::optional<ResourceKey> ResourceKey::create(std::string_view value) {
    if (value.empty()) {
        return std::nullopt;
    }

    // Validate bytes, not locale characters. Casting first makes bytes >= 0x80 unambiguously
    // invalid on platforms where plain char is signed and avoids every locale-sensitive ctype API.
    for (const char byte : value) {
        if (!isAsciiResourceKeyByte(static_cast<unsigned char>(byte))) {
            return std::nullopt;
        }
    }

    return ResourceKey{std::string{value}};
}

} // namespace sasd::ui
