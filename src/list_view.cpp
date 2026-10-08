#include <sasd/ui/list_view.hpp>

#include <algorithm>
#include <variant>

namespace sasd::ui {

ListView::ListView() {
    setFocusable(true);
}

ListView::~ListView() = default;

void ListView::setModel(ListModel* model) {
    if (model_.get() == model) {
        return;
    }
    model_subscription_.reset();
    model_ = model != nullptr ? model->reference() : ListModel::Reference{};
    if (model != nullptr) {
        model_subscription_ = model->observe([this](const ListModelChange& change) {
            modelChanged(change);
        });
    }
    normalizeViewport();
    invalidateMeasure();
    invalidateVisual();
}

void ListView::setSelectionModel(ListSelectionModel* selection_model) {
    if (selection_model_.get() == selection_model) {
        return;
    }
    selection_subscription_.reset();
    selection_model_ = selection_model != nullptr ? selection_model->reference()
                                                    : ListSelectionModel::Reference{};
    if (selection_model != nullptr) {
        selection_subscription_ = selection_model->observe([this](const ListSelectionChange& change) {
            selectionChanged(change);
        });
    }
    keepSelectionVisible();
    invalidateVisual();
}

void ListView::setViewport(std::size_t first_row, std::size_t row_count) {
    first_visible_row_ = first_row;
    visible_row_count_ = row_count;
    normalizeViewport();
    invalidateVisual();
}

void ListView::normalizeViewport() noexcept {
    const auto* model = model_.get();
    if (model == nullptr) {
        first_visible_row_ = 0;
        return;
    }
    const auto count = model->rowCount();
    if (first_visible_row_ > count) {
        first_visible_row_ = count;
    }
}

std::vector<ListViewRow> ListView::visibleRows() const {
    std::vector<ListViewRow> result;
    const auto* model = model_.get();
    if (model == nullptr || visible_row_count_ == 0) {
        return result;
    }

    const auto count = model->rowCount();
    const auto start = std::min(first_visible_row_, count);
    const auto available = count - start;
    const auto rows_to_copy = std::min(visible_row_count_, available);
    result.reserve(rows_to_copy);
    const auto selected = selection_model_.get();
    const auto selected_row = selected != nullptr ? selected->selectedRow() : std::nullopt;
    for (std::size_t offset = 0; offset < rows_to_copy; ++offset) {
        const auto row = start + offset;
        // Copy text before constructing the value snapshot. The model owns the source bytes and may
        // mutate immediately after this function returns; the DisplayList must not retain a view.
        result.push_back({row, std::string{model->textAt(row)}, selected_row == row});
    }
    return result;
}

bool ListView::moveSelection(Key key) {
    auto* selection = selection_model_.get();
    if (selection == nullptr) {
        return false;
    }
    bool changed = false;
    switch (key) {
    case Key::up:
        changed = selection->selectPrevious();
        break;
    case Key::down:
        changed = selection->selectNext();
        break;
    case Key::home:
        changed = selection->selectFirst();
        break;
    case Key::end:
        changed = selection->selectLast();
        break;
    default:
        return false;
    }
    if (changed) {
        keepSelectionVisible();
    }
    return changed;
}

EventResult ListView::onEvent(const Event& event) {
    const auto* key_event = std::get_if<KeyEvent>(&event);
    if (key_event == nullptr || !key_event->pressed || key_event->modifiers != KeyModifier::none ||
        !hasFocus() || !isVisible() || !isEnabled()) {
        return EventResult::ignored;
    }
    return moveSelection(key_event->key) ? EventResult::handled : EventResult::ignored;
}

void ListView::keepSelectionVisible() {
    const auto* selection = selection_model_.get();
    const auto selected = selection != nullptr ? selection->selectedRow() : std::nullopt;
    if (!selected.has_value() || visible_row_count_ == 0) {
        return;
    }
    if (*selected < first_visible_row_) {
        first_visible_row_ = *selected;
    } else if (*selected - first_visible_row_ >= visible_row_count_) {
        first_visible_row_ = *selected - visible_row_count_ + 1;
    }
    normalizeViewport();
}

void ListView::modelChanged(const ListModelChange&) {
    normalizeViewport();
    keepSelectionVisible();
    invalidateMeasure();
    invalidateVisual();
}

void ListView::selectionChanged(const ListSelectionChange&) {
    keepSelectionVisible();
    invalidateVisual();
}

} // namespace sasd::ui
