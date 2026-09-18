#include "test_framework.hpp"

#include <sasd/ui/text/utf8.hpp>

#include <string>

using namespace sasd::ui;

TEST_CASE("UTF-8 utility decodes valid one to four byte scalars") {
    const std::string text{"A\xCE\xA9\xF0\x9F\x98\x80"};

    const auto a = utf8::decodeOne(text, 0);
    CHECK(a.valid);
    CHECK(a.value == U'A');
    CHECK(a.consumed == 1);

    const auto omega = utf8::decodeOne(text, 1);
    CHECK(omega.valid);
    CHECK(omega.value == U'\u03A9');
    CHECK(omega.consumed == 2);

    const auto emoji = utf8::decodeOne(text, 3);
    CHECK(emoji.valid);
    CHECK(emoji.value == U'\U0001F600');
    CHECK(emoji.consumed == 4);
}

TEST_CASE("UTF-8 utility recovers malformed input one byte at a time") {
    const std::string malformed{"\xC3("};

    const auto first = utf8::decodeOne(malformed, 0);
    CHECK(!first.valid);
    CHECK(first.value == U'\uFFFD');
    CHECK(first.consumed == 1);

    CHECK(utf8::scalarCount(malformed) == 2);
    CHECK(utf8::byteOffsetForScalarIndex(malformed, 1) == 1);
    CHECK(utf8::byteOffsetForScalarIndex(malformed, 99) == malformed.size());
}

TEST_CASE("UTF-8 single-line sanitizer replaces malformed bytes and removes controls") {
    const std::string input{"A\nB\t\xC3("};
    const std::string sanitized = utf8::sanitizeSingleLine(input);

    CHECK(sanitized == std::string{"AB\xEF\xBF\xBD("});
    CHECK(utf8::scalarCount(sanitized) == 4);
}


TEST_CASE("UTF-8 utility appends valid Unicode scalars") {
    std::string encoded;

    utf8::appendScalar(encoded, U'A');
    utf8::appendScalar(encoded, U'\u03A9');
    utf8::appendScalar(encoded, U'\U0001F600');

    CHECK(encoded == std::string{"A\xCE\xA9\xF0\x9F\x98\x80"});
}

TEST_CASE("UTF-8 utility replaces invalid Unicode scalar values") {
    std::string encoded;

    utf8::appendScalar(encoded, static_cast<char32_t>(0xD800));
    utf8::appendScalar(encoded, static_cast<char32_t>(0x110000));

    CHECK(encoded == std::string{"\xEF\xBF\xBD\xEF\xBF\xBD"});
}
