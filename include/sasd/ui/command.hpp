#pragma once

#include <sasd/ui/component.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sasd::ui {

/**
 * Backend-neutral semantic command that can be owned like any other non-visual Component.
 *
 * Command deliberately remains smaller than a complete action system. It owns semantic identity
 * through object lifetime, user-facing UTF-8 text, enabled state, synchronous execution, and a small
 * lifetime-safe state-observation seam. Presentation, shortcut routing, menu policy and visual control
 * synchronization remain separate concerns built on top of this primitive.
 *
 * Because Command derives from Component rather than Widget, owning it in a Container participates in
 * normal lifetime/ownership semantics without adding anything to that Container's visual child list.
 * Presentation backends therefore never need to know that a Command exists.
 */
class Command final : public Component {
private:
    struct ObserverSlot;
    struct ObserverState;

public:
    /** Identifies the semantic property whose value changed. */
    enum class StateChange {
        text,
        enabled,
    };

    using ExecutionHandler = std::function<void()>;
    using StateChangedHandler = std::function<void(StateChange)>;

    /**
     * Move-only RAII connection returned by observeState().
     *
     * The subscription does not own the Command. Destroying or resetting the token disconnects the
     * observer; destroying the Command first simply expires the weak slot and leaves token destruction
     * harmless. This avoids storing raw Command pointers in later bindings merely to unsubscribe.
     */
    class StateSubscription final {
    public:
        StateSubscription() noexcept = default;
        ~StateSubscription() { reset(); }

        StateSubscription(const StateSubscription&) = delete;
        StateSubscription& operator=(const StateSubscription&) = delete;

        StateSubscription(StateSubscription&& other) noexcept
            : slot_{std::move(other.slot_)} {}

        StateSubscription& operator=(StateSubscription&& other) noexcept {
            if (this == &other) {
                return *this;
            }

            /*
             * Moving over an existing live token must disconnect that token first. Defaulted move
             * assignment would merely overwrite the weak_ptr and leave the old observer active with no
             * remaining handle through which client code could retire it.
             */
            reset();
            slot_ = std::move(other.slot_);
            return *this;
        }

        /** Returns whether the observed slot is still active. */
        [[nodiscard]] bool connected() const noexcept;

        /** Disconnects this observer. Repeated calls are harmless. */
        void reset() noexcept;

    private:
        friend class Command;

        explicit StateSubscription(std::weak_ptr<ObserverSlot> slot) noexcept
            : slot_{std::move(slot)} {}

        std::weak_ptr<ObserverSlot> slot_;
    };

    Command() = default;
    explicit Command(std::string text) : text_{std::move(text)} {}

    /** Returns the UTF-8 user-facing command text. */
    [[nodiscard]] std::string_view text() const noexcept { return text_; }

    /**
     * Replaces user-facing text metadata and synchronously notifies state observers on a real change.
     *
     * Assigning the identical byte sequence is a no-op. Observation is semantic only: Command still
     * performs no Widget invalidation and remains independent of presentation objects.
     */
    void setText(std::string text) {
        if (text_ == text) {
            return;
        }

        text_ = std::move(text);

        /*
         * Copy the registry before invoking client code. A callback may destroy this Command; the
         * shared registry keeps the in-flight notification safe while this method deliberately performs
         * no further member access after notifyObservers() returns.
         */
        notifyObservers(observers_, StateChange::text);
    }

    /** Returns whether semantic execution is currently permitted. */
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }

    /**
     * Enables or disables semantic execution and notifies observers only when the value changes.
     *
     * This is Command state rather than Widget state. A later binding may mirror it into one or more
     * controls, but Command itself remains independent of focus, visibility and presentation.
     */
    void setEnabled(bool enabled) {
        if (enabled_ == enabled) {
            return;
        }

        enabled_ = enabled;
        notifyObservers(observers_, StateChange::enabled);
    }

    /** Replaces the optional synchronous execution callback. */
    void setOnExecuted(ExecutionHandler handler) { on_executed_ = std::move(handler); }

    /**
     * Subscribes to future semantic state changes.
     *
     * Observation is synchronous and does not emit an initial snapshot. Callers that bind a visual or
     * menu object must read text()/isEnabled() once when establishing the binding, then retain the
     * returned token for incremental updates. An empty handler returns a disconnected token.
     *
     * Callbacks run in registration order for the current notification snapshot. Disconnecting a later
     * observer from an earlier callback prevents that later observer from running in the same pass.
     * Observers added during a callback begin with the next state change. The contract is intentionally
     * single-threaded; cross-thread command dispatch is outside the current UI-core model.
     */
    [[nodiscard]] StateSubscription observeState(StateChangedHandler handler) {
        if (!handler) {
            return {};
        }

        if (!observers_) {
            observers_ = std::make_shared<ObserverState>();
        }

        compactInactiveObservers(observers_);

        auto slot = std::make_shared<ObserverSlot>();
        slot->handler = std::move(handler);
        observers_->slots.push_back(slot);
        return StateSubscription{slot};
    }

    /**
     * Requests semantic execution.
     *
     * Disabled commands reject execution and return false. Enabled commands accept execution and return
     * true even when no callback is installed; this mirrors Button::activate(), where acceptance is a
     * semantic state decision and the presence of an observer is not part of eligibility.
     *
     * The callback is copied before invocation. That small lifetime rule is intentional: a handler may
     * replace the Command's current callback, release/reparent the Command through external ownership,
     * or otherwise mutate Command state without invalidating the std::function object that is presently
     * executing. No Command member is accessed after the callback returns.
     */
    bool execute() {
        if (!enabled_) {
            return false;
        }

        const auto handler = on_executed_;
        if (handler) {
            handler();
        }

        return true;
    }

private:
    struct ObserverSlot {
        StateChangedHandler handler;
        bool active{true};
    };

    struct ObserverState {
        std::vector<std::shared_ptr<ObserverSlot>> slots;
    };

    static void compactInactiveObservers(const std::shared_ptr<ObserverState>& state) {
        if (!state) {
            return;
        }

        state->slots.erase(
            std::remove_if(
                state->slots.begin(),
                state->slots.end(),
                [](const std::shared_ptr<ObserverSlot>& slot) {
                    return !slot || !slot->active;
                }),
            state->slots.end());
    }

    static void notifyObservers(std::shared_ptr<ObserverState> state, StateChange change) {
        if (!state) {
            return;
        }

        compactInactiveObservers(state);

        /*
         * Freeze registration membership for this notification while keeping each slot shared and
         * independently active. This gives mutations during callbacks deterministic semantics without
         * holding iterators into a vector that callbacks are allowed to extend.
         */
        const auto snapshot = state->slots;
        for (const auto& slot : snapshot) {
            if (!slot || !slot->active) {
                continue;
            }

            /*
             * Copy the callback before invoking client code. The subscription may be reset, moved or
             * destroyed while the callback runs; the local std::function remains valid for this call.
             */
            const auto handler = slot->handler;
            if (handler) {
                handler(change);
            }
        }
    }

    std::string text_;
    ExecutionHandler on_executed_;
    std::shared_ptr<ObserverState> observers_;
    bool enabled_{true};
};

inline bool Command::StateSubscription::connected() const noexcept {
    const auto slot = slot_.lock();
    return slot != nullptr && slot->active;
}

inline void Command::StateSubscription::reset() noexcept {
    if (const auto slot = slot_.lock()) {
        slot->active = false;
    }
    slot_.reset();
}

} // namespace sasd::ui
