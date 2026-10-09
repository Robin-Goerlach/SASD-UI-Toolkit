#pragma once

#include <sasd/ui/table_model.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace sasd::ui {

/** A semantic two-dimensional TableModel cell identity. */
struct TableCell final {
    std::size_t row{0};
    std::size_t column{0};

    friend constexpr bool operator==(const TableCell&, const TableCell&) = default;
};

struct TableSelectionChange final {
    std::optional<TableCell> previous;
    std::optional<TableCell> current;
};

/**
 * Single-cell selection independent from keyboard focus.
 *
 * This contract is deliberately table-specific: a selection has both row and column identity, and
 * positional row mutation is normalized only according to the TableModel change kinds that the
 * current model actually publishes. The model is observed but never owned. Once the model expires,
 * selectedCell() fails closed rather than dereferencing stale storage.
 */
class TableSelectionModel final {
private:
    struct ObserverSlot;
    struct ObserverState;

public:
    using ChangedHandler = std::function<void(const TableSelectionChange&)>;

    class Reference final {
    public:
        Reference() noexcept = default;
        [[nodiscard]] TableSelectionModel* get() const noexcept;
        [[nodiscard]] explicit operator bool() const noexcept { return get() != nullptr; }

    private:
        friend class TableSelectionModel;
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
        friend class TableSelectionModel;
        explicit Subscription(std::weak_ptr<ObserverSlot> slot) noexcept : slot_{std::move(slot)} {}
        std::weak_ptr<ObserverSlot> slot_;
    };

    TableSelectionModel() = default;
    ~TableSelectionModel();

    [[nodiscard]] TableModel* model() const noexcept { return model_.get(); }
    [[nodiscard]] std::uint64_t modelRevision() const noexcept {
        const auto* current_model = model_.get();
        return current_model != nullptr ? current_model->revision() : 0;
    }
    [[nodiscard]] Reference reference();
    [[nodiscard]] Subscription observe(ChangedHandler handler);

    /** Rebinds the non-owning model observation and clears the previous selection. */
    void setModel(TableModel* model);

    /** Returns no selection when the model is gone or the identity is no longer valid. */
    [[nodiscard]] std::optional<TableCell> selectedCell() const noexcept;

    [[nodiscard]] bool clear();
    [[nodiscard]] bool select(std::size_t row, std::size_t column);
    [[nodiscard]] bool selectUp();
    [[nodiscard]] bool selectDown();
    [[nodiscard]] bool selectLeft();
    [[nodiscard]] bool selectRight();

private:
    struct ObserverSlot {
        ChangedHandler handler;
        bool active{true};
    };

    struct ObserverState {
        TableSelectionModel* selection{nullptr};
        std::vector<std::shared_ptr<ObserverSlot>> slots;
    };

    void modelChanged(const TableModelChange& change);
    bool setSelected(std::optional<TableCell> cell);
    void notifySelection(std::optional<TableCell> previous);

    [[nodiscard]] std::shared_ptr<ObserverState> ensureObserverState();
    static void compactInactiveObservers(const std::shared_ptr<ObserverState>& state);
    static void notifyObservers(std::shared_ptr<ObserverState> state,
                                const TableSelectionChange& change);

    TableModel::Reference model_;
    TableModel::Subscription model_subscription_;
    std::optional<TableCell> selected_cell_;
    std::shared_ptr<ObserverState> observers_;
};

} // namespace sasd::ui
