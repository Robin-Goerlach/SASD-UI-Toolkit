#include "test_framework.hpp"

#include <sasd/ui/resource_dictionary.hpp>

#include <string>

using namespace sasd::ui;

namespace {

ResourceKey key(std::string_view value) {
    auto result = ResourceKey::create(value);
    CHECK(result.has_value());
    return *result;
}

} // namespace

TEST_CASE("ResourceDictionary is empty and unknown invalid or wrong-case lookups fail closed") {
    ResourceDictionary resources;
    CHECK(resources.textStyleCount() == 0);
    CHECK(resources.textStyles().empty());
    CHECK(resources.findTextStyle("theme.heading") == nullptr);
    CHECK(resources.findTextStyle("theme heading") == nullptr);

    const TextStyle heading{Color::bright_cyan, true, false, true, true};
    CHECK(resources.addTextStyle(key("theme.heading"), heading));
    CHECK(resources.findTextStyle("theme.heading") != nullptr);
    CHECK(*resources.findTextStyle("theme.heading") == heading);
    CHECK(resources.findTextStyle("Theme.Heading") == nullptr);
    CHECK(resources.findTextStyle("theme.missing") == nullptr);
}

TEST_CASE("ResourceDictionary duplicate insertion preserves the original owned TextStyle") {
    ResourceDictionary resources;
    TextStyle original{Color::bright_cyan, true, true, true, true};
    const TextStyle replacement{Color::bright_red, false, false, false, false};

    CHECK(resources.addTextStyle(key("theme.heading"), original));
    original.foreground = Color::green;
    original.bold = false;

    CHECK(!resources.addTextStyle(key("theme.heading"), replacement));
    CHECK(resources.textStyleCount() == 1);
    CHECK(*resources.findTextStyle(key("theme.heading")) ==
          TextStyle{Color::bright_cyan, true, true, true, true});
}

TEST_CASE("ResourceDictionary enumerates TextStyle resources in exact insertion order") {
    ResourceDictionary resources;
    CHECK(resources.addTextStyle(key("theme.body"), {}));
    CHECK(resources.addTextStyle(key("theme.heading"), {Color::bright_cyan, true}));
    CHECK(resources.addTextStyle(key("theme.error"), {Color::bright_red, true}));

    const auto entries = resources.textStyles();
    CHECK(entries.size() == 3);
    CHECK(entries[0].key.value() == "theme.body");
    CHECK(entries[1].key.value() == "theme.heading");
    CHECK(entries[2].key.value() == "theme.error");
}

TEST_CASE("ResourceDictionary copies own independent keys values and storage") {
    ResourceDictionary original;
    CHECK(original.addTextStyle(key("theme.body"), {Color::white}));

    ResourceDictionary copy = original;
    CHECK(copy.addTextStyle(key("theme.heading"), {Color::bright_cyan, true}));

    CHECK(original.textStyleCount() == 1);
    CHECK(original.findTextStyle("theme.heading") == nullptr);
    CHECK(copy.textStyleCount() == 2);
    CHECK(copy.findTextStyle("theme.body") != original.findTextStyle("theme.body"));
    CHECK(*copy.findTextStyle("theme.body") == *original.findTextStyle("theme.body"));
}
