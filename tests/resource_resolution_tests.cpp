#include "test_framework.hpp"

#include <sasd/ui/resource_resolution.hpp>

#include <array>
#include <span>
#include <string_view>
#include <utility>

using namespace sasd::ui;

namespace {

ResourceKey key(std::string_view value) {
    auto result = ResourceKey::create(value);
    CHECK(result.has_value());
    return *result;
}

ResourceReference textStyleReference(std::string_view value) {
    auto result = ResourceReference::create(ResourceKind::text_style, key(value));
    CHECK(result.has_value());
    return *result;
}

} // namespace

TEST_CASE("Resource resolution fails closed for an empty explicit scope chain") {
    const auto reference = textStyleReference("theme.heading");
    CHECK(!resolveTextStyleResource(reference, std::span<const ResourceScope>{}));
}

TEST_CASE("Resource resolution returns a copied complete TextStyle and its single scope index") {
    ResourceDictionary resources;
    const TextStyle heading{Color::bright_cyan, true, true, true, true};
    CHECK(resources.addTextStyle(key("theme.heading"), heading));

    const std::array<ResourceScope, 1> scopes{std::cref(resources)};
    const auto resolution = resolveTextStyleResource(
        textStyleReference("theme.heading"), scopes);

    CHECK(resolution.has_value());
    CHECK(resolution->value == heading);
    CHECK(resolution->scope_index == 0);
}

TEST_CASE("Resource resolution uses exact keys and returns no implicit default") {
    ResourceDictionary resources;
    CHECK(resources.addTextStyle(key("theme.body"), {Color::white}));
    CHECK(resources.addTextStyle(key("theme.heading"), {Color::bright_cyan, true}));

    const std::array<ResourceScope, 1> scopes{std::cref(resources)};
    CHECK(!resolveTextStyleResource(textStyleReference("theme.missing"), scopes));
    CHECK(!resolveTextStyleResource(textStyleReference("Theme.Heading"), scopes));
}

TEST_CASE("Resource resolution stops at the nearest matching scope") {
    ResourceDictionary local;
    ResourceDictionary application;
    const TextStyle local_heading{Color::bright_red, true};
    const TextStyle application_heading{Color::blue};
    CHECK(local.addTextStyle(key("theme.heading"), local_heading));
    CHECK(application.addTextStyle(key("theme.heading"), application_heading));

    const std::array<ResourceScope, 2> scopes{
        std::cref(local), std::cref(application)};
    const auto resolution = resolveTextStyleResource(
        textStyleReference("theme.heading"), scopes);

    CHECK(resolution == TextStyleResourceResolution{local_heading, 0});
}

TEST_CASE("Resource resolution searches outward only until the first exact match") {
    ResourceDictionary local;
    ResourceDictionary window;
    ResourceDictionary application;
    const TextStyle window_heading{Color::bright_blue, false, true};
    const TextStyle application_heading{Color::blue};
    CHECK(local.addTextStyle(key("theme.body"), {Color::white}));
    CHECK(window.addTextStyle(key("theme.heading"), window_heading));
    CHECK(application.addTextStyle(key("theme.heading"), application_heading));

    const std::array<ResourceScope, 3> scopes{
        std::cref(local), std::cref(window), std::cref(application)};
    const auto resolution = resolveTextStyleResource(
        textStyleReference("theme.heading"), scopes);

    CHECK(resolution == TextStyleResourceResolution{window_heading, 1});
}

TEST_CASE("Resource scope order is caller policy") {
    ResourceDictionary first;
    ResourceDictionary second;
    const TextStyle first_heading{Color::red};
    const TextStyle second_heading{Color::blue};
    CHECK(first.addTextStyle(key("theme.heading"), first_heading));
    CHECK(second.addTextStyle(key("theme.heading"), second_heading));

    const std::array<ResourceScope, 2> first_then_second{
        std::cref(first), std::cref(second)};
    const std::array<ResourceScope, 2> second_then_first{
        std::cref(second), std::cref(first)};

    CHECK(resolveTextStyleResource(textStyleReference("theme.heading"), first_then_second) ==
          TextStyleResourceResolution{first_heading, 0});
    CHECK(resolveTextStyleResource(textStyleReference("theme.heading"), second_then_first) ==
          TextStyleResourceResolution{second_heading, 0});
}

TEST_CASE("Resource resolution remains owned after the winning dictionary is replaced") {
    ResourceDictionary resources;
    const TextStyle original{Color::bright_cyan, true, true, true, true};
    const TextStyle replacement{Color::bright_red, false, false, false, false};
    CHECK(resources.addTextStyle(key("theme.heading"), original));

    const std::array<ResourceScope, 1> scopes{std::cref(resources)};
    const auto resolution = resolveTextStyleResource(
        textStyleReference("theme.heading"), scopes);
    CHECK(resolution == TextStyleResourceResolution{original, 0});

    ResourceDictionary changed;
    CHECK(changed.addTextStyle(key("theme.heading"), replacement));
    resources = std::move(changed);

    CHECK(resolution == TextStyleResourceResolution{original, 0});
    CHECK(resolveTextStyleResource(textStyleReference("theme.heading"), scopes) ==
          TextStyleResourceResolution{replacement, 0});
}
