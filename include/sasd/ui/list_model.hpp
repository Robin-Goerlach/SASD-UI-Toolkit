#pragma once

#include <sasd/ui/component.hpp>

#include <algorithm>
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
 * Backend-neutral change description for a ListModel.
 *
 * Row indices are positional, not durable item identities. In particular, an insertion/removal
 * notification only describes the coherent post-change range; consumers must not reinterpret an
 * old index as the same application item after rows before it moved. A reset explicitly invalidates
 * every previous positional interpretation.
 */
struct ListModelChange final {
    enum class Kind {
        reset,
        rows_inserted,
        rows_removed,
        row_changed,
    };

    Kind kind{Kind::reset};
    std::size_t first_row{0};
    std::size_t row_count{0};
};

/**
 * Small model boundary for a virtualized textual list.
 *
 * ListModel is a non-visual Component so an application may own it in the same explicit ownership
 * tree as other services, but ListView never adopts it implicitly. `textAt()` returns a borrowed view
 * valid only until the next model mutation. Presentation code must copy text into its own frame
 * snapshot before allowing callbacks or later model changes to run.
 */
class ListModel : public Component {
private:
    struct ObserverSlot;
    struct ObserverState;

public:
    using ChangedHandler = std::function<void(const ListModelChange&)>;

    /** Lifetime-safe, non-owning reference used by views and selection state. */
    class Reference final {
    public:
        Reference() noexcept = default;

        [[nodiscard]] ListModel* get() const noexcept;
        [[nodiscard]] explicit operator bool() const noexcept { return get() != nullptr; }

    private:
        friend class ListModel;
        explicit Reference(std::weak_ptr<ObserverState> state) noexcept
            : state_{std::move(state)} {}

        std::weak_ptr<ObserverState> state_;
    };

    /** Move-only RAII observer token; it never extends model lifetime. */
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
        friend class ListModel;
        explicit Subscription(std::weak_ptr<ObserverSlot> slot) noexcept
            : slot_{std::move(slot)} {}

        std::weak_ptr<ObserverSlot> slot_;
    };

    ListModel() = default;
    ~ListModel() override;

    ListModel(const ListModel&) = delete;
    ListModel& operator=(const ListModel&) = delete;

    [[nodiscard]] virtual std::size_t rowCount() const noexcept = 0;
    [[nodiscard]] virtual std::string_view textAt(std::size_t row) const = 0;

    /** Monotonic semantic revision used to reject stale presentation snapshots. */
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }

    [[nodiscard]] Reference reference();
    [[nodiscard]] Subscription observe(ChangedHandler handler);

protected:
    /** Publishes only after the derived model has completed its entire mutation. */
    void notifyChanged(ListModelChange change);

private:
    struct ObserverSlot {
        ChangedHandler handler;
        bool active{true};
    };

    struct ObserverState {
        ListModel* model{nullptr};
        std::vector<std::shared_ptr<ObserverSlot>> slots;
    };

    [[nodiscard]] std::shared_ptr<ObserverState> ensureObserverState();
    static void compactInactiveObservers(const std::shared_ptr<ObserverState>& state);
    static void notifyObservers(std::shared_ptr<ObserverState> state, const ListModelChange& change);

    std::shared_ptr<ObserverState> observers_;
    std::uint64_t revision_{0};
};

/**
 * Small owned string model for examples and deterministic tests.
 *
 * This is deliberately a consumer, not the definition of the ListModel abstraction. Applications
 * with generated, remote or database-backed rows can implement the four semantic operations without
 * adopting this vector storage or copying their complete data set into UI objects.
 */
class StringListModel final : public ListModel {
public:
    StringListModel() = default;
    explicit StringListModel(std::vector<std::string> rows) : rows_{std::move(rows)} {}

    [[nodiscard]] std::size_t rowCount() const noexcept override { return rows_.size(); }
    [[nodiscard]] std::string_view textAt(std::size_t row) const override {
        return rows_.at(row);
    }

    void append(std::string text);
    void insert(std::size_t row, std::string text);
    void erase(std::size_t row);
    void setText(std::size_t row, std::string text);
    void reset(std::vector<std::string> rows);

private:
    std::vector<std::string> rows_;
};

} // namespace sasd::ui
