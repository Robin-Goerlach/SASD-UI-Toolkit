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

/** Configuration for delayed pointer-hover submenu opening. */
struct TerminalMenuHoverOptions {
    std::chrono::milliseconds submenu_open_delay{300};

    friend constexpr bool operator==(const TerminalMenuHoverOptions&,
                                     const TerminalMenuHoverOptions&) = default;
};

/**
 * Host-owned deterministic timing policy for delayed terminal submenu opening.
 *
 * TerminalMenuPointerInteraction deliberately keeps PointerAction::move limited to immediate geometric
 * selection. This companion object adds only the missing time dimension needed for desktop-style submenu
 * hover: a stable popup-row identity is observed, retained for a configurable delay, and then submitted to
 * MenuInteractionController::openPopupSubmenu(). The semantic controller remains the final authority for
 * whether that identity still names the selected enabled submenu.
 *
 * The class owns no thread, timer handle, callback, MenuModel/MenuItem pointer, presentation snapshot, or
 * terminal device. Hosts supply an explicit monotonic TimePoint both when motion is observed and when the
 * event loop advances the hover policy. Tests can therefore exercise the complete timing state machine without
 * sleeping, while production hosts can reuse their ordinary event-loop clock.
 *
 * A pending candidate stores only value identity:
 *
 * - selected top-level menu index;
 * - the structural MenuPath prefix that owns the popup level;
 * - popup level and item index;
 * - first-observed time.
 *
 * Recording the root index and owner-path prefix is intentional. A bare {level,item_index} can be numerically
 * identical after the user switches to another root menu or another sibling submenu. Such scope changes must
 * retire the old hover candidate instead of reinterpreting it in unrelated menu semantics.
 */
class TerminalMenuHoverInteraction final {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit TerminalMenuHoverInteraction(TerminalMenuHoverOptions options = {})
        : options_{options} {
        if (options_.submenu_open_delay < std::chrono::milliseconds::zero()) {
            throw std::invalid_argument(
                "TerminalMenuHoverInteraction requires a non-negative submenu delay");
        }
    }

    /** Retires any pending hover identity without changing semantic menu state. */
    void reset() noexcept { candidate_.reset(); }

    [[nodiscard]] bool hasPendingCandidate() const noexcept {
        return candidate_.has_value();
    }

    [[nodiscard]] const TerminalMenuHoverOptions& options() const noexcept { return options_; }

    /**
     * Observes one pointer event after ordinary menu-pointer interaction has processed it.
     *
     * Hosts should call TerminalMenuPointerInteraction first, then pass the same event/frame here. This order
     * matters because motion selection may truncate invalid descendants before the hover candidate captures its
     * owning MenuPath prefix. The frame is used only synchronously for hit testing and is never retained.
     *
     * Only PointerAction::move over a visible popup row can arm a candidate. Press/release, an inactive menu,
     * or motion outside popup rows resets pending hover timing. Repeated motion within the same semantic popup
     * row preserves the original start time; otherwise a terminal producing many all-motion reports while the
     * pointer drifts horizontally across one row could postpone opening forever.
     */
    void observe(const MenuInteractionController& controller,
                 const MenuFramePresentationSnapshot& frame,
                 const PointerEvent& event,
                 AmbiguousWidthMode ambiguous_width,
                 TimePoint now) {
        if (!controller.isActive() || event.action != PointerAction::move) {
            reset();
            return;
        }

        const auto popup_hit =
            TerminalMenuHitTest::popupItemAt(frame, event.position, ambiguous_width);
        if (!popup_hit.has_value()) {
            reset();
            return;
        }

        const auto top_level = controller.menuBarSelection();
        const auto& popup_path = controller.popupPath();
        if (!top_level.has_value() || !popup_path.has_value() ||
            popup_hit->level > popup_path->size()) {
            /*
             * A visible hit should normally agree with the current controller snapshot. If semantic state has
             * already changed enough that the owning prefix cannot be proven, fail closed instead of arming an
             * identity against stale geometry.
             */
            reset();
            return;
        }

        MenuPath owner_path;
        owner_path.reserve(popup_hit->level);
        for (std::size_t depth = 0; depth < popup_hit->level; ++depth) {
            owner_path.push_back((*popup_path)[depth]);
        }

        if (candidate_.has_value() &&
            candidate_->top_level_index == *top_level &&
            candidate_->owner_path == owner_path &&
            candidate_->popup_item == *popup_hit) {
            /*
             * Keep the first-observed timestamp for the same semantic row. Continuous all-motion traffic must
             * not restart the hover delay merely because the pointer moved between cells inside that row.
             */
            return;
        }

        candidate_ = Candidate{
            *top_level,
            std::move(owner_path),
            *popup_hit,
            now,
        };
    }

    /**
     * Advances the pending hover transaction against the current semantic menu state.
     *
     * Before the configured delay expires this is a no-op. At/after the deadline the candidate is retired
     * first, then Core is asked to open exactly that submenu. Retiring before the semantic transaction avoids
     * repeated opening attempts and remains safe if a future controller operation triggers presentation/model
     * changes in the surrounding host.
     *
     * Scope is revalidated before the deadline is committed. A different top-level menu, a changed owner-path
     * prefix, or a closed popup cancels the candidate. The controller still performs the final selectability,
     * item-kind, model-liveness, and current-selection proof in openPopupSubmenu().
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
            now - candidate_->first_observed < options_.submenu_open_delay) {
            return {};
        }

        const TerminalMenuPopupHit popup_item = candidate_->popup_item;
        reset();
        return controller.openPopupSubmenu(
            bar,
            popup_item.level,
            popup_item.item_index);
    }

private:
    struct Candidate {
        std::size_t top_level_index{0};
        MenuPath owner_path{};
        TerminalMenuPopupHit popup_item{};
        TimePoint first_observed{};
    };

    [[nodiscard]] static bool scopeStillMatches(const MenuInteractionController& controller,
                                                const Candidate& candidate) noexcept {
        if (!controller.isActive() || !controller.popupOpen() ||
            controller.menuBarSelection() != candidate.top_level_index ||
            controller.popupDepth() <= candidate.popup_item.level) {
            return false;
        }

        const auto& popup_path = controller.popupPath();
        if (!popup_path.has_value() || popup_path->size() < candidate.owner_path.size()) {
            return false;
        }

        for (std::size_t depth = 0; depth < candidate.owner_path.size(); ++depth) {
            if ((*popup_path)[depth] != candidate.owner_path[depth]) {
                return false;
            }
        }

        return true;
    }

    TerminalMenuHoverOptions options_{};
    std::optional<Candidate> candidate_{};
};

} // namespace sasd::ui::terminal
