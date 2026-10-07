#include "test_framework.hpp"

#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/text_field.hpp>

#include <cstdint>
#include <string>
#include <string_view>

using namespace sasd::ui;

namespace {

class SelectAllMeasurementContext final : public MeasurementContext {
public:
    Size measureText(std::string_view) const override { return {1, 1}; }
    Size measureTextField(std::string_view) const override { return {12, 1}; }
    std::uint64_t revision() const noexcept override { return 0; }
};

} // namespace

TEST_CASE("TextField selectAll selects complete Unicode scalar content") {
    TextField field{std::string{"A\xCE\xA9\xE7\x95\x8C" "B"}}; // A + Omega + CJK + B

    field.setCursorPosition(2);
    field.selectAll();

    /*
     * selectAll() is intentionally scalar-based, not byte-based. The selected string contains ASCII,
     * a two-byte Greek scalar and a three-byte CJK scalar, while the active cursor lands at scalar four
     * rather than at the UTF-8 byte length. This is the same coordinate domain used by keyboard,
     * pointer and clipboard selection code.
     */
    CHECK(field.hasSelection());
    CHECK(field.selectionAnchor() == 0);
    CHECK(field.cursorPosition() == 4);
    CHECK(field.selectionStart() == 0);
    CHECK(field.selectionEnd() == 4);
    CHECK(field.selectedText() == std::string{"A\xCE\xA9\xE7\x95\x8C" "B"});
}

TEST_CASE("TextField selectAll is presentation-only and stable for empty fields") {
    SelectAllMeasurementContext context;

    TextField field{"abc"};
    (void)field.measure(context);
    field.acknowledgeVisualUpdate();

    field.selectAll();

    /*
     * Complete-content selection changes caret/selection painting but not text bytes or intrinsic text
     * metrics. Keeping this as visual-only state preserves the existing invalidation architecture and
     * prevents a future Select All command from forcing unnecessary layout work.
     */
    CHECK(field.hasSelection());
    CHECK(field.isMeasureValid());
    CHECK(field.isVisualUpdatePending());

    TextField empty;
    (void)empty.measure(context);
    empty.acknowledgeVisualUpdate();

    empty.selectAll();

    /*
     * An empty field already has anchor and cursor at scalar zero. selectAll() therefore remains a
     * strict no-op, which is useful for command handlers that prefer to call the semantic operation
     * directly instead of special-casing empty controls first.
     */
    CHECK(!empty.hasSelection());
    CHECK(empty.selectionAnchor() == 0);
    CHECK(empty.cursorPosition() == 0);
    CHECK(empty.isMeasureValid());
    CHECK(!empty.isVisualUpdatePending());
}
