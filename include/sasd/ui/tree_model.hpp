#pragma once

#include <sasd/ui/component.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sasd::ui {

/**
 * Positional identity of a tree node below the invisible root.
 *
 * The path is deliberately a value, not a pointer into application storage. It identifies the
 * child at each level only for the observed model revision; a structural reset invalidates every
 * old path. This honest positional contract avoids promising stable IDs that a model may not own.
 */
struct TreeNodePath final {
    std::vector<std::size_t> indices;

    [[nodiscard]] bool empty() const noexcept { return indices.empty(); }
    [[nodiscard]] std::size_t depth() const noexcept { return indices.size(); }
    [[nodiscard]] TreeNodePath parent() const;
    [[nodiscard]] TreeNodePath child(std::size_t index) const;

    friend bool operator==(const TreeNodePath&, const TreeNodePath&) = default;
};

struct TreeModelChange final {
    enum class Kind {
        reset,
        node_changed,
    };

    Kind kind{Kind::reset};
    TreeNodePath node;
};

/** Backend-neutral, observed textual tree contract. */
class TreeModel : public Component {
private:
    struct ObserverSlot;
    struct ObserverState;

public:
    using ChangedHandler = std::function<void(const TreeModelChange&)>;

    /** Non-owning reference whose lookup fails closed after model destruction. */
    class Reference final {
    public:
        Reference() noexcept = default;
        [[nodiscard]] TreeModel* get() const noexcept;
        [[nodiscard]] explicit operator bool() const noexcept { return get() != nullptr; }

    private:
        friend class TreeModel;
        explicit Reference(std::weak_ptr<ObserverState> state) noexcept : state_{std::move(state)} {}
        std::weak_ptr<ObserverState> state_;
    };

    /** Move-only observer token; it never keeps the model alive. */
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
        friend class TreeModel;
        explicit Subscription(std::weak_ptr<ObserverSlot> slot) noexcept : slot_{std::move(slot)} {}
        std::weak_ptr<ObserverSlot> slot_;
    };

    TreeModel() = default;
    ~TreeModel() override;
    TreeModel(const TreeModel&) = delete;
    TreeModel& operator=(const TreeModel&) = delete;

    /** Empty path denotes the invisible root and is valid only for childCount(). */
    [[nodiscard]] virtual std::size_t childCount(const TreeNodePath& parent) const noexcept = 0;
    /** The returned text is borrowed for this observation and must be copied by views. */
    [[nodiscard]] virtual std::string_view textAt(const TreeNodePath& node) const = 0;

    [[nodiscard]] bool isValidNode(const TreeNodePath& node) const noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }
    [[nodiscard]] Reference reference();
    [[nodiscard]] Subscription observe(ChangedHandler handler);

protected:
    /** Derived storage must be coherent before this synchronous notification begins. */
    void notifyChanged(TreeModelChange change);

private:
    struct ObserverSlot {
        ChangedHandler handler;
        bool active{true};
    };
    struct ObserverState {
        TreeModel* model{nullptr};
        std::vector<std::shared_ptr<ObserverSlot>> slots;
    };

    [[nodiscard]] std::shared_ptr<ObserverState> ensureObserverState();
    static void compactInactiveObservers(const std::shared_ptr<ObserverState>& state);
    static void notifyObservers(std::shared_ptr<ObserverState> state,
                                const TreeModelChange& change);

    std::shared_ptr<ObserverState> observers_;
    std::uint64_t revision_{0};
};

/** Small owned tree storage used by tests and examples, not by the abstract model contract. */
class StringTreeModel final : public TreeModel {
public:
    StringTreeModel() = default;
    explicit StringTreeModel(std::vector<std::string> roots);

    [[nodiscard]] std::size_t childCount(const TreeNodePath& parent) const noexcept override;
    [[nodiscard]] std::string_view textAt(const TreeNodePath& node) const override;

    /** Appends a node and returns its positional path in the completed structure. */
    TreeNodePath append(const TreeNodePath& parent, std::string text);
    void setText(const TreeNodePath& node, std::string text);
    void reset(std::vector<std::string> roots);

private:
    struct Node {
        std::string text;
        std::vector<Node> children;
    };

    [[nodiscard]] Node* nodeAt(const TreeNodePath& path) noexcept;
    [[nodiscard]] const Node* nodeAt(const TreeNodePath& path) const noexcept;
    [[nodiscard]] std::vector<Node>* childrenAt(const TreeNodePath& parent) noexcept;
    [[nodiscard]] const std::vector<Node>* childrenAt(const TreeNodePath& parent) const noexcept;

    std::vector<Node> roots_;
};

} // namespace sasd::ui
