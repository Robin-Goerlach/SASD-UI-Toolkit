#pragma once

#include <sasd/ui/style.hpp>
#include <sasd/ui/widget.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sasd::ui {

/**
 * Backend-neutral non-editable single-selection control.
 *
 * The M4 ComboBox contract owns semantic item/selection state plus a small backend-neutral drop-down
 * visibility state. The control deliberately does not own popup geometry, popup rows, pointer capture,
 * native peers or a model/view-backed large data source. Keeping those concerns outside this class
 * prevents backend coordinates from leaking into Core and avoids prematurely introducing the M5
 * ListModel abstraction solely to satisfy one form control.
 *
 * Selection is index-based and optional. Replacing the complete item collection clears selection
 * rather than reinterpreting an old numeric index against unrelated content. Appending one item is
 * intentionally weaker: existing indices remain valid, so the current selection is preserved.
 *
 * While closed, unmodified Up/Down/Home/End continue to navigate committed selection immediately.
 * While open, the same keys move a separate transient preview without notifying committed-selection
 * observers. Enter commits that preview and closes; Escape closes and discards it. F4 toggles the
 * drop-down intent, Alt+Down opens it and Alt+Up closes/cancels it. A focused collapsed ComboBox also
 * owns the same backend-neutral armed Primary-pointer gesture as other simple controls: press arms,
 * captured motion updates pressed feedback, and a matching release inside toggles the drop-down.
 * Entering the open state requires logical focus, and losing focus closes/cancels it synchronously.
 * Popup placement, popup-row hit testing and native peers remain presentation/backend concerns.
 */
class ComboBox final : public Widget {
public:
    using SelectionChangedHandler =
        std::function<void(std::optional<std::size_t>)>;
    using DropDownChangedHandler = std::function<void(bool)>;

    ComboBox();
    explicit ComboBox(std::vector<std::string> items);

    [[nodiscard]] std::size_t itemCount() const noexcept { return items_.size(); }

    /**
     * Returns one owned UTF-8 item by structural index.
     *
     * The returned view remains valid until the item collection is changed. std::out_of_range is
     * propagated for an invalid index so callers cannot silently observe an unrelated fallback item.
     */
    [[nodiscard]] std::string_view itemAt(std::size_t index) const {
        return items_.at(index);
    }

    [[nodiscard]] std::optional<std::size_t> selectedIndex() const noexcept {
        return selected_index_;
    }

    /** Returns the selected item's UTF-8 text, or std::nullopt when no selection exists. */
    [[nodiscard]] std::optional<std::string_view> selectedText() const noexcept;

    /**
     * Returns the transient preview index used only while the drop-down is open.
     *
     * Opening seeds preview from committed selection. Navigation and future pointer-row interaction
     * change preview independently; ordinary SelectionChanged notification occurs only if/when the
     * preview is committed. Closed ComboBoxes maintain the invariant that preview is std::nullopt.
     */
    [[nodiscard]] std::optional<std::size_t> previewIndex() const noexcept {
        return preview_index_;
    }

    /** Returns the preview item's UTF-8 text, or std::nullopt when no preview exists. */
    [[nodiscard]] std::optional<std::string_view> previewText() const noexcept;

    /** Returns whether this ComboBox currently requests an open drop-down presentation. */
    [[nodiscard]] bool isDropDownOpen() const noexcept { return drop_down_open_; }

    /**
     * Returns transient Primary-pointer pressed state for collapsed-control presentation.
     *
     * This is independent from committed/preview selection and drop-down visibility. It is true only
     * while an armed Primary gesture remains geometrically inside a visible/enabled ComboBox. The
     * PointerRouter owns capture; ComboBox owns only the semantic gesture bits.
     */
    [[nodiscard]] bool isPressed() const noexcept {
        return pointer_armed_ && pointer_inside_ && isVisible() && isEnabled();
    }

    /**
     * Changes the backend-neutral drop-down visibility intent.
     *
     * Opening is accepted only while the ComboBox owns logical focus and is visible/enabled. That
     * invariant gives transient popup state a deterministic lifetime: FocusManager delivers a lost-
     * focus notification before another control takes keyboard ownership, and ComboBox closes from
     * that notification. Closing is always allowed, including cleanup after focus has already gone.
     *
     * Opening seeds preview from the current committed selection. Closing without an explicit commit
     * discards preview, so F4-close, Alt+Up, focus loss and direct setDropDownOpen(false) are all
     * cancellation paths. These transitions change no committed selection and no measurement; they
     * invalidate presentation and then invoke the optional callback with fully coherent state. The
     * callback is copied before invocation and no member is touched afterwards, so application code
     * may release/destroy the control from the notification.
     *
     * @returns true when the open/closed state changed; false for an idempotent or rejected open.
     */
    bool setDropDownOpen(bool open);

    /**
     * Replaces the transient preview index while the drop-down is open.
     *
     * A non-empty index must name an existing item or std::out_of_range is thrown before state changes.
     * Closed ComboBoxes reject preview mutation and return false. Preview affects presentation only;
     * it deliberately does not invoke SelectionChanged because application selection remains committed
     * until commitPreviewSelection() succeeds.
     *
     * @returns true when open preview state changed.
     */
    bool setPreviewIndex(std::optional<std::size_t> index);

    /**
     * Commits the current preview as application selection and closes the drop-down atomically.
     *
     * The operation is accepted only while open. State and visual invalidation are completed before
     * application callbacks run. When committed selection really changes, SelectionChanged is emitted;
     * DropDownChanged(false) is emitted for the close. Both callbacks are copied before the transition,
     * and this method performs no member access after callback delivery begins.
     *
     * @returns true when an open preview transaction was committed/closed.
     */
    bool commitPreviewSelection();

    [[nodiscard]] const TextStyle& textStyle() const noexcept { return text_style_; }

    /** Changes presentation-only text/chrome styling without affecting intrinsic measurement. */
    void setTextStyle(TextStyle style);

    /**
     * Appends one item and returns its structural index.
     *
     * Existing selection is preserved because append does not change any earlier index. The item may
     * change intrinsic width/height and future drop-down presentation, so both measurement and visual
     * state are invalidated.
     */
    std::size_t appendItem(std::string text);

    /**
     * Replaces the complete owned item collection.
     *
     * A real replacement clears selection before any callback is invoked. This fail-closed rule avoids
     * reusing the same numeric index for semantically unrelated replacement data. If a selection was
     * cleared, the callback receives std::nullopt after items, selection and invalidation state are all
     * coherent. The handler is copied before invocation and no member is touched afterwards, allowing
     * application code to release/destroy this control from the callback.
     *
     * @returns true when the collection changed.
     */
    bool setItems(std::vector<std::string> items);

    /** Clears the item collection and any current selection. */
    bool clearItems() { return setItems({}); }

    /**
     * Replaces the optional selected index.
     *
     * Programmatic selection remains legal while hidden or disabled; those gates apply only to user
     * input. A non-empty index must name an existing item or std::out_of_range is thrown before state
     * changes. Selection affects presentation only because intrinsic ComboBox measurement is based on
     * the widest item, not whichever item happens to be selected.
     *
     * @returns true when selection changed.
     */
    bool setSelectedIndex(std::optional<std::size_t> index);

    bool clearSelection() { return setSelectedIndex(std::nullopt); }

    void setOnSelectionChanged(SelectionChangedHandler handler) {
        on_selection_changed_ = std::move(handler);
    }

    void setOnDropDownChanged(DropDownChangedHandler handler) {
        on_drop_down_changed_ = std::move(handler);
    }

protected:
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;

    /**
     * Handles focus lifetime, collapsed Primary-pointer activation, drop-down transactions and keyboard
     * selection navigation.
     *
     * A new Primary pointer gesture is accepted only while logical focus already belongs to this
     * ComboBox. Hosts remain responsible for their focus-on-pointer-press policy and normally assign
     * focus before PointerRouter dispatch, as the Terminal and SDL demos do. Matching release inside
     * toggles the drop-down; release outside merely retires the gesture.
     *
     * F4 toggles open state; exact Alt+Down opens and exact Alt+Up cancels/closes. While open,
     * unmodified Up/Down/Home/End navigate preview, Enter commits/ closes, and Escape cancels/closes.
     * While closed, the navigation keys retain the existing immediate committed-selection behavior.
     * Key-down performs mutations because terminal input cannot promise paired key-up; recognized
     * key-up is consumed without a second mutation on desktop backends. Focus loss is cancellation.
     */
    [[nodiscard]] EventResult onEvent(const Event& event) override;
    void onPointerCaptureLost() noexcept override;

private:
    std::vector<std::string> items_;
    std::optional<std::size_t> selected_index_;
    std::optional<std::size_t> preview_index_;
    bool drop_down_open_{false};
    TextStyle text_style_{};

    /*
     * Collapsed-control Primary gesture mechanics. Armed means this ComboBox owns the physical click
     * transaction; inside tracks whether the captured pointer is still within clipped visual bounds.
     * Presentation observes only isPressed(). Popup-row gestures remain separate overlay state.
     */
    bool pointer_armed_{false};
    bool pointer_inside_{false};
    SelectionChangedHandler on_selection_changed_;
    DropDownChangedHandler on_drop_down_changed_;
};

} // namespace sasd::ui
