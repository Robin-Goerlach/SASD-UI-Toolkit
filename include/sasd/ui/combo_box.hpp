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
 * The first M4 ComboBox slice deliberately establishes only semantic item/selection state plus
 * keyboard navigation. The control owns a small UTF-8 item vector and an optional selected index;
 * popup placement, open/closed drop-down interaction, pointer item hit testing, native peers and
 * model/view-backed large data sets remain later slices. Keeping those concerns out of this class
 * prevents the initial public API from freezing backend geometry or prematurely introducing the M5
 * ListModel abstraction solely to satisfy one form control.
 *
 * Selection is index-based and optional. Replacing the complete item collection clears selection
 * rather than reinterpreting an old numeric index against unrelated content. Appending one item is
 * intentionally weaker: existing indices remain valid, so the current selection is preserved.
 *
 * While focused, visible and enabled, unmodified Up/Down/Home/End key events navigate the current
 * collection immediately. Navigation is non-wrapping. If no item is selected, Down/Home choose the
 * first item and Up/End choose the last item. Pointer/drop-down interaction is intentionally not
 * assigned a provisional behavior in this foundation slice; later presentation-specific popup work
 * can build on the same selection contract without undoing a temporary click-to-cycle policy.
 */
class ComboBox final : public Widget {
public:
    using SelectionChangedHandler =
        std::function<void(std::optional<std::size_t>)>;

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

protected:
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;

    /**
     * Handles unmodified Up/Down/Home/End while this ComboBox owns logical focus.
     *
     * Key-down performs selection immediately because terminal input cannot promise paired key-up.
     * Matching key-up events are consumed without a second change on desktop backends that do report
     * them. Empty collections still consume these four navigation keys while focused: they belong to
     * the focused selector even though no legal target currently exists.
     */
    [[nodiscard]] EventResult onEvent(const Event& event) override;

private:
    std::vector<std::string> items_;
    std::optional<std::size_t> selected_index_;
    TextStyle text_style_{};
    SelectionChangedHandler on_selection_changed_;
};

} // namespace sasd::ui
