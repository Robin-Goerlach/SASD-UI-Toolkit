#pragma once

#include <sasd/ui/resource_key.hpp>

#include <optional>
#include <utility>

namespace sasd::ui {

/**
 * Stable machine-readable identity of a concrete resource family.
 *
 * ResourceKind is type information carried by an authoring reference. ResourceKey remains the
 * identity of one entry inside a dictionary, while the kind lets a consumer verify that a
 * reference is suitable for a property slot before any future lookup is attempted. Enum ordinals
 * are deliberately neither persistence values nor an ABI or JSON contract.
 *
 * Only families that exist as concrete public resource values belong here. TextStyle is currently
 * the sole family; fonts, images, brushes and themes must not be promised before real consumers
 * establish their contracts.
 */
enum class ResourceKind {
    text_style,
};

/** Returns true only for a ResourceKind implemented by the current built-in Core contract. */
[[nodiscard]] bool isBuiltInResourceKind(ResourceKind kind) noexcept;

/**
 * Fully owning typed resource identity for authoring and description layers.
 *
 * A ResourceReference owns its ResourceKey and carries the expected ResourceKind. It borrows no
 * caller memory and stores no dictionary, Component or backend address. In particular, this value
 * is not a runtime binding: creating it does not alter a control's inline TextStyle, perform lookup,
 * select a scope, observe dictionary changes or define inline-versus-resource precedence.
 * Resolution, scoping and durable encoding are intentionally later contracts.
 *
 * Construction uses create() rather than a public aggregate constructor so fabricated enum values
 * cannot produce a formally valid reference. ResourceKey has already validated and copied its
 * input, leaving the factory responsible only for the small built-in kind vocabulary.
 */
class ResourceReference final {
public:
    /**
     * Creates an owning reference, or returns std::nullopt for an unknown ResourceKind.
     * Allocation failure from moving/copying the owned key may propagate.
     */
    [[nodiscard]] static std::optional<ResourceReference> create(ResourceKind kind,
                                                                  ResourceKey key);

    [[nodiscard]] ResourceKind kind() const noexcept { return kind_; }

    /** Returns the owned key; the reference itself controls the returned object's lifetime. */
    [[nodiscard]] const ResourceKey& key() const noexcept { return key_; }

    friend bool operator==(const ResourceReference&, const ResourceReference&) = default;

private:
    ResourceReference(ResourceKind kind, ResourceKey key)
        : kind_(kind), key_(std::move(key)) {}

    ResourceKind kind_;
    ResourceKey key_;
};

} // namespace sasd::ui
