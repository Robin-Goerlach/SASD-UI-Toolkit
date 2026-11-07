#pragma once

#include <sasd/ui/resource_dictionary.hpp>
#include <sasd/ui/resource_reference.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <span>

namespace sasd::ui {

/**
 * Non-owning access to one caller-selected resource dictionary.
 *
 * A reference_wrapper makes the borrowing contract visible and prevents a null scope without
 * introducing shared ownership. The caller owns every dictionary and must keep it alive for the
 * duration of resolveTextStyleResource(). The scope sequence itself is caller policy: index 0 is
 * the nearest/highest-priority scope, followed by progressively more distant scopes.
 */
using ResourceScope = std::reference_wrapper<const ResourceDictionary>;

/**
 * Owned result of resolving one TextStyle reference.
 *
 * The TextStyle is copied out of the winning dictionary. Consequently the result neither exposes
 * ResourceDictionary's mutation-sensitive lookup pointer nor observes later dictionary changes.
 * scope_index is provenance for this lookup only: it is the winning index in the caller-supplied
 * sequence, not a persistent scope identifier.
 */
struct TextStyleResourceResolution final {
    TextStyle value;
    std::size_t scope_index{};

    friend bool operator==(const TextStyleResourceResolution&,
                           const TextStyleResourceResolution&) = default;
};

/**
 * Resolves a TextStyle reference against explicit nearest-first dictionary scopes.
 *
 * The first exact, case-sensitive key match wins and shadows the complete resource in every later
 * scope. Missing resources, an empty scope sequence, and a reference of any kind other than
 * ResourceKind::text_style fail closed with std::nullopt. No fallback key, normalization, cascade
 * or field merge is performed.
 *
 * This function is intentionally a stateless lookup transaction. It traverses no Component tree,
 * consults no Application/global registry, stores no binding and observes no future mutation. The
 * caller supplies both lifetime and precedence for this lookup. Neither the scope index nor the
 * result establishes a persistence or inline-versus-resource precedence contract. Reconstruction
 * may use the owned result for one-time materialization, but doing so does not turn this stateless
 * lookup into a live binding or extend any supplied dictionary's lifetime.
 */
[[nodiscard]] std::optional<TextStyleResourceResolution>
resolveTextStyleResource(const ResourceReference& reference,
                         std::span<const ResourceScope> scopes) noexcept;

} // namespace sasd::ui
