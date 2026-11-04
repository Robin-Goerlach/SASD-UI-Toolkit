#include "test_framework.hpp"

#include <sasd/ui/resource_reference.hpp>

#include <string>
#include <utility>

using namespace sasd::ui;

namespace {

ResourceKey key(std::string_view value) {
    auto result = ResourceKey::create(value);
    CHECK(result.has_value());
    return *result;
}

} // namespace

TEST_CASE("TextStyle is the only current built-in resource kind") {
    CHECK(isBuiltInResourceKind(ResourceKind::text_style));
    CHECK(!isBuiltInResourceKind(static_cast<ResourceKind>(255)));
}

TEST_CASE("ResourceReference owns its key and behaves as a normal copied and moved value") {
    std::string source = "theme.heading";
    auto reference = ResourceReference::create(ResourceKind::text_style, key(source));
    CHECK(reference.has_value());

    source.assign("changed.after.creation");
    CHECK(reference->kind() == ResourceKind::text_style);
    CHECK(reference->key().value() == "theme.heading");

    const ResourceReference copied = *reference;
    CHECK(copied == *reference);
    CHECK(copied.key().value() == "theme.heading");

    ResourceReference moved = std::move(*reference);
    CHECK(moved == copied);
    CHECK(moved.key().value() == "theme.heading");
}

TEST_CASE("ResourceReference rejects a fabricated resource kind") {
    CHECK(!ResourceReference::create(static_cast<ResourceKind>(255), key("theme.heading")));
}
