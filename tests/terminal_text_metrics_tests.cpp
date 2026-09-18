#include "test_framework.hpp"

#include <sasd/ui/terminal/text_metrics.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("Terminal TextMetrics exposes its pinned Unicode table version") {
    CHECK(TextMetrics::unicodeTableVersion() == "17.0.0");
}

TEST_CASE("Terminal TextMetrics decodes valid and malformed UTF-8 deterministically") {
    const std::string text{"A\xCE\xA9\xF0\x9F\x98\x80"};

    const auto ascii = TextMetrics::decodeOne(text, 0);
    CHECK(ascii.valid);
    CHECK(ascii.value == U'A');
    CHECK(ascii.consumed == 1);

    const auto omega = TextMetrics::decodeOne(text, 1);
    CHECK(omega.valid);
    CHECK(omega.value == U'\u03A9');
    CHECK(omega.consumed == 2);

    const auto emoji = TextMetrics::decodeOne(text, 3);
    CHECK(emoji.valid);
    CHECK(emoji.value == U'\U0001F600');
    CHECK(emoji.consumed == 4);

    const std::string malformed{"\xC3("};
    const auto replacement = TextMetrics::decodeOne(malformed, 0);
    CHECK(!replacement.valid);
    CHECK(replacement.value == U'\uFFFD');
    CHECK(replacement.consumed == 1);

    const auto end = TextMetrics::decodeOne(text, text.size());
    CHECK(!end.valid);
    CHECK(end.consumed == 0);
}

TEST_CASE("Terminal TextMetrics classifies narrow wide zero-width and control code points") {
    CHECK(TextMetrics::codePointWidth(U'A') == 1);
    CHECK(TextMetrics::codePointWidth(U'\u754C') == 2); // CJK ideograph

    CHECK(TextMetrics::codePointWidth(U'\u0301') == 0); // combining acute accent
    CHECK(TextMetrics::codePointWidth(U'\t') == -1);

    CHECK(TextMetrics::codePointWidth(U'\u00A1', AmbiguousWidthMode::narrow) == 1);
    CHECK(TextMetrics::codePointWidth(U'\u00A1', AmbiguousWidthMode::wide) == 2);
}

TEST_CASE("Terminal TextMetrics measures line width in cells rather than UTF-8 bytes") {
    // A (1) + CJK U+754C (2) + B (1) = four terminal columns.
    const std::string text{"A\xE7\x95\x8C" "B"};
    const auto measured = TextMetrics::measureUtf8(text);

    CHECK(measured.columns == 4);
    CHECK(measured.rows == 1);
    CHECK(!measured.had_invalid_utf8);
    CHECK(measured.simpleCellRenderable());
}

TEST_CASE("Terminal TextMetrics measures multiline text by the widest row") {
    const std::string text{"abc\n\xE7\x95\x8C"};
    const auto measured = TextMetrics::measureUtf8(text);

    CHECK(measured.columns == 3);
    CHECK(measured.rows == 2);
}

TEST_CASE("Terminal TextMetrics reports zero-width grapheme requirements") {
    // e + COMBINING ACUTE ACCENT has display width one but needs grapheme-aware cell storage.
    const std::string text{"e\xCC\x81"};
    const auto measured = TextMetrics::measureUtf8(text);

    CHECK(measured.columns == 1);
    CHECK(measured.contains_zero_width);
    CHECK(!measured.simpleCellRenderable());
}

TEST_CASE("Terminal TextMetrics reports terminal controls separately from line endings") {
    const auto tabbed = TextMetrics::measureUtf8("a\tb");
    CHECK(tabbed.contains_nonprinting_control);
    CHECK(!tabbed.simpleCellRenderable());

    const auto lines = TextMetrics::measureUtf8("a\r\nb");
    CHECK(!lines.contains_nonprinting_control);
    CHECK(lines.rows == 2);
    CHECK(lines.columns == 1);
}

TEST_CASE("Terminal TextMetrics counts malformed UTF-8 as visible replacement width") {
    const std::string malformed{"A\xC3("};
    const auto measured = TextMetrics::measureUtf8(malformed);

    CHECK(measured.had_invalid_utf8);
    CHECK(measured.columns == 3); // A + U+FFFD + '('
    CHECK(measured.simpleCellRenderable());
}
