#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace sasd::ui {

/**
 * Stable, owning machine identity for one resource in a ResourceDictionary.
 *
 * A ResourceKey is deliberately not display text. Its compact ASCII grammar avoids locale,
 * Unicode-normalization and case-folding questions in durable technical identity; localized names
 * and help text belong to a future authoring/tooling layer. The accepted characters are ASCII
 * letters, digits, '_', '-', '.' and '/'. Punctuation is opaque: in particular, '.' and '/' do
 * not create paths, namespaces, parents or resource scopes.
 *
 * The value is copied at creation, so value() never observes or borrows the caller's input buffer.
 */
class ResourceKey final {
public:
    /**
     * Creates a key when value is non-empty and every byte belongs to the stable ASCII grammar.
     * Ordinary validation failure returns std::nullopt; allocation failure may propagate.
     */
    [[nodiscard]] static std::optional<ResourceKey> create(std::string_view value);

    /** Returns a view into this ResourceKey's owned string. */
    [[nodiscard]] std::string_view value() const noexcept { return value_; }

    /** Resource identity is exact, case-sensitive and locale-independent. */
    friend bool operator==(const ResourceKey&, const ResourceKey&) = default;

private:
    explicit ResourceKey(std::string value) : value_(std::move(value)) {}

    std::string value_;
};

} // namespace sasd::ui
