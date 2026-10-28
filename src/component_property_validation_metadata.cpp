#include <sasd/ui/component_property_validation_metadata.hpp>

#include <sasd/ui/geometry.hpp>

#include <limits>

namespace sasd::ui {
namespace {

bool isExactBuiltInProperty(const ComponentPropertyMetadata& property) noexcept {
    // Catalog addresses are an implementation detail. Comparing the full descriptor also keeps
    // equal local names on sibling layouts distinct and rejects fabricated enum values.
    const auto* resolved =
        findBuiltInComponentProperty(property.declaring_type_id, property.property_id);
    return resolved != nullptr && resolved->declaring_type_id == property.declaring_type_id &&
           resolved->property_id == property.property_id &&
           resolved->value_kind == property.value_kind;
}

bool isSignedSpacingProperty(const ComponentPropertyMetadata& property) noexcept {
    if (property.property_id == "spacing") {
        return property.declaring_type_id == "sasd.ui.HBox" ||
               property.declaring_type_id == "sasd.ui.VBox";
    }
    if (property.property_id == "columnSpacing" || property.property_id == "rowSpacing") {
        return property.declaring_type_id == "sasd.ui.GridLayout" ||
               property.declaring_type_id == "sasd.ui.FormLayout";
    }
    return false;
}

} // namespace

std::optional<SignedIntegerPropertyRange> builtInSignedIntegerPropertyRange(
    const ComponentPropertyMetadata& property) noexcept {
    if (property.value_kind != ComponentPropertyValueKind::signed_integer ||
        !isExactBuiltInProperty(property) || !isSignedSpacingProperty(property)) {
        return std::nullopt;
    }

    // int64_t is the persistent carrier, but Coordinate is the public semantic property type.
    // Advertising Coordinate::max prevents instance-free tooling from offering values that no
    // runtime layout can represent. The non-negative minimum mirrors the authoritative setters.
    return SignedIntegerPropertyRange{
        0, static_cast<std::int64_t>(std::numeric_limits<Coordinate>::max())};
}

std::optional<UnsignedIntegerPropertyRange> builtInUnsignedIntegerPropertyRange(
    const ComponentPropertyMetadata& property) noexcept {
    if (property.value_kind != ComponentPropertyValueKind::unsigned_integer ||
        !isExactBuiltInProperty(property) ||
        property.declaring_type_id != "sasd.ui.GridLayout" ||
        property.property_id != "columnCount") {
        return std::nullopt;
    }

    // The portable semantic rule is columns > 0. Do not narrow this metadata to size_t: whether a
    // valid uint64_t can be represented by one host belongs to typed access/reconstruction.
    return UnsignedIntegerPropertyRange{1, std::numeric_limits<std::uint64_t>::max()};
}

std::optional<StringPropertyNormalizationKind> builtInStringPropertyNormalization(
    const ComponentPropertyMetadata& property) noexcept {
    if (property.value_kind != ComponentPropertyValueKind::string ||
        !isExactBuiltInProperty(property)) {
        return std::nullopt;
    }

    if (property.declaring_type_id == "sasd.ui.TextField" && property.property_id == "text") {
        return StringPropertyNormalizationKind::single_line_utf8;
    }

    // Identity is listed explicitly for every current descriptor. A future string property must
    // make a conscious policy choice instead of silently inheriting behavior from its value kind.
    if (property.property_id == "text" &&
        (property.declaring_type_id == "sasd.ui.Command" ||
         property.declaring_type_id == "sasd.ui.Label" ||
         property.declaring_type_id == "sasd.ui.Button" ||
         property.declaring_type_id == "sasd.ui.CheckBox" ||
         property.declaring_type_id == "sasd.ui.RadioButton")) {
        return StringPropertyNormalizationKind::identity;
    }

    return std::nullopt;
}

std::optional<SizeConstraintsPropertyValidationKind>
builtInSizeConstraintsPropertyValidation(const ComponentPropertyMetadata& property) noexcept {
    if (property.value_kind != ComponentPropertyValueKind::size_constraints ||
        !isExactBuiltInProperty(property) ||
        property.declaring_type_id != "sasd.ui.Widget" ||
        property.property_id != "sizeConstraints") {
        return std::nullopt;
    }

    // SizeConstraints is one atomic property. This policy mirrors hasValidRange() exactly: preferred
    // and negative coordinates remain legal, while each minimum must not exceed its maximum.
    return SizeConstraintsPropertyValidationKind::minimum_not_greater_than_maximum;
}

} // namespace sasd::ui
