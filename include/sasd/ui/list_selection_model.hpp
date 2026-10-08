#pragma once

#include <sasd/ui/list_model.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace sasd::ui {

struct ListSelectionChange final {
    std::optional<std::size_t> previous;
    std::optional<std::size_t> current;
};

/**
 * Single-row selection independent from keyboard focus.
 *
 * The model is observed but never owned. When rows are inserted or removed, a selected row after the
 * changed range is shifted to continue identifying the same surviving positional item; a selected row
 * inside a removed range is cleared. Reset and model destruction fail closed. This policy is specific
 * to positional list selection and is intentionally not presented as a universal selection model.
 */
class ListSelectionModel final {
private:
    struct ObserverSlot;
    struct ObserverState;

public:
    using ChangedHandler = std::function<void(const ListSelectionChange&)>;

    class Reference final {
    public:
        Reference() noexcept = default;
        [[nodiscard]] ListSelectionModel* get() const noexcept;
        [[nodiscard]] explicit operator bool() const noexcept { return get() != nullptr; }

    private:
        friend class ListSelectionModel;
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
        friend class ListSelectionModel;
        explicit Subscription(std::weak_ptr<ObserverSlot> slot) noexcept : slot_{std::move(slot)} {}
        std::weak_ptr<ObserverSlot> slot_;
    };

    ListSelectionModel() = default;
    ~ListSelectionModel();

    [[nodiscard]] ListModel* model() const noexcept { return model_.get(); }
    [[nodiscard]] Reference reference();
    [[nodiscard]] Subscription observe(ChangedHandler handler);

    /** Rebinds observation without taking ownership and clears the previous selection. */
    void setModel(ListModel* model);

    /** Returns no selection when the observed model has expired or the row is no longer valid. */
    [[nodiscard]] std::optional<std::size_t> selectedRow() const noexcept;

    [[nodiscard]] bool clear();
    [[nodiscard]] bool select(std::size_t row);
    [[nodiscard]] bool selectNext();
    [[nodiscard]] bool selectPrevious();
    [[nodiscard]] bool selectFirst();
    [[nodiscard]] bool selectLast();

private:
    struct ObserverSlot {
        ChangedHandler handler;
        bool active{true};
    };

    struct ObserverState {
        ListSelectionModel* selection{nullptr};
        std::vector<std::shared_ptr<ObserverSlot>> slots;
    };

    void modelChanged(const ListModelChange& change);
    bool setSelected(std::optional<std::size_t> row);
    void notifySelection(std::optional<std::size_t> previous);

    [[nodiscard]] std::shared_ptr<ObserverState> ensureObserverState();
    static void compactInactiveObservers(const std::shared_ptr<ObserverState>& state);
    static void notifyObservers(std::shared_ptr<ObserverState> state,
                                const ListSelectionChange& change);

    ListModel::Reference model_;
    ListModel::Subscription model_subscription_;
    std::optional<std::size_t> selected_row_;
    std::shared_ptr<ObserverState> observers_;
};

} // namespace sasd::ui
