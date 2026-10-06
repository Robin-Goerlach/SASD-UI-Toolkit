#include <sasd/ui/combo_box.hpp>

#include <sasd/ui/measurement_context.hpp>

#include <algorithm>
#include <stdexcept>
#include <variant>

namespace sasd::ui {

ComboBox::ComboBox() {
    /*
     * ComboBox is a keyboard selection target even before drop-down presentation is added. Making
     * focusability explicit here follows Button/CheckBox/RadioButton/TextField and keeps generic Widget
     * non-focusable by default.
     */
    setFocusable(true);
}

ComboBox::ComboBox(std::vector<std::string> items)
    : ComboBox() {
    /* Construction establishes initial model state without manufacturing a change notification. */
    items_ = std::move(items);
}

std::optional<std::string_view> ComboBox::selectedText() const noexcept {
    if (!selected_index_.has_value()) {
        return std::nullopt;
    }

    return std::string_view{items_[*selected_index_]};
}

void ComboBox::setTextStyle(TextStyle style) {
    if (text_style_ == style) {
        return;
    }

    text_style_ = style;

    /* Current TextStyle fields affect presentation only, not logical text/control metrics. */
    invalidateVisual();
}

std::size_t ComboBox::appendItem(std::string text) {
    const std::size_t index = items_.size();
    items_.push_back(std::move(text));

    /*
     * The widest item defines intrinsic closed-control width in this first contract. Even an item
     * that is not selected may therefore enlarge the ComboBox. Future drop-down presentation also
     * depends on the collection, so visual invalidation is deliberately conservative.
     */
    invalidateMeasure();
    invalidateVisual();
    return index;
}

bool ComboBox::setItems(std::vector<std::string> items) {
    if (items_ == items) {
        return false;
    }

    const bool selection_was_present = selected_index_.has_value();

    /*
     * Replace first, then clear selection. The old numeric index is deliberately not carried into a
     * semantically unrelated collection even when it would happen to remain in range. This mirrors the
     * fail-closed value-identity rules used elsewhere in the toolkit for transient menu state.
     */
    items_ = std::move(items);
    selected_index_.reset();

    invalidateMeasure();
    invalidateVisual();

    if (selection_was_present) {
        /*
         * Copy before invoking application code. The callback may replace itself, release this Widget
         * from its owner, or destroy it. All internal state and invalidation are complete, so nothing
         * below the callback needs to inspect this object again.
         */
        SelectionChangedHandler handler = on_selection_changed_;
        if (handler) {
            handler(std::nullopt);
        }
    }

    return true;
}

bool ComboBox::setSelectedIndex(std::optional<std::size_t> index) {
    if (index.has_value() && *index >= items_.size()) {
        throw std::out_of_range("ComboBox selected index is outside the item collection");
    }

    if (selected_index_ == index) {
        return false;
    }

    selected_index_ = index;

    /*
     * Selection changes only which already-measured item is displayed. Because intrinsic size uses the
     * maximum item extent, switching selection never invalidates measurement and cannot cause layout
     * jitter between a short and a long choice.
     */
    invalidateVisual();

    SelectionChangedHandler handler = on_selection_changed_;
    if (handler) {
        /* No member access after this call; application code may destroy the control. */
        handler(index);
    }

    return true;
}

Size ComboBox::onMeasure(const MeasurementContext& context,
                         const MeasureConstraints&) {
    /*
     * An empty ComboBox still has backend-specific control chrome (for example a terminal marker or
     * rendered drop arrow), so ask the semantic control-specific measurement hook for empty text first.
     * Every item is then measured through the same hook and the component-wise maximum becomes the
     * intrinsic size. This keeps size stable when selection changes and lets each backend own chrome.
     */
    Size result = context.measureComboBox({});
    for (const std::string& item : items_) {
        const Size measured = context.measureComboBox(item);
        result.width = std::max(result.width, measured.width);
        result.height = std::max(result.height, measured.height);
    }
    return result;
}

EventResult ComboBox::onEvent(const Event& event) {
    const auto* key = std::get_if<KeyEvent>(&event);
    if (key == nullptr || key->modifiers != KeyModifier::none) {
        return EventResult::ignored;
    }

    const bool navigation_key =
        key->key == Key::up || key->key == Key::down ||
        key->key == Key::home || key->key == Key::end;
    if (!navigation_key) {
        return EventResult::ignored;
    }

    if (!hasFocus() || !isEnabled() || !isVisible()) {
        return EventResult::ignored;
    }

    /*
     * Desktop backends may report key-up, while terminal input generally cannot. Consume the complete
     * logical navigation gesture but mutate selection only on key-down so both environments agree.
     */
    if (!key->pressed || items_.empty()) {
        return EventResult::handled;
    }

    std::size_t next = 0U;
    const std::size_t last = items_.size() - 1U;

    switch (key->key) {
    case Key::home:
        next = 0U;
        break;
    case Key::end:
        next = last;
        break;
    case Key::down:
        if (!selected_index_.has_value()) {
            next = 0U;
        } else {
            next = std::min(*selected_index_ + 1U, last);
        }
        break;
    case Key::up:
        if (!selected_index_.has_value()) {
            next = last;
        } else {
            next = *selected_index_ == 0U ? 0U : *selected_index_ - 1U;
        }
        break;
    default:
        /* navigation_key proved this switch is exhaustive for the current first contract. */
        return EventResult::ignored;
    }

    /*
     * setSelectedIndex() owns notification/lifetime safety. Do not inspect members after the call:
     * an application callback is allowed to release or destroy this ComboBox synchronously.
     */
    (void)setSelectedIndex(next);
    return EventResult::handled;
}

} // namespace sasd::ui
