#pragma once

#include <sasd/ui/combo_box.hpp>
#include <sasd/ui/events/event.hpp>
#include <sasd/ui/rendered/combo_box_popup_hit_test.hpp>
#include <sasd/ui/rendered/combo_box_popup_presentation.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sasd::ui::rendered {

enum class RenderedComboBoxPopupPointerAction {
    none,
    preview_changed,
    committed,
    dismissed,
};

/** Result of one pointer sample consumed by an open rendered ComboBox popup. */
struct RenderedComboBoxPopupPointerResult {
    RenderedComboBoxPopupPointerAction action{
        RenderedComboBoxPopupPointerAction::none};
    std::optional<std::size_t> row_index{};
};

/**
 * Applies rendered popup pointer input to the existing Core ComboBox preview transaction.
 *
 * Geometry belongs entirely to the supplied owned presentation snapshot. The helper retains no Widget,
 * metric provider, DisplayList or native-device pointer between events. An engaged result means the
 * modal overlay consumed the event, including samples outside its rows, which prevents click-through.
 */
class RenderedComboBoxPopupPointerInteraction final {
public:
    /** Host-owned identity for one possible Primary row click. */
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
        friend class RenderedComboBoxPopupPointerInteraction;
        std::optional<std::size_t> pressed_row_{};
    };

    RenderedComboBoxPopupPointerInteraction() = delete;

    /**
     * Handles hover preview, Primary outside-press dismissal and stateful row click completion.
     *
     * A press previews and arms one painted row. Motion may update preview without replacing the armed
     * identity. Primary release commits only on the originally armed row. The gesture is retired before
     * ComboBox completion callbacks begin because those callbacks may release or destroy the control.
     */
    [[nodiscard]] static std::optional<RenderedComboBoxPopupPointerResult>
    handle(ComboBox& combo,
           const RenderedComboBoxPopupPresentationSnapshot& snapshot,
           const PointerEvent& event,
           GestureState& gesture_state) {
        if (!semanticSnapshotMatches(combo, snapshot)) {
            gesture_state.reset();
            return std::nullopt;
        }

        const auto row = RenderedComboBoxPopupHitTest::rowIndexAt(
            snapshot, event.position);

        if (event.action == PointerAction::move) {
            if (!row.has_value()) {
                return RenderedComboBoxPopupPointerResult{
                    RenderedComboBoxPopupPointerAction::none,
                    std::nullopt,
                };
            }

            const bool changed = combo.setPreviewIndex(*row);
            return RenderedComboBoxPopupPointerResult{
                changed
                    ? RenderedComboBoxPopupPointerAction::preview_changed
                    : RenderedComboBoxPopupPointerAction::none,
                row,
            };
        }

        if (event.action == PointerAction::press) {
            /* Every fresh press invalidates a possible identity from an older interaction scope. */
            gesture_state.reset();

            if (event.button != PointerButton::primary) {
                return RenderedComboBoxPopupPointerResult{
                    RenderedComboBoxPopupPointerAction::none,
                    row,
                };
            }

            if (!row.has_value()) {
                /*
                 * Dismiss on press and consume it here. Replaying it into the Widget tree would turn a
                 * modal cancellation into activation of a covered control.
                 */
                const bool dismissed = combo.setDropDownOpen(false);
                return RenderedComboBoxPopupPointerResult{
                    dismissed
                        ? RenderedComboBoxPopupPointerAction::dismissed
                        : RenderedComboBoxPopupPointerAction::none,
                    std::nullopt,
                };
            }

            const bool changed = combo.setPreviewIndex(*row);
            gesture_state.pressed_row_ = *row;
            return RenderedComboBoxPopupPointerResult{
                changed
                    ? RenderedComboBoxPopupPointerAction::preview_changed
                    : RenderedComboBoxPopupPointerAction::none,
                row,
            };
        }

        if (event.button != PointerButton::primary ||
            !gesture_state.pressed_row_.has_value()) {
            gesture_state.reset();
            return RenderedComboBoxPopupPointerResult{
                RenderedComboBoxPopupPointerAction::none,
                row,
            };
        }

        const std::size_t pressed_row = *gesture_state.pressed_row_;
        gesture_state.reset();

        if (!row.has_value() || *row != pressed_row) {
            return RenderedComboBoxPopupPointerResult{
                RenderedComboBoxPopupPointerAction::none,
                row,
            };
        }

        /*
         * Motion may have previewed another row. Realign to the clicked identity before Core publishes
         * completion, then never access combo again because the callback may destroy it.
         */
        (void)combo.setPreviewIndex(pressed_row);
        const bool committed = combo.commitPreviewSelection();
        return RenderedComboBoxPopupPointerResult{
            committed
                ? RenderedComboBoxPopupPointerAction::committed
                : RenderedComboBoxPopupPointerAction::none,
            pressed_row,
        };
    }

private:
    [[nodiscard]] static bool semanticSnapshotMatches(
        const ComboBox& combo,
        const RenderedComboBoxPopupPresentationSnapshot& snapshot) noexcept {
        if (!combo.isDropDownOpen() ||
            !combo.hasFocus() ||
            !combo.isEnabled() ||
            !combo.isVisible() ||
            snapshot.items.size() != combo.itemCount() ||
            snapshot.preview_index != combo.previewIndex() ||
            snapshot.row_height <= 0 ||
            snapshot.padding <= 0 ||
            snapshot.border_thickness < 0) {
            return false;
        }

        if (snapshot.items.empty()) {
            if (!snapshot.bounds.isEmpty() || !snapshot.content_bounds.isEmpty()) {
                return false;
            }
        } else {
            if (snapshot.bounds.isEmpty() || snapshot.content_bounds.isEmpty()) {
                return false;
            }

            /*
             * Input must reject geometry that the renderer would reject. Check the exact inset using
             * widened arithmetic rather than reconstructing Rect values with possibly overflowing
             * Coordinate additions/subtractions.
             */
            const auto border = static_cast<std::int64_t>(snapshot.border_thickness);
            const auto expected_content_x =
                static_cast<std::int64_t>(snapshot.bounds.x) + border;
            const auto expected_content_y =
                static_cast<std::int64_t>(snapshot.bounds.y) + border;
            const auto expected_content_width =
                static_cast<std::int64_t>(snapshot.bounds.width) - (2 * border);
            const auto expected_content_height =
                static_cast<std::int64_t>(snapshot.bounds.height) - (2 * border);
            if (static_cast<std::int64_t>(snapshot.content_bounds.x) != expected_content_x ||
                static_cast<std::int64_t>(snapshot.content_bounds.y) != expected_content_y ||
                static_cast<std::int64_t>(snapshot.content_bounds.width) != expected_content_width ||
                static_cast<std::int64_t>(snapshot.content_bounds.height) != expected_content_height) {
                return false;
            }

            const auto first = presentation::fixedPopupRowBounds(
                snapshot.content_bounds,
                snapshot.items.size(),
                0U,
                snapshot.row_height);
            const auto last = presentation::fixedPopupRowBounds(
                snapshot.content_bounds,
                snapshot.items.size(),
                snapshot.items.size() - 1U,
                snapshot.row_height);
            if (!first.has_value() || !last.has_value()) {
                return false;
            }

            const auto double_padding =
                static_cast<std::int64_t>(snapshot.padding) * 2;
            const auto available_text_width =
                static_cast<std::int64_t>(snapshot.content_bounds.width) -
                double_padding;
            const auto available_text_height =
                static_cast<std::int64_t>(snapshot.row_height) -
                double_padding;
            if (available_text_width < 0 || available_text_height < 0) {
                return false;
            }

            for (const auto& item : snapshot.items) {
                if (item.text_size.width < 0 || item.text_size.height < 0 ||
                    static_cast<std::int64_t>(item.text_size.width) >
                        available_text_width ||
                    static_cast<std::int64_t>(item.text_size.height) >
                        available_text_height) {
                    return false;
                }
            }
        }

        for (std::size_t index = 0; index < snapshot.items.size(); ++index) {
            if (std::string_view{snapshot.items[index].text} != combo.itemAt(index)) {
                return false;
            }
        }

        return true;
    }
};

} // namespace sasd::ui::rendered
