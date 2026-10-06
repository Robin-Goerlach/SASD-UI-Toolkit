#pragma once

#include <sasd/ui/events/event.hpp>
#include <sasd/ui/menu_interaction_controller.hpp>
#include <sasd/ui/terminal/menu_hit_test.hpp>
#include <sasd/ui/terminal/menu_pointer_intent.hpp>

#include <chrono>
#include <cstddef>
#include <optional>
#include <stdexcept>

namespace sasd::ui::terminal {

/** Configuration for delaying sibling-row replacement while pointer intent targets an open child submenu. */
struct TerminalMenuPointerDeferralOptions {
    std::chrono::milliseconds submenu_switch_delay{300};

    friend constexpr bool operator==(const TerminalMenuPointerDeferralOptions&,
                                     const TerminalMenuPointerDeferralOptions&) = default;
};

/** Host decision for one pointer sample presented to TerminalMenuPointerDeferralInteraction. */
enum class TerminalMenuPointerDeferralDecision {
    process_now,
    defer,
};

/**
 * Host-owned deterministic timing policy for safe-triangle sibling-row deferral.
 *
 * TerminalMenuPointerIntent is deliberately advisory: it can prove that a motion sample still lies inside
 * the transfer corridor toward the deepest open child popup, but it neither suppresses nor delays the event.
 * This companion object adds only that missing time policy. It recognizes the narrow case in which an
 * intent-qualified motion is physically over a different row of the owning parent popup, retains the latest
 * motion sample for a bounded interval, and lets the host decide later whether to replay that sample through
 * ordinary TerminalMenuPointerInteraction.
 *
 * The policy never mutates MenuInteractionController. In particular it does not select the sibling row,
 * close the child, synthesize Core keys, or execute commands. At expiration advance() returns a copied
 * PointerEvent; the host must submit that value to the normal pointer adapter against its CURRENT presentation
 * frame. That separation keeps all semantic selection/path repair in the existing interaction controller and
 * prevents this timing helper from becoming a second menu state machine.
 *
 * A pending transaction stores only value state: selected top-level index, complete open MenuPath, owning
 * parent popup row, the first deferral timestamp, and the latest deferred PointerEvent. It retains no
 * MenuModel/MenuItem pointer, Widget, presentation-frame pointer, terminal handle, callback, thread, or timer.
 * Explicit steady-clock TimePoint values make the behavior deterministic in tests and compatible with the
 * existing polling host loop.
 *
 * The first timestamp is intentionally preserved while later intent-qualified samples remain in the same
 * semantic scope. All-motion terminals can report many points while the pointer slides diagonally across one
 * or several sibling rows; restarting the timeout for every packet could suppress the real sibling selection
 * indefinitely. The stored event itself IS updated so expiration replays the user's most recent physical row.
 */
class TerminalMenuPointerDeferralInteraction final {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit TerminalMenuPointerDeferralInteraction(
        TerminalMenuPointerDeferralOptions options = {})
        : options_{options} {
        if (options_.submenu_switch_delay < std::chrono::milliseconds::zero()) {
            throw std::invalid_argument(
                "TerminalMenuPointerDeferralInteraction requires a non-negative submenu switch delay");
        }
    }

    /** Retires any deferred pointer sample without changing semantic menu state. */
    void reset() noexcept { pending_.reset(); }

    [[nodiscard]] bool hasPendingEvent() const noexcept { return pending_.has_value(); }

    [[nodiscard]] const TerminalMenuPointerDeferralOptions& options() const noexcept {
        return options_;
    }

    /**
     * Decides whether one PRE-interaction pointer event should run immediately or be deferred.
     *
     * The host first calls TerminalMenuPointerIntent::observe() against the same pre-interaction frame/state,
     * then supplies that advisory classification here. Only a move classified as toward_open_submenu can be
     * deferred, and even then only when its current geometric hit is a DIFFERENT row of the parent popup that
     * owns the deepest open child. Gap motion, child motion, top-level-title motion, press/release, or an intent
     * result of none clears any pending transaction and returns process_now.
     *
     * This narrow qualification matters. Gap motion does not threaten the owning selection and is already
     * handled by TerminalMenuCloseInteraction. Child motion means the transfer succeeded. Deferring either one
     * would add latency without protecting anything.
     */
    [[nodiscard]] TerminalMenuPointerDeferralDecision
    observe(const MenuInteractionController& controller,
            const MenuFramePresentationSnapshot& frame,
            const PointerEvent& event,
            TerminalMenuPointerIntentKind intent,
            AmbiguousWidthMode ambiguous_width,
            TimePoint now) {
        if (event.action != PointerAction::move ||
            intent != TerminalMenuPointerIntentKind::toward_open_submenu) {
            reset();
            return TerminalMenuPointerDeferralDecision::process_now;
        }

        const auto scope = currentScope(controller, frame);
        if (!scope.has_value()) {
            reset();
            return TerminalMenuPointerDeferralDecision::process_now;
        }

        const auto hit = TerminalMenuHitTest::popupItemAt(
            frame,
            event.position,
            ambiguous_width);
        if (!isSiblingThreat(hit, scope->owning_parent_item)) {
            /*
             * Intent can remain true while the pointer is geometrically in the parent/child gap. That sample
             * is harmless to immediate menu selection, so do not turn it into a delayed transaction. Clearing
             * an older sibling sample also prevents it from firing after the pointer has already left that row.
             */
            reset();
            return TerminalMenuPointerDeferralDecision::process_now;
        }

        if (pending_.has_value() && sameScope(*pending_, *scope)) {
            /*
             * Preserve the first timestamp but follow the latest physical row. If the pointer crosses row 1
             * and then row 2 inside the same corridor, timeout should eventually select row 2 rather than
             * replaying an obsolete earlier sample or extending the grace period forever.
             */
            pending_->deferred_event = event;
            return TerminalMenuPointerDeferralDecision::defer;
        }

        pending_ = Pending{
            scope->top_level_index,
            scope->popup_path,
            scope->owning_parent_item,
            now,
            event,
        };
        return TerminalMenuPointerDeferralDecision::defer;
    }

    /**
     * Returns an expired deferred event when its structural and CURRENT presentation scope still match.
     *
     * The candidate is revalidated before the deadline is committed. If the selected root menu, complete
     * MenuPath, popup depth, or owning parent row changed, the old event is discarded. The stored position is
     * also hit-tested against the supplied current frame; a resize/reflow that moves that cell away from the
     * sibling parent row therefore fails closed even if a host forgot to reset this helper explicitly.
     *
     * No controller mutation happens here. A returned event is merely permission for the host to run its usual
     * TerminalMenuPointerInteraction::handle(...) path. The pending transaction is retired BEFORE returning the
     * event so the replay cannot recursively inherit the same deferral state.
     */
    [[nodiscard]] std::optional<PointerEvent>
    advance(const MenuInteractionController& controller,
            const MenuFramePresentationSnapshot& frame,
            AmbiguousWidthMode ambiguous_width,
            TimePoint now) {
        if (!pending_.has_value()) {
            return std::nullopt;
        }

        const auto scope = currentScope(controller, frame);
        if (!scope.has_value() || !sameScope(*pending_, *scope)) {
            reset();
            return std::nullopt;
        }

        const auto hit = TerminalMenuHitTest::popupItemAt(
            frame,
            pending_->deferred_event.position,
            ambiguous_width);
        if (!isSiblingThreat(hit, scope->owning_parent_item)) {
            reset();
            return std::nullopt;
        }

        if (now < pending_->first_observed ||
            now - pending_->first_observed < options_.submenu_switch_delay) {
            return std::nullopt;
        }

        const PointerEvent event = pending_->deferred_event;
        reset();
        return event;
    }

private:
    struct Scope {
        std::size_t top_level_index{0};
        MenuPath popup_path{};
        TerminalMenuPopupHit owning_parent_item{};
    };

    struct Pending {
        std::size_t top_level_index{0};
        MenuPath popup_path{};
        TerminalMenuPopupHit owning_parent_item{};
        TimePoint first_observed{};
        PointerEvent deferred_event{};
    };

    [[nodiscard]] static std::optional<Scope>
    currentScope(const MenuInteractionController& controller,
                 const MenuFramePresentationSnapshot& frame) {
        if (!controller.isActive() || !controller.popupOpen() || controller.popupDepth() <= 1U) {
            return std::nullopt;
        }

        const auto top_level = controller.menuBarSelection();
        const auto& popup_path = controller.popupPath();
        const std::size_t depth = controller.popupDepth();
        if (!top_level.has_value() || !popup_path.has_value() || popup_path->empty() ||
            popup_path->size() + 1U != depth || frame.popups.size() != depth) {
            return std::nullopt;
        }

        const std::size_t child_level = depth - 1U;
        const std::size_t parent_level = child_level - 1U;
        const std::size_t parent_item_index = popup_path->back();
        if (parent_item_index >= frame.popups[parent_level].snapshot.items.size()) {
            return std::nullopt;
        }

        return Scope{
            *top_level,
            *popup_path,
            TerminalMenuPopupHit{parent_level, parent_item_index},
        };
    }

    [[nodiscard]] static bool isSiblingThreat(
        const std::optional<TerminalMenuPopupHit>& hit,
        TerminalMenuPopupHit owning_parent_item) noexcept {
        return hit.has_value() &&
               hit->level == owning_parent_item.level &&
               hit->item_index != owning_parent_item.item_index;
    }

    [[nodiscard]] static bool sameScope(const Pending& pending,
                                        const Scope& scope) noexcept {
        return pending.top_level_index == scope.top_level_index &&
               pending.popup_path == scope.popup_path &&
               pending.owning_parent_item == scope.owning_parent_item;
    }

    TerminalMenuPointerDeferralOptions options_{};
    std::optional<Pending> pending_{};
};

} // namespace sasd::ui::terminal
