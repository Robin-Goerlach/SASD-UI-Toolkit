#pragma once

#include <sasd/ui/resource_key.hpp>
#include <sasd/ui/style.hpp>

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace sasd::ui {

/** One fully owning entry in the first concrete resource family. */
struct TextStyleResource final {
    ResourceKey key;
    TextStyle value;

    friend bool operator==(const TextStyleResource&, const TextStyleResource&) = default;
};

/**
 * Small, explicit owner of backend-neutral application resources.
 *
 * The dictionary is an ordinary copyable and movable value. It owns every key and TextStyle and
 * shares no hidden state with its callers or with another copied dictionary. There is intentionally
 * no process-wide registry: a later Application/UI-description owner can place dictionary lifetime
 * exactly where its resource-resolution policy requires it.
 *
 * M8.24 stores TextStyle directly instead of introducing std::variant, std::any or type erasure
 * before another resource family exists. Future families can add their own typed buckets. Keys are
 * nevertheless reserved as globally unique within one dictionary across all such families, so a
 * future reference need not carry an additional runtime payload-kind discriminator.
 */
class ResourceDictionary final {
public:
    /**
     * Adds an owned copy of key and value.
     *
     * An exact duplicate key fails closed: false is returned and the original value is unchanged.
     * A successful add may invalidate every pointer/reference/span previously obtained from this
     * dictionary. This explicit boundary avoids promising vector element stability that the owning
     * insertion-ordered representation cannot provide.
     */
    [[nodiscard]] bool addTextStyle(ResourceKey key, TextStyle value);

    /**
     * Finds an exact, case-sensitive key, or returns nullptr for an unknown/invalid spelling.
     *
     * A returned pointer is non-owning and remains valid only until the next mutating operation on
     * this dictionary. It is intended for immediate lookup use, not long-lived observation.
     */
    [[nodiscard]] const TextStyle* findTextStyle(std::string_view key) const noexcept;

    /** Same lookup and lifetime contract as the textual overload. */
    [[nodiscard]] const TextStyle* findTextStyle(const ResourceKey& key) const noexcept;

    /**
     * Enumerates entries in exact insertion order.
     *
     * The span borrows this dictionary and is invalidated by the next mutating operation. No
     * sorting is performed: deterministic authoring order and canonical persisted order are
     * intentionally separate future decisions.
     */
    [[nodiscard]] std::span<const TextStyleResource> textStyles() const noexcept {
        return text_styles_;
    }

    [[nodiscard]] std::size_t textStyleCount() const noexcept { return text_styles_.size(); }

private:
    /**
     * Centralizes dictionary-wide key uniqueness. Each future typed family must participate here;
     * this is deliberately a concrete scan rather than a speculative generic payload registry.
     */
    [[nodiscard]] bool containsKey(std::string_view key) const noexcept;

    // A vector keeps small resource sets simple, fully owning and deterministically ordered. A
    // private lookup index can be added later without changing the public API or its semantics.
    std::vector<TextStyleResource> text_styles_;
};

} // namespace sasd::ui
