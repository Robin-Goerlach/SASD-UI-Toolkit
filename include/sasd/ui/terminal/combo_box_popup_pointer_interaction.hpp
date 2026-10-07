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
    committed,
};

/**
 * Result of one terminal ComboBox popup pointer sample.
 *
 * An engaged result from handle() means the open popup scope consumed the physical event even when
 * action == none. row_index reports the painted row under the pointer when one exists.
 *
 * committed means a stateful Primary press/release transaction accepted the currently previewed row,
 * transferred it to committed ComboBox selection and closed the drop-down. The helper never executes a
 * second application callback of its own; it delegates semantic publication to ComboBox's existing
 * commitPreviewSelection() contract.
 */
struct TerminalComboBoxPopupPointerResult {
    TerminalComboBoxPopupPointerAction action{
        TerminalComboBoxPopupPointerAction::none};
    std::optional<std::size_t> row_index{};
};

/**
 * Translates Terminal ComboBox popup pointer geometry into the existing Core preview transaction.
 *
 * Geometry remains presentation-owned: every event is hit-tested only against an already-built
 * ComboBoxPopupPresentationSnapshot. The adapter does not remeasure text, place the popup, walk Widget
 * parents or inspect ScreenBuffer cells.
 *
 * Two overloads deliberately expose two levels of policy:
 *
 * - stateless handle() provides hover preview and modal consumption only;
 * - stateful handle(..., GestureState&) additionally provides Primary press/release click completion.
 *
 * Keeping the stateless overload is useful for hosts that want hover but own a different click policy.
 * The stateful overload stores only a numeric painted-row identity between events; it retains no
 * ComboBox, Widget, item string, frame or terminal-device pointer.
 *
 * Outside-click dismissal and PointerRouter capture are intentionally still absent. A press outside the
 * popup cancels any armed row but leaves the ComboBox open, and all open-popup pointer events remain
 * consumed so they cannot click through to Widgets underneath.
 */
class TerminalComboBoxPopupPointerInteraction final {
public:
    /**
     * Host-owned identity for one possible popup row click.
     *
     * A Primary press on a valid painted row arms only that row index. Matching release must hit the
     * same row against a freshly validated snapshot before commit is requested. Keyboard takeover,
     * resize, surface-leave or another host scope can call reset() explicitly; handle() also resets
     * automatically when it observes stale/inactive semantic state or a fresh nonmatching press.
     */
    class GestureState final {
    public:
        GestureState() = default;

        void reset() noexcept {
            pressed_row_.reset();
        }

        [[nodiscard]] bool hasPressedRow() const noexcept {
            return pressed_row_.has_value();
        }

        [[nodiscard]] std::optional<std::size_t> pressedRow() const noexcept {
            return pressed_row_;
        }

    private:
        friend class TerminalComboBoxPopupPointerInteraction;

        std::optional<std::size_t> pressed_row_{};
    };

    TerminalComboBoxPopupPointerInteraction() = delete;

    /**
     * Stateless hover-only handling.
     *
     * Motion over a painted row changes preview; press/release/outside motion are consumed without
     * completion semantics. This preserves the ADR 0125 contract for callers that do not supply
     * cross-event gesture state.
     */
    [[nodiscard]] static std::optional<TerminalComboBoxPopupPointerResult>
    handle(ComboBox& combo,
           const ComboBoxPopupPresentationSnapshot& snapshot,
           const PointerEvent& event,
           AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow) {
        return handleImpl(
            combo,
            snapshot,
            event,
            ambiguous_width,
            nullptr);
    }

    /**
     * Stateful Primary click handling.
     *
     * Primary press on a row first previews that row and arms its presentation identity. Pointer motion
     * may continue to update preview without changing the armed identity. A later Primary release commits
     * only when it geometrically hits the exact row that was armed by the press. Any other release retires
     * the gesture without changing committed selection.
     *
     * GestureState is reset *before* commitPreviewSelection() is called. The Core commit may synchronously
     * invoke application callbacks that release/destroy the ComboBox; after that call begins this helper
     * performs no ComboBox member access.
     */
    [[nodiscard]] static std::optional<TerminalComboBoxPopupPointerResult>
    handle(ComboBox& combo,
           const ComboBoxPopupPresentationSnapshot& snapshot,
           const PointerEvent& event,
           AmbiguousWidthMode ambiguous_width,
           GestureState& gesture_state) {
        return handleImpl(
            combo,
            snapshot,
            event,
            ambiguous_width,
            &gesture_state);
    }

private:
    [[nodiscard]] static std::optional<TerminalComboBoxPopupPointerResult>
    handleImpl(ComboBox& combo,
               const ComboBoxPopupPresentationSnapshot& snapshot,
               const PointerEvent& event,
               AmbiguousWidthMode ambiguous_width,
               GestureState* gesture_state) {
        if (!semanticSnapshotMatches(combo, snapshot, ambiguous_width)) {
            /*
             * Presentation/semantic disagreement invalidates any cross-event row identity. Reset before
             * returning so a later reopen/current snapshot cannot accidentally complete an older press.
             */
            if (gesture_state != nullptr) {
                gesture_state->reset();
            }
            return std::nullopt;
        }

        const auto row =
            TerminalComboBoxPopupHitTest::rowIndexAt(snapshot, event.position);

        if (event.action == PointerAction::move) {
            if (!row.has_value()) {
                /*
                 * Motion outside remains modal but intentionally keeps the previous preview. Outside
                 * dismissal belongs to the later dismissal/capture policy, not to passive hover.
                 */
                return TerminalComboBoxPopupPointerResult{
                    TerminalComboBoxPopupPointerAction::none,
                    std::nullopt,
                };
            }

            const bool changed = combo.setPreviewIndex(*row);
            return TerminalComboBoxPopupPointerResult{
                changed
                    ? TerminalComboBoxPopupPointerAction::preview_changed
                    : TerminalComboBoxPopupPointerAction::none,
                row,
            };
        }

        if (event.action == PointerAction::press) {
            if (gesture_state == nullptr) {
                /*
                 * Stateless callers deliberately get no click semantics. The popup still consumes the
                 * physical press so a visible overlay row cannot activate a Widget underneath it.
                 */
                return TerminalComboBoxPopupPointerResult{
                    TerminalComboBoxPopupPointerAction::none,
                    row,
                };
            }

            /*
             * Every physical press begins a new possible click transaction. Reset first so a press on
             * empty/outside geometry or with another button can never inherit an older Primary row.
             */
            gesture_state->reset();

            if (event.button != PointerButton::primary || !row.has_value()) {
                return TerminalComboBoxPopupPointerResult{
                    TerminalComboBoxPopupPointerAction::none,
                    row,
                };
            }

            /*
             * Press gives immediate visual feedback by previewing the row it arms. setPreviewIndex()
             * is presentation-only and does not invoke SelectionChanged application callbacks.
             */
            const bool changed = combo.setPreviewIndex(*row);
            gesture_state->pressed_row_ = *row;

            return TerminalComboBoxPopupPointerResult{
                changed
                    ? TerminalComboBoxPopupPointerAction::preview_changed
                    : TerminalComboBoxPopupPointerAction::none,
                row,
            };
        }

        /*
         * PointerAction currently has only move/press/release. A non-Primary release cannot complete a
         * Primary click and conservatively retires any pending identity.
         */
        if (gesture_state == nullptr) {
            return TerminalComboBoxPopupPointerResult{
                TerminalComboBoxPopupPointerAction::none,
                row,
            };
        }

        if (event.button != PointerButton::primary ||
            !gesture_state->pressed_row_.has_value()) {
            gesture_state->reset();
            return TerminalComboBoxPopupPointerResult{
                TerminalComboBoxPopupPointerAction::none,
                row,
            };
        }

        const std::size_t pressed_row = *gesture_state->pressed_row_;

        /*
         * Retire cross-event identity before any semantic completion. This remains correct whether the
         * release misses, hits another row, or commit callbacks destroy/release the control.
         */
        gesture_state->reset();

        if (!row.has_value() || *row != pressed_row) {
            return TerminalComboBoxPopupPointerResult{
                TerminalComboBoxPopupPointerAction::none,
                row,
            };
        }

        /*
         * Motion may have previewed another row after press. A release that geometrically returns to the
         * originally armed row should commit that clicked row, so realign preview before accepting the
         * transaction. setPreviewIndex() has no application callback; commitPreviewSelection() owns all
         * selection/drop-down notifications and its documented lifetime-safe callback ordering.
         */
        (void)combo.setPreviewIndex(pressed_row);
        const bool committed = combo.commitPreviewSelection();

        /*
         * Do not touch combo after commitPreviewSelection(): application callbacks may have destroyed it.
         * All remaining return data is copied value state.
         */
        return TerminalComboBoxPopupPointerResult{
            committed
                ? TerminalComboBoxPopupPointerAction::committed
                : TerminalComboBoxPopupPointerAction::none,
            pressed_row,
        };
    }

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
         * Revalidate presentation shape as well as semantic identity. A synthetic/stale snapshot with
         * truncated height must not become an input surface merely because copied strings still equal
         * Core. Width may exceed natural measurement because the builder expands to anchor width.
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
