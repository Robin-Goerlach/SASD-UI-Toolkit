#pragma once

#include <sasd/ui/component_property_metadata.hpp>

#include <cstdint>
#include <optional>

namespace sasd::ui {

/** Semantic normalization applied by the runtime setter of a string property. */
enum class StringPropertyNormalizationKind {
    identity,
    single_line_utf8,
};

/** Atomic validity policy for a SizeConstraints property value. */
enum class SizeConstraintsPropertyValidationKind {
    minimum_not_greater_than_maximum,
};

/** Same-node relation supported for an optional unsigned integer property. */
enum class OptionalUnsignedIntegerCrossPropertyValidationKind {
    index_within_string_collection,
};

/**
 * Contextual validity policy and its exact same-node dependency descriptor.
 *
 * The concrete type deliberately describes only the relation currently present in the built-in
 * catalog. It is not a generic expression, predicate or dependency-graph vocabulary.
 */
struct OptionalUnsignedIntegerCrossPropertyValidation final {
    OptionalUnsignedIntegerCrossPropertyValidationKind kind{
        OptionalUnsignedIntegerCrossPropertyValidationKind::index_within_string_collection};
    ComponentPropertyMetadata dependency_property;

    friend constexpr bool operator==(
        const OptionalUnsignedIntegerCrossPropertyValidation& left,
        const OptionalUnsignedIntegerCrossPropertyValidation& right) noexcept {
        return left.kind == right.kind &&
               left.dependency_property.declaring_type_id ==
                   right.dependency_property.declaring_type_id &&
               left.dependency_property.property_id == right.dependency_property.property_id &&
               left.dependency_property.value_kind == right.dependency_property.value_kind;
    }
};

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

/**
 * Returns the normalization policy of an exact built-in string property descriptor.
 *
 * An engaged identity result is deliberate: it distinguishes a known string property which adds no
 * semantic normalization from an unknown, malformed or wrong-kind descriptor. The runtime setter
 * remains authoritative; tooling may use this instance-free policy to compute the same canonical
 * value without constructing a Component.
 */
[[nodiscard]] std::optional<StringPropertyNormalizationKind>
builtInStringPropertyNormalization(const ComponentPropertyMetadata& property) noexcept;

/**
 * Returns the atomic validity policy of an exact built-in SizeConstraints property descriptor.
 *
 * This describes only invariants of the compound value itself. Contextual cross-property and graph
 * rules use separate concrete contracts, and runtime setters continue to enforce their own
 * contract.
 */
[[nodiscard]] std::optional<SizeConstraintsPropertyValidationKind>
builtInSizeConstraintsPropertyValidation(const ComponentPropertyMetadata& property) noexcept;

/**
 * Returns the same-node contextual policy of an exact optional-unsigned property descriptor.
 *
 * The dependency is the complete catalog descriptor rather than a loose property-name string.
 * A copied valid input descriptor succeeds; malformed, wrong-kind and unknown descriptors return
 * std::nullopt. No Component instance is constructed, and this metadata does not define property
 * restoration order.
 */
[[nodiscard]] std::optional<OptionalUnsignedIntegerCrossPropertyValidation>
builtInOptionalUnsignedIntegerCrossPropertyValidation(
    const ComponentPropertyMetadata& property) noexcept;

} // namespace sasd::ui
