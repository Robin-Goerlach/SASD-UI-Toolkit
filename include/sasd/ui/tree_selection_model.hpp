#pragma once

#include <sasd/ui/tree_model.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace sasd::ui {

struct TreeSelectionChange final {
    std::optional<TreeNodePath> previous;
    std::optional<TreeNodePath> current;
};

/**
 * Initial single-node selection contract for TreeView.
 *
 * Selection is separate from keyboard focus and observes, but never owns, a TreeModel. Paths remain
 * valid only for the current model revision; reset and model destruction clear selection. A content
 * change keeps selection because it does not change positional identity.
 */
class TreeSelectionModel final {
private:
    struct ObserverSlot;
    struct ObserverState;

public:
    using ChangedHandler = std::function<void(const TreeSelectionChange&)>;

    class Reference final {
    public:
        Reference() noexcept = default;
        [[nodiscard]] TreeSelectionModel* get() const noexcept;
        [[nodiscard]] explicit operator bool() const noexcept { return get() != nullptr; }

    private:
        friend class TreeSelectionModel;
        explicit Reference(std::weak_ptr<ObserverState> state) noexcept : state_{std::move(state)} {}
        std::weak_ptr<ObserverState> state_;
    };

    class Subscription final {
    public:
        Subscription() noexcept = default;
        ~Subscription() { reset(); }
        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;
        Subscription(Subscription&& other) noexcept : slot_{std::move(other.slot_)} {}
        Subscription& operator=(Subscription&& other) noexcept {
            if (this != &other) {
                reset();
                slot_ = std::move(other.slot_);
            }
            return *this;
        }
        [[nodiscard]] bool connected() const noexcept;
        void reset() noexcept;

    private:
        friend class TreeSelectionModel;
        explicit Subscription(std::weak_ptr<ObserverSlot> slot) noexcept : slot_{std::move(slot)} {}
        std::weak_ptr<ObserverSlot> slot_;
    };

    TreeSelectionModel() = default;
    ~TreeSelectionModel();

    void setModel(TreeModel* model);
    [[nodiscard]] TreeModel* model() const noexcept { return model_.get(); }
    [[nodiscard]] std::optional<TreeNodePath> selectedNode() const noexcept;
    [[nodiscard]] Reference reference();
    [[nodiscard]] Subscription observe(ChangedHandler handler);

    [[nodiscard]] bool clear();
    [[nodiscard]] bool select(const TreeNodePath& node);

private:
    struct ObserverSlot {
        ChangedHandler handler;
        bool active{true};
    };
    struct ObserverState {
        TreeSelectionModel* selection{nullptr};
        std::vector<std::shared_ptr<ObserverSlot>> slots;
    };

    void modelChanged(const TreeModelChange& change);
    bool setSelected(std::optional<TreeNodePath> node);
    void notifySelection(std::optional<TreeNodePath> previous);
    [[nodiscard]] std::shared_ptr<ObserverState> ensureObserverState();
    static void compactInactiveObservers(const std::shared_ptr<ObserverState>& state);
    static void notifyObservers(std::shared_ptr<ObserverState> state,
                                const TreeSelectionChange& change);

    TreeModel::Reference model_;
    TreeModel::Subscription model_subscription_;
    std::optional<TreeNodePath> selected_node_;
    std::shared_ptr<ObserverState> observers_;
};

} // namespace sasd::ui
