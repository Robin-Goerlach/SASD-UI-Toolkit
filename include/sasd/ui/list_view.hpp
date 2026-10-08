#pragma once

#include <sasd/ui/list_selection_model.hpp>
#include <sasd/ui/widget.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace sasd::ui {

/** One owned row in the backend-neutral visible presentation snapshot. */
struct ListViewRow final {
    std::size_t row{0};
    std::string text;
    bool selected{false};
};

/**
 * A single virtualized list widget with no per-row Widget children.
 *
 * Core owns only the semantic viewport (row offset and visible row count). Presentation backends
 * request `visibleRows()` and receive an owned value snapshot containing only that range. The model
 * is queried at most once per returned row; no full-model walk or row-widget construction is part of
 * this contract. A future backend can attach geometry to the snapshot without moving geometry into
 * ListModel.
 */
class ListView final : public Widget {
public:
    ListView();
    ~ListView() override;

    void setModel(ListModel* model);
    [[nodiscard]] ListModel* model() const noexcept { return model_.get(); }

    void setSelectionModel(ListSelectionModel* selection_model);
    [[nodiscard]] ListSelectionModel* selectionModel() const noexcept {
        return selection_model_.get();
    }

    /** Sets the first row and number of logical rows visible in the viewport. */
    void setViewport(std::size_t first_row, std::size_t row_count);
    [[nodiscard]] std::size_t firstVisibleRow() const noexcept { return first_visible_row_; }
    [[nodiscard]] std::size_t visibleRowCount() const noexcept { return visible_row_count_; }

    /** Copies only the normalized visible range and current selection into a presentation snapshot. */
    [[nodiscard]] std::vector<ListViewRow> visibleRows() const;

protected:
    [[nodiscard]] EventResult onEvent(const Event& event) override;

private:
    void modelChanged(const ListModelChange& change);
    void selectionChanged(const ListSelectionChange& change);
    void normalizeViewport() noexcept;
    void keepSelectionVisible();
    [[nodiscard]] bool moveSelection(Key key);

    ListModel::Reference model_;
    ListModel::Subscription model_subscription_;
    ListSelectionModel::Reference selection_model_;
    ListSelectionModel::Subscription selection_subscription_;
    std::size_t first_visible_row_{0};
    std::size_t visible_row_count_{0};
};

} // namespace sasd::ui
