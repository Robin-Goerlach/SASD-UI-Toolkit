#pragma once

#include <sasd/ui/table_model.hpp>
#include <sasd/ui/table_selection_model.hpp>
#include <sasd/ui/widget.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui {

/** One owned visible row in the backend-neutral TableView snapshot. */
struct TableViewRow final {
    std::size_t row{0};
    std::vector<std::string> cells;
    /** Semantic model column selected in this row, or no visible selection. */
    std::optional<std::size_t> selected_column;
};

/**
 * Virtualized semantic table view.
 *
 * TableView owns no TableModel. It retains only a lifetime-safe reference and asks the model for the
 * configured visible rectangle. The returned headers and cells are owned values: a backend may keep
 * them in a presentation frame after the model changes without retaining string_view or model
 * pointers. No Widget is created per row or cell, which is the scalability boundary established by
 * ADR 0008. Table selection uses a dedicated two-dimensional TableSelectionModel; focus and
 * selection remain independent policies.
 */
class TableView final : public Widget {
public:
    TableView();
    ~TableView() override;

    void setModel(TableModel* model);
    [[nodiscard]] TableModel* model() const noexcept { return model_.get(); }

    /** Observes a non-owning table-specific selection model; focus remains a separate policy. */
    void setSelectionModel(TableSelectionModel* selection_model);
    [[nodiscard]] TableSelectionModel* selectionModel() const noexcept {
        return selection_model_.get();
    }

    /** Sets the visible row and column rectangle in model coordinates. */
    void setViewport(std::size_t first_row,
                     std::size_t row_count,
                     std::size_t first_column,
                     std::size_t column_count);

    [[nodiscard]] std::size_t firstVisibleRow() const noexcept { return first_visible_row_; }
    [[nodiscard]] std::size_t visibleRowCount() const noexcept { return visible_row_count_; }
    [[nodiscard]] std::size_t firstVisibleColumn() const noexcept { return first_visible_column_; }
    [[nodiscard]] std::size_t visibleColumnCount() const noexcept { return visible_column_count_; }

    /** Copies only the configured visible headers and cells into owned values. */
    [[nodiscard]] std::vector<std::string> visibleHeaders() const;
    [[nodiscard]] std::vector<TableViewRow> visibleRows() const;

protected:
    [[nodiscard]] EventResult onEvent(const Event& event) override;

private:
    void modelChanged(const TableModelChange& change);
    void selectionChanged(const TableSelectionChange& change);
    void normalizeViewport() noexcept;
    void keepSelectionVisible();
    [[nodiscard]] bool moveSelection(Key key);

    TableModel::Reference model_;
    TableModel::Subscription model_subscription_;
    TableSelectionModel::Reference selection_model_;
    TableSelectionModel::Subscription selection_subscription_;
    std::size_t first_visible_row_{0};
    std::size_t visible_row_count_{0};
    std::size_t first_visible_column_{0};
    std::size_t visible_column_count_{0};
};

} // namespace sasd::ui
