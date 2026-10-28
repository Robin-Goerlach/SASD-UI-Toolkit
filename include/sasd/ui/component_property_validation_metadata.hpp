#pragma once

#include <sasd/ui/component_property_metadata.hpp>

#include <cstdint>
#include <optional>

namespace sasd::ui {

/** Static inclusive domain for one signed_integer property. */
struct SignedIntegerPropertyRange final {
    std::int64_t minimum{};
    std::int64_t maximum{};

    [[nodiscard]] constexpr bool contains(std::int64_t value) const noexcept {
        return value >= minimum && value <= maximum;
    }

    friend constexpr bool operator==(const SignedIntegerPropertyRange&,
                                     const SignedIntegerPropertyRange&) = default;
};

/** Static inclusive domain for one unsigned_integer property. */
struct UnsignedIntegerPropertyRange final {
    std::uint64_t minimum{};
    std::uint64_t maximum{};

    [[nodiscard]] constexpr bool contains(std::uint64_t value) const noexcept {
        return value >= minimum && value <= maximum;
    }

    friend constexpr bool operator==(const UnsignedIntegerPropertyRange&,
                                     const UnsignedIntegerPropertyRange&) = default;
};

/**
 * Returns the static signed domain of an exact built-in property descriptor.
 *
 * Descriptor identity is the complete declaring-type/property/kind value, not its address. Unknown,
 * malformed, wrong-kind and non-integer descriptors return std::nullopt. Runtime setters remain the
 * authoritative enforcement boundary; this metadata lets tooling inspect the same static contract
 * without constructing a Component.
 */
[[nodiscard]] std::optional<SignedIntegerPropertyRange> builtInSignedIntegerPropertyRange(
    const ComponentPropertyMetadata& property) noexcept;

/**
 * Returns the static unsigned domain of an exact built-in property descriptor.
 *
 * The range describes portable semantic values. Host representability required by a concrete
 * runtime setter is deliberately checked later by typed property access or reconstruction.
 */
[[nodiscard]] std::optional<UnsignedIntegerPropertyRange> builtInUnsignedIntegerPropertyRange(
    const ComponentPropertyMetadata& property) noexcept;

} // namespace sasd::ui
