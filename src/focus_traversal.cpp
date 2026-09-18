#include <sasd/ui/focus_traversal.hpp>

#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/widget.hpp>

#include <algorithm>
#include <cstddef>
#include <variant>
#include <vector>

namespace sasd::ui {
namespace {

/**
 * Collects eligible focus targets in deterministic visual preorder.
 *
 * Visibility/enabled state is treated effectively for traversal: if a Container is hidden or
 * disabled, descendants are unreachable even if their own local flags remain true. This is stricter
 * than Widget::canReceiveFocus()'s current local-only M1 contract and is intentionally a property of
 * keyboard navigation rather than a silent change to explicit FocusManager::requestFocus().
 */
void collectCandidates(Container& container, std::vector<Widget*>& candidates) {
    for (std::size_t index = 0; index < container.childCount(); ++index) {
        Widget& child = container.childAt(index);

        if (!child.isVisible() || !child.isEnabled()) {
            // A hidden/disabled visual subtree cannot be reached by user keyboard traversal.
            continue;
        }

        if (child.canReceiveFocus()) {
            candidates.push_back(&child);
        }

        if (auto* nested = dynamic_cast<Container*>(&child)) {
            collectCandidates(*nested, candidates);
        }
    }
}

[[nodiscard]] std::vector<Widget*> candidatesFor(Container& scope) {
    std::vector<Widget*> candidates;

    /*
     * Scope visibility/enabled state gates the complete traversal tree. A hidden Window/Panel should
     * not yield keyboard targets merely because individual descendants still have local visible=true.
     */
    if (!scope.isVisible() || !scope.isEnabled()) {
        return candidates;
    }

    collectCandidates(scope, candidates);
    return candidates;
}

[[nodiscard]] bool hasOnlyShift(KeyModifier modifiers) noexcept {
    return modifiers == KeyModifier::shift;
}

} // namespace

bool FocusTraversal::move(FocusManager& focus,
                          Container& scope,
                          FocusTraversalDirection direction) {
    std::vector<Widget*> candidates = candidatesFor(scope);
    if (candidates.empty()) {
        return false;
    }

    Widget* const current = focus.focusedWidget();
    const auto current_it = std::find(candidates.begin(), candidates.end(), current);

    Widget* target = nullptr;

    if (current_it == candidates.end()) {
        target = direction == FocusTraversalDirection::forward
                     ? candidates.front()
                     : candidates.back();
    } else {
        const std::size_t current_index =
            static_cast<std::size_t>(std::distance(candidates.begin(), current_it));

        if (direction == FocusTraversalDirection::forward) {
            const std::size_t next_index = (current_index + 1U) % candidates.size();
            target = candidates[next_index];
        } else {
            const std::size_t previous_index =
                current_index == 0U ? candidates.size() - 1U : current_index - 1U;
            target = candidates[previous_index];
        }
    }

    /*
     * requestFocus() owns transition/lifetime/re-entrancy semantics. Do not duplicate them here. In
     * particular, a focus-lost callback may redirect focus; requestFocus() reports that final state.
     */
    return target != nullptr && focus.requestFocus(*target);
}

EventResult FocusTraversal::handleEvent(FocusManager& focus,
                                        Container& scope,
                                        const Event& event) {
    const auto* key = std::get_if<KeyEvent>(&event);
    if (key == nullptr || key->key != Key::tab) {
        return EventResult::ignored;
    }

    const bool forward = key->modifiers == KeyModifier::none;
    const bool backward = hasOnlyShift(key->modifiers);

    if (!forward && !backward) {
        // Ctrl/Alt/Meta+Tab remains application/backend territory.
        return EventResult::ignored;
    }

    if (!key->pressed) {
        /*
         * Mirror Button/TextField key semantics: a desktop key-up must not repeat traversal. Consume
         * release only when this scope actually owns at least one traversable focus target.
         */
        return candidatesFor(scope).empty() ? EventResult::ignored : EventResult::handled;
    }

    const bool moved = move(focus,
                            scope,
                            forward ? FocusTraversalDirection::forward
                                    : FocusTraversalDirection::backward);
    return moved ? EventResult::handled : EventResult::ignored;
}

} // namespace sasd::ui
