#include "test_framework.hpp"

#include <sasd/ui/resource_key.hpp>

#include <array>
#include <string>
#include <string_view>

using namespace sasd::ui;

TEST_CASE("ResourceKey accepts only its stable non-empty ASCII machine grammar") {
    constexpr std::array valid{
        std::string_view{"a"},
        std::string_view{"A"},
        std::string_view{"theme.default"},
        std::string_view{"theme.heading"},
        std::string_view{"button-primary"},
        std::string_view{"dialog_error"},
        std::string_view{"forms/login/title"},
    };
    for (const auto value : valid) {
        const auto key = ResourceKey::create(value);
        CHECK(key.has_value());
        CHECK(key->value() == value);
    }

    const std::string control{static_cast<char>(0x1f)};
    const std::string non_ascii{static_cast<char>(0xc3), static_cast<char>(0xa4)};
    const std::array invalid{
        std::string_view{""},
        std::string_view{" "},
        std::string_view{"theme heading"},
        std::string_view{"\t"},
        std::string_view{"\n"},
        std::string_view{"foo\\bar"},
        std::string_view{"theme:heading"},
        std::string_view{"theme@heading"},
        std::string_view{control},
        std::string_view{non_ascii},
    };
    for (const auto value : invalid) {
        CHECK(!ResourceKey::create(value).has_value());
    }
}

TEST_CASE("ResourceKey owns caller text and compares exact case-sensitive identity") {
    std::string source{"theme.heading"};
    const auto key = ResourceKey::create(source);
    CHECK(key.has_value());

    source.clear();
    CHECK(key->value() == "theme.heading");

    const ResourceKey copy = *key;
    const auto different_case = ResourceKey::create("Theme.Heading");
    CHECK(different_case.has_value());
    CHECK(copy == *key);
    CHECK(copy != *different_case);
}
