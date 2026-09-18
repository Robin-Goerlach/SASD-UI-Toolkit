#include "test_framework.hpp"

#include <sasd/ui/label.hpp>
#include <sasd/ui/terminal/terminal_measurement_context.hpp>

#include <string>

using namespace sasd::ui;
using namespace sasd::ui::terminal;

TEST_CASE("TerminalMeasurementContext measures Label in terminal cells") {
    TerminalMeasurementContext context;
    Label label{std::string{"A\xE7\x95\x8C" "B"}}; // A + wide CJK + B

    CHECK(label.measure(context) == Size{4, 1});
}

TEST_CASE("TerminalMeasurementContext measures explicit Label lines") {
    TerminalMeasurementContext context;
    Label label{std::string{"abc\n\xE7\x95\x8C"}};

    CHECK(label.measure(context) == Size{3, 2});
}

TEST_CASE("TerminalMeasurementContext policy changes invalidate cached Label size") {
    TerminalMeasurementContext context{AmbiguousWidthMode::narrow};
    Label label{std::string{"\xC2\xA1" "X"}}; // inverted exclamation + X

    CHECK(label.measure(context) == Size{2, 1});
    const auto narrow_revision = context.revision();

    context.setAmbiguousWidthMode(AmbiguousWidthMode::wide);
    CHECK(context.revision() != narrow_revision);
    CHECK(label.measure(context) == Size{3, 1});
}

TEST_CASE("Setting the same terminal width policy preserves measurement revision") {
    TerminalMeasurementContext context{AmbiguousWidthMode::wide};
    const auto revision = context.revision();

    context.setAmbiguousWidthMode(AmbiguousWidthMode::wide);

    CHECK(context.revision() == revision);
}

TEST_CASE("TerminalMeasurementContext can measure combining text before grapheme rendering exists") {
    TerminalMeasurementContext context;
    Label label{std::string{"e\xCC\x81"}}; // e + COMBINING ACUTE ACCENT

    /*
     * Layout can already know the sequence occupies one terminal column. Presentation remains
     * deferred by TerminalPresentationSink until ScreenBuffer can preserve the grapheme itself.
     */
    CHECK(label.measure(context) == Size{1, 1});
}
