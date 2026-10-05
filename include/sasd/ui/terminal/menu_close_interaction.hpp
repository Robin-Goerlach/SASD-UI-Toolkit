#pragma once

#include <sasd/ui/events/event.hpp>
#include <sasd/ui/menu_interaction_controller.hpp>
#include <sasd/ui/terminal/menu_hit_test.hpp>

#include <chrono>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>

namespace sasd::ui::terminal {

/** Configuration for delayed closing of already-open terminal submenu descendants. */
struct TerminalMenuCloseOptions {
    std::chrono::milliseconds submenu_close_delay{250};

    friend constexpr bool operator==(const TerminalMenuCloseOptions&,
                                     const TerminalMenuCloseOptions&) = default;
};

/**
 * Host-owned deterministic grace period for keeping open submenu descendants alive while the pointer
 * temporarily leaves every popup row.
 *
 * The immediate pointer adapter already guarantees semantic path coherence: selecting another ancestor row
 * closes descendants immediately because the old child no longer belongs to the new selection. A different
 * case occurs while the pointer travels through cells that belong to no popup row at all, for example a fitted
 * one-cell gap between a parent popup and its child. Closing the child immediately at that geometric gap would
 * make submenu transfer unnecessarily fragile; keeping it forever would leave stale descendants open after the
 * user has clearly moved away.
 *
 * This companion object supplies only the missing time policy. It observes the same presentation frame after
 * ordinary pointer interaction has run. When a menu has child popups open and PointerAction::move lands outside
 * every popup row, one close candidate is armed. Returning to any popup row before the configured deadline
 * cancels that candidate. Repeated outside-motion reports in the same semantic popup scope preserve the original
 * timestamp so terminal all-motion traffic cannot postpone the close indefinitely.
 *
 * No MenuModel/MenuItem, Widget, terminal device, timer handle, thread, callback, or presentation-frame pointer
 * is retained. Candidate scope is represented only by the selected top-level index, the complete open MenuPath,
 * and the first-observed steady-clock time. The host provides explicit TimePoint values, making the behavior
 * deterministic in tests and compatible with the toolkit's existing polling/event-loop architecture.
 *
 * At the deadline the candidate is retired first, then the controller's existing nested-popup Left transition
 * is applied repeatedly until only the root popup remains. Reusing that established semantic transition avoids
 * adding a new public Core mutation solely for one terminal timing policy. The loop never sends Left at root,
 * where that key would mean top-level menu switching.
 */
class TerminalMenuCloseInteraction final {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit TerminalMenuCloseInteraction(TerminalMenuCloseOptions options = {})
        : options_{options} {
        if (options_.submenu_close_delay < std::chrono::milliseconds::zero()) {
            throw std::invalid_argument(
                "TerminalMenuCloseInteraction requires a non-negative submenu close delay");
        }
    }

    /** Retires any pending close candidate without changing semantic menu state. */
    void reset() noexcept { candidate_.reset(); }

    [[nodiscard]] bool hasPendingCandidate() const noexcept {
        return candidate_.has_value();
    }

    [[nodiscard]] const TerminalMenuCloseOptions& options() const noexcept { return options_; }

    /**
     * Observes one pointer event after TerminalMenuPointerInteraction has processed it.
     *
     * The post-interaction ordering is deliberate. If motion selected another ancestor row, Core may already
     * have closed descendants that no longer belong to that parent; in that case popupDepth() has returned to
     * one and there is nothing for a delayed close policy to own. The grace period is only for geometry that
     * lies outside all popup rows while a coherent child route is still open.
     *
     * Any non-motion pointer event resets the candidate so click completion cannot inherit timing from an older
     * hover transfer. Motion over any visible popup row also resets it: reaching either the parent or one of its
     * descendants proves that the pointer is still interacting with the popup chain. Motion outside all popup
     * rows arms one candidate when at least one child popup is open.
     */
    void observe(const MenuInteractionController& controller,
                 const MenuFramePresentationSnapshot& frame,
                 const PointerEvent& event,
                 AmbiguousWidthMode ambiguous_width,
                 TimePoint now) {
        if (!controller.isActive() || !controller.popupOpen() ||
            controller.popupDepth() <= 1U || event.action != PointerAction::move) {
            reset();
            return;
        }

        if (TerminalMenuHitTest::popupItemAt(frame, event.position, ambiguous_width).has_value()) {
            /*
             * Any popup row is a valid transfer destination. Do not distinguish parent/child kinds here;
             * immediate pointer interaction and Core semantic state already decide which route remains valid.
             */
            reset();
            return;
        }

        const auto top_level = controller.menuBarSelection();
        const auto& popup_path = controller.popupPath();
        if (!top_level.has_value() || !popup_path.has_value() || popup_path->empty() ||
            popup_path->size() + 1U != controller.popupDepth()) {
            /*
             * A child popup should imply one structural path element per additional popup level. If that value
             * invariant cannot be proven, do not arm timing against potentially stale presentation geometry.
             */
            reset();
            return;
        }

        if (candidate_.has_value() &&
            candidate_->top_level_index == *top_level &&
            candidate_->popup_path == *popup_path) {
            /*
             * Preserve the first outside timestamp for the same semantic popup chain. All-motion terminals can
             * produce a dense stream of reports while crossing a gap; restarting here could keep descendants
             * open indefinitely even though the pointer never returned to a popup.
             */
            return;
        }

        candidate_ = Candidate{
            *top_level,
            *popup_path,
            now,
        };
    }

    /**
     * Advances the pending close transaction against current semantic menu state.
     *
     * Scope is checked again before time is committed. If the selected top-level menu or structural popup path
     * changed, the old candidate is discarded rather than being reinterpreted against a numerically similar
     * route. Before the deadline no semantic mutation occurs.
     *
     * Once the delay expires, all child popups are closed back to the still-open root popup. The method uses
     * the controller's existing nested Left-key transition one level at a time, stopping defensively if a call
     * fails to reduce depth. This preserves the controller as the sole owner of MenuPath invariants and avoids
     * exposing terminal timing concepts in Core.
     */
    [[nodiscard]] MenuInteractionResult advance(const MenuBarModel& bar,
                                                MenuInteractionController& controller,
                                                TimePoint now) {
        if (!candidate_.has_value()) {
            return {};
        }

        if (!scopeStillMatches(controller, *candidate_)) {
            reset();
            return {};
        }

        if (now < candidate_->first_observed ||
            now - candidate_->first_observed < options_.submenu_close_delay) {
            return {};
        }

        reset();

        bool changed = false;
        while (controller.isActive() && controller.popupOpen() && controller.popupDepth() > 1U) {
            const std::size_t depth_before = controller.popupDepth();
            const MenuInteractionResult step = controller.handleKey(
                bar,
                KeyEvent{Key::left, true, KeyModifier::none});

            if (step.action == MenuInteractionAction::state_changed) {
                changed = true;
            } else if (step.action == MenuInteractionAction::activate_command ||
                       step.action == MenuInteractionAction::closed) {
                /*
                 * Left on a valid nested popup should never activate or close the complete menu. Preserve any
                 * future controller contract change instead of converting it to a misleading state_changed.
                 */
                return step;
            }

            if (controller.popupDepth() >= depth_before) {
                /*
                 * Defensive progress guard. A future key policy may stop interpreting Left as parent-close;
                 * never spin the host loop if that semantic contract changes.
                 */
                break;
            }
        }

        return {changed ? MenuInteractionAction::state_changed : MenuInteractionAction::none, {}};
    }

private:
    struct Candidate {
        std::size_t top_level_index{0};
        MenuPath popup_path{};
        TimePoint first_observed{};
    };

    [[nodiscard]] static bool scopeStillMatches(const MenuInteractionController& controller,
                                                const Candidate& candidate) noexcept {
        const auto& popup_path = controller.popupPath();
        return controller.isActive() && controller.popupOpen() && controller.popupDepth() > 1U &&
               controller.menuBarSelection() == candidate.top_level_index &&
               popup_path.has_value() && *popup_path == candidate.popup_path &&
               popup_path->size() + 1U == controller.popupDepth();
    }

    TerminalMenuCloseOptions options_{};
    std::optional<Candidate> candidate_{};
};

} // namespace sasd::ui::terminal
