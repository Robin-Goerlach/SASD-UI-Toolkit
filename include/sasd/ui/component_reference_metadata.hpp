#pragma once

#include <sasd/ui/component_property_metadata.hpp>
#include <sasd/ui/component_reference_snapshot.hpp>

#include <span>
#include <string_view>

namespace sasd::ui {

/**
 * Immutable, format-neutral type contract for one built-in Component reference kind.
 *
 * The type IDs name the required source and target contracts. Actual built-in endpoints may be
 * those exact types or derive from them through ComponentTypeMetadata::base_type_id. JSON symbols
 * deliberately remain in the JSON encoding layer: other tooling can consume this catalog without
 * inheriting one persistence format's names.
 *
 * Every built-in reference kind is an optional singular relation per source Component. A source
 * may therefore have at most one edge of one kind, while the same source may use other kinds too.
 */
struct BuiltInComponentReferenceMetadata final {
    BuiltInComponentReferenceKind kind{BuiltInComponentReferenceKind::button_command};
    std::string_view source_type_id{};
    std::string_view target_type_id{};
};

/** The concrete graph-level validation pattern currently present in the built-in catalog. */
enum class BuiltInComponentReferenceGraphValidationKind {
    at_most_one_source_boolean_true_per_target,
};

/**
 * Additional semantic graph constraint for a built-in reference kind.
 *
 * Reference endpoint compatibility and graph constraints remain separate because every reference
 * has exactly one source/target contract but may have zero or more additional graph rules. The
 * complete source-property descriptor prevents an unrelated same-named property from participating.
 */
struct BuiltInComponentReferenceGraphValidationMetadata final {
    BuiltInComponentReferenceKind reference_kind{
        BuiltInComponentReferenceKind::radio_button_group};
    BuiltInComponentReferenceGraphValidationKind kind{
        BuiltInComponentReferenceGraphValidationKind::
            at_most_one_source_boolean_true_per_target};
    ComponentPropertyMetadata source_property;
};

/** Returns all built-in reference contracts in their deterministic semantic catalog order. */
[[nodiscard]] std::span<const BuiltInComponentReferenceMetadata>
builtInComponentReferenceMetadata() noexcept;

/** Returns the exact immutable contract for a known kind, or nullptr for a fabricated kind. */
[[nodiscard]] const BuiltInComponentReferenceMetadata*
findBuiltInComponentReferenceMetadata(BuiltInComponentReferenceKind kind) noexcept;

/**
 * Returns the zero-or-more additional graph rules for one reference kind.
 *
 * Unknown kinds and known kinds without graph constraints both return an empty span. Callers that
 * need to distinguish those cases first use findBuiltInComponentReferenceMetadata().
 */
[[nodiscard]] std::span<const BuiltInComponentReferenceGraphValidationMetadata>
builtInComponentReferenceGraphValidations(BuiltInComponentReferenceKind kind) noexcept;

} // namespace sasd::ui
