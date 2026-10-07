#pragma once

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/events/event.hpp>
#include <sasd/ui/terminal/combo_box_popup_hit_test.hpp>
#include <sasd/ui/terminal/combo_box_popup_presentation.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

namespace sasd::ui::terminal {

enum class TerminalComboBoxPopupPointerAction {
    none,
    preview_changed,
};

/**
 * Result of one terminal ComboBox popup pointer sample.
 *
 * An engaged result from handle() means the open popup scope consumed the physical event even when
 * action == none. row_index reports the painted row under the pointer when one exists. This explicit
 * consumption contract is important while click completion/outside dismissal are still separate later
 * slices: visible popup rows must never leak pointer events through to Widgets underneath.
 */
struct TerminalComboBoxPopupPointerResult {
    TerminalComboBoxPopupPointerAction action{
        TerminalComboBoxPopupPointerAction::none};
    std::optional<std::size_t> row_index{};
};

/**
 * Applies the first pointer semantics to an open Terminal ComboBox popup.
 *
 * This slice intentionally owns only passive pointer-motion preview:
 *
 * - motion over a painted row updates ComboBox::previewIndex();
 * - motion outside the rows keeps the previous preview;
 * - press/release are consumed but do not yet commit/cancel;
 * - no PointerRouter capture is created;
 * - no outside-click dismissal is inferred.
 *
 * Separating hover preview from click completion keeps the current contract small and testable. A later
 * press/release slice can add explicit gesture identity without retrofitting it into this stateless helper.
 *
 * Before applying a row index, handle() proves that the supplied owned presentation snapshot still
 * represents the current semantic ComboBox item collection and preview state. This prevents a stale
 * numeric row from acquiring a new meaning after setItems() or another semantic mutation. The helper
 * retains no pointers or frame state after the synchronous call.
 */
class TerminalComboBoxPopupPointerInteraction final {
public:
    TerminalComboBoxPopupPointerInteraction() = delete;

    /**
     * Handles one pointer sample against a currently presented popup snapshot.
     *
     * @returns std::nullopt when the ComboBox/snapshot scope is stale or inactive and therefore cannot
     *          safely claim the event. Otherwise an engaged result means the popup consumed the event.
     */
    [[nodiscard]] static std::optional<TerminalComboBoxPopupPointerResult>
    handle(ComboBox& combo,
           const ComboBoxPopupPresentationSnapshot& snapshot,
           const PointerEvent& event,
           AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        if (!semanticSnapshotMatches(combo, snapshot, ambiguous_width)) {
            return std::nullopt;
        }

        const auto row =
            TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, event.position);

        if (event.action != PointerAction::move || !row.has_value()) {
            /*
             * The popup is modal even though this first pointer slice only changes preview on motion.
             * Consuming press/release and outside motion here is deliberate click-through protection,
             * not a claim that those gestures already have completion semantics.
             */
            return TerminalComboBoxPopupPointerResult{
                TerminalComboBoxPopupPointerAction::none,
                row,
            };
        }

        /*
         * setPreviewIndex() changes presentation only and invokes no application selection callback.
         * The row is already proven against both snapshot and current semantic item count below, so no
         * out_of_range path is expected. Its bool return distinguishes a real hover transition from a
         * repeated all-motion report over the row that is already previewed.
         */
        const bool changed = combo.setPreviewIndex(*row);
        return TerminalComboBoxPopupPointerResult{
            changed
                ? TerminalComboBoxPopupPointerAction::preview_changed
                : TerminalComboBoxPopupPointerAction::none,
            row,
        };
    }

private:
    [[nodiscard]] static bool semanticSnapshotMatches(
        const ComboBox& combo,
        const ComboBoxPopupPresentationSnapshot& snapshot,
        AmbiguousWidthMode ambiguous_width) {
        if (!combo.isDropDownOpen() ||
            !combo.hasFocus() ||
            !combo.isEnabled() ||
            !combo.isVisible() ||
            snapshot.items.size() != combo.itemCount() ||
            snapshot.preview_index != combo.previewIndex()) {
            return false;
        }

        /*
         * Revalidate the presentation shape as well as semantic identity. A synthetic/stale snapshot
         * with truncated height must not become an input surface merely because its copied strings still
         * equal Core. Width may be larger than natural measurement because the builder expands to the
         * collapsed anchor width.
         */
        const auto measured =
            measureComboBoxPopupPresentation(snapshot, ambiguous_width);
        if (!measured.has_value()) {
            return false;
        }

        if (snapshot.items.empty()) {
            if (!snapshot.bounds.isEmpty()) {
                return false;
            }
        } else if (snapshot.bounds.isEmpty() ||
                   snapshot.bounds.width < measured->size.width ||
                   snapshot.bounds.height != measured->size.height) {
            return false;
        }

        for (std::size_t index = 0; index < snapshot.items.size(); ++index) {
            if (std::string_view{snapshot.items[index]} != combo.itemAt(index)) {
                return false;
            }
        }

        return true;
    }
};

} // namespace sasd::ui::terminal
