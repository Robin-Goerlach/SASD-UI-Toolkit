#pragma once

#include <sasd/ui/tree_selection_model.hpp>
#include <sasd/ui/widget.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace sasd::ui {

struct TreeViewRow final {
    TreeNodePath node;
    std::size_t depth{0};
    std::string text;
    bool has_children{false};
    bool expanded{false};
    bool selected{false};
};

/**
 * One virtualized widget over a TreeModel; it never creates a widget per node and never flattens the
 * complete tree. The viewport stores a value anchor path and a count. `visibleRows()` advances with
 * iterative depth-first sibling/parent queries, so the number of queries depends on visible rows and
 * path depth rather than on the number of hidden siblings or descendants.
 */
class TreeView final : public Widget {
public:
    TreeView();
    ~TreeView() override;

    void setModel(TreeModel* model);
    [[nodiscard]] TreeModel* model() const noexcept { return model_.get(); }
    void setSelectionModel(TreeSelectionModel* selection_model);
    [[nodiscard]] TreeSelectionModel* selectionModel() const noexcept {
        return selection_model_.get();
    }

    /** A missing anchor means the first top-level node. Invalid anchors fail closed to that state. */
    void setViewport(std::optional<TreeNodePath> first_node, std::size_t row_count);
    [[nodiscard]] std::optional<TreeNodePath> firstVisibleNode() const { return first_visible_node_; }
    [[nodiscard]] std::size_t visibleRowCount() const noexcept { return visible_row_count_; }
    [[nodiscard]] std::vector<TreeViewRow> visibleRows() const;

    [[nodiscard]] bool isExpanded(const TreeNodePath& node) const noexcept;
    [[nodiscard]] bool expand(const TreeNodePath& node);
    [[nodiscard]] bool collapse(const TreeNodePath& node);
    [[nodiscard]] bool toggleExpanded(const TreeNodePath& node);

protected:
    [[nodiscard]] EventResult onEvent(const Event& event) override;

private:
    void modelChanged(const TreeModelChange& change);
    void selectionChanged(const TreeSelectionChange& change);
    void normalizeViewport() noexcept;
    void keepSelectionVisible();
    [[nodiscard]] bool moveSelection(Key key);
    [[nodiscard]] std::optional<TreeNodePath> firstVisibleCandidate() const;
    [[nodiscard]] bool isVisibleNode(const TreeNodePath& node) const;
    [[nodiscard]] std::optional<TreeNodePath> nextVisible(const TreeNodePath& node) const;
    [[nodiscard]] std::optional<TreeNodePath> previousVisible(const TreeNodePath& node) const;
    [[nodiscard]] bool isDescendantOf(const TreeNodePath& node,
                                      const TreeNodePath& ancestor) const noexcept;

    TreeModel::Reference model_;
    TreeModel::Subscription model_subscription_;
    TreeSelectionModel::Reference selection_model_;
    TreeSelectionModel::Subscription selection_subscription_;
    std::optional<TreeNodePath> first_visible_node_;
    std::size_t visible_row_count_{0};
    std::vector<TreeNodePath> expanded_nodes_;
};

} // namespace sasd::ui
