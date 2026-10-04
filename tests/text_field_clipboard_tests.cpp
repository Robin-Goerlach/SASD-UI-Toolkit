#include "test_framework.hpp"

#include <sasd/ui/clipboard.hpp>
#include <sasd/ui/measurement_context.hpp>
#include <sasd/ui/testing/memory_clipboard.hpp>
#include <sasd/ui/text_field.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace sasd::ui;
using sasd::ui::testing::MemoryClipboard;

namespace {

class ClipboardMeasurementContext final : public MeasurementContext {
public:
    Size measureText(std::string_view) const override { return {1, 1}; }
    Size measureTextField(std::string_view) const override { return {11, 1}; }
    std::uint64_t revision() const noexcept override { return 0; }
};

class ThrowingWriteClipboard final : public Clipboard {
public:
    [[nodiscard]] std::optional<std::string> readText() const override {
        return std::nullopt;
    }

    void writeText(std::string) override {
        throw std::runtime_error{"synthetic clipboard write failure"};
    }

    void clear() override {}
};

} // namespace

TEST_CASE("TextField copies the selected Unicode scalar range without mutating editor state") {
    TextField field{std::string{"A\xCE\xA9\xE7\x95\x8C" "B"}}; // A + Omega + CJK + B
    MemoryClipboard clipboard;

    /*
     * Reverse selection is intentional. Copy must consume the normalized semantic range while keeping
     * anchor/cursor direction untouched for later keyboard/pointer selection policy.
     */
    field.setSelection(3, 1);
    const TextField& read_only_field = field;

    CHECK(read_only_field.copySelectionToClipboard(clipboard));
    CHECK(clipboard.readText() ==
          std::optional<std::string>{std::string{"\xCE\xA9\xE7\x95\x8C"}});
    CHECK(field.text() == std::string{"A\xCE\xA9\xE7\x95\x8C" "B"});
    CHECK(field.selectionAnchor() == 3);
    CHECK(field.cursorPosition() == 1);
}

TEST_CASE("TextField copy with collapsed selection preserves existing clipboard contents") {
    TextField field{"abc"};
    MemoryClipboard clipboard;
    clipboard.writeText("keep me");

    /*
     * No selection means there is no Copy payload. Treating that as an empty string would overwrite a
     * clipboard value owned by another command/application interaction, which is an observable side
     * effect despite the editor having nothing selected.
     */
    CHECK(!field.copySelectionToClipboard(clipboard));
    CHECK(clipboard.readText() == std::optional<std::string>{"keep me"});
    CHECK(field.text() == "abc");
    CHECK(!field.hasSelection());
}

TEST_CASE("TextField cuts selected Unicode text only after clipboard write succeeds") {
    TextField field{std::string{"A\xCE\xA9\xE7\x95\x8C" "B"}};
    ClipboardMeasurementContext context;
    MemoryClipboard clipboard;

    (void)field.measure(context);
    field.acknowledgeVisualUpdate();
    field.setSelection(1, 3);
    field.acknowledgeVisualUpdate();

    CHECK(field.isMeasureValid());
    CHECK(!field.isVisualUpdatePending());

    CHECK(field.cutSelectionToClipboard(clipboard));

    CHECK(clipboard.readText() ==
          std::optional<std::string>{std::string{"\xCE\xA9\xE7\x95\x8C"}});
    CHECK(field.text() == "AB");
    CHECK(field.cursorPosition() == 1);
    CHECK(field.selectionAnchor() == 1);
    CHECK(!field.hasSelection());

    /*
     * Cut changes text bytes, unlike Copy or pure selection motion. Both intrinsic measurement and
     * presentation therefore become stale after the successful deletion.
     */
    CHECK(!field.isMeasureValid());
    CHECK(field.isVisualUpdatePending());
}

TEST_CASE("TextField cut with collapsed selection is a no-op for editor and clipboard") {
    TextField field{"abc"};
    MemoryClipboard clipboard;
    clipboard.writeText("existing");
    field.setCursorPosition(1);

    CHECK(!field.cutSelectionToClipboard(clipboard));
    CHECK(clipboard.readText() == std::optional<std::string>{"existing"});
    CHECK(field.text() == "abc");
    CHECK(field.cursorPosition() == 1);
    CHECK(!field.hasSelection());
}

TEST_CASE("TextField copy propagates clipboard write failure without changing selection") {
    TextField field{"abcd"};
    ThrowingWriteClipboard clipboard;
    field.setSelection(1, 3);

    bool threw = false;
    try {
        (void)field.copySelectionToClipboard(clipboard);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    CHECK(threw);
    CHECK(field.text() == "abcd");
    CHECK(field.selectionAnchor() == 1);
    CHECK(field.cursorPosition() == 3);
}

TEST_CASE("TextField cut preserves user text when clipboard write fails") {
    TextField field{"abcd"};
    ThrowingWriteClipboard clipboard;
    field.setSelection(3, 1);

    bool threw = false;
    try {
        (void)field.cutSelectionToClipboard(clipboard);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    /*
     * The transactional ordering is the core regression guarded here: native clipboard failure must
     * happen before TextField deletes anything. Direction is checked as well so failure preserves the
     * complete selection state, not merely the visible text bytes.
     */
    CHECK(threw);
    CHECK(field.text() == "abcd");
    CHECK(field.selectionAnchor() == 3);
    CHECK(field.cursorPosition() == 1);
    CHECK(field.selectionStart() == 1);
    CHECK(field.selectionEnd() == 3);
}
