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
 * Backend-neutral description of a completed TableModel mutation.
 *
 * Row and column numbers are positional. They are useful for conservative invalidation only; they
 * are not durable identities for application records. The notification is published after the
 * derived model has restored its rectangular invariant, so an observer may immediately rebuild a
 * visible snapshot without observing a half-applied row or column.
 */
struct TableModelChange final {
    enum class Kind {
        reset,
        rows_inserted,
        rows_removed,
        cells_changed,
    };

    Kind kind{Kind::reset};
    std::size_t first_row{0};
    std::size_t row_count{0};
    std::size_t first_column{0};
    std::size_t column_count{0};
};

/**
 * Small backend-neutral contract for a rectangular, textual table.
 *
 * The model owns its source data. `headerAt()` and `textAt()` return borrowed views that are valid
 * only during the current observation of the model; a TableView must copy every value needed by a
 * presentation snapshot before returning control to application code. The view stores only a
 * lifetime-safe non-owning Reference and therefore never keeps the model alive accidentally.
 */
class TableModel : public Component {
private:
    struct ObserverSlot;
    struct ObserverState;

public:
    using ChangedHandler = std::function<void(const TableModelChange&)>;

    /** Lifetime-safe, non-owning observation of a model. */
    class Reference final {
    public:
        Reference() noexcept = default;

        [[nodiscard]] TableModel* get() const noexcept;
        [[nodiscard]] explicit operator bool() const noexcept { return get() != nullptr; }

    private:
        friend class TableModel;
        explicit Reference(std::weak_ptr<ObserverState> state) noexcept
            : state_{std::move(state)} {}

        std::weak_ptr<ObserverState> state_;
    };

    /** Move-only observer token; it never extends the model lifetime. */
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
        friend class TableModel;
        explicit Subscription(std::weak_ptr<ObserverSlot> slot) noexcept : slot_{std::move(slot)} {}

        std::weak_ptr<ObserverSlot> slot_;
    };

    TableModel() = default;
    ~TableModel() override;

    TableModel(const TableModel&) = delete;
    TableModel& operator=(const TableModel&) = delete;

    [[nodiscard]] virtual std::size_t rowCount() const noexcept = 0;
    [[nodiscard]] virtual std::size_t columnCount() const noexcept = 0;
    [[nodiscard]] virtual std::string_view headerAt(std::size_t column) const = 0;
    [[nodiscard]] virtual std::string_view textAt(std::size_t row, std::size_t column) const = 0;

    /** Monotonic revision used by presentation snapshots to reject stale structure. */
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }

    [[nodiscard]] Reference reference();
    [[nodiscard]] Subscription observe(ChangedHandler handler);

protected:
    /** Publishes only after the derived mutation has restored the complete table invariant. */
    void notifyChanged(TableModelChange change);

private:
    struct ObserverSlot {
        ChangedHandler handler;
        bool active{true};
    };

    struct ObserverState {
        TableModel* model{nullptr};
        std::vector<std::shared_ptr<ObserverSlot>> slots;
    };

    [[nodiscard]] std::shared_ptr<ObserverState> ensureObserverState();
    static void compactInactiveObservers(const std::shared_ptr<ObserverState>& state);
    static void notifyObservers(std::shared_ptr<ObserverState> state,
                                const TableModelChange& change);

    std::shared_ptr<ObserverState> observers_;
    std::uint64_t revision_{0};
};

/**
 * Owned rectangular string storage used by tests and small examples.
 *
 * The concrete storage is intentionally separate from the TableModel contract. A database-backed
 * or generated model can expose the same visible range without copying its complete data set.
 */
class StringTableModel final : public TableModel {
public:
    StringTableModel() = default;
    StringTableModel(std::vector<std::string> headers,
                     std::vector<std::vector<std::string>> rows);

    [[nodiscard]] std::size_t rowCount() const noexcept override { return rows_.size(); }
    [[nodiscard]] std::size_t columnCount() const noexcept override { return headers_.size(); }
    [[nodiscard]] std::string_view headerAt(std::size_t column) const override {
        return headers_.at(column);
    }
    [[nodiscard]] std::string_view textAt(std::size_t row, std::size_t column) const override {
        if (row >= rows_.size() || column >= headers_.size()) {
            throw std::out_of_range{"StringTableModel cell out of range"};
        }
        return rows_[row][column];
    }

    void appendRow(std::vector<std::string> row);
    void insertRow(std::size_t row, std::vector<std::string> values);
    void eraseRow(std::size_t row);
    void setCell(std::size_t row, std::size_t column, std::string value);
    void reset(std::vector<std::string> headers,
               std::vector<std::vector<std::string>> rows);

private:
    void validateRow(const std::vector<std::string>& row) const;
    void validateTable(const std::vector<std::string>& headers,
                       const std::vector<std::vector<std::string>>& rows) const;

    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> rows_;
};

} // namespace sasd::ui
