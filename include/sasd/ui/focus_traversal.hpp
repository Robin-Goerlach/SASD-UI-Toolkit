#pragma once

#include <sasd/ui/events/event.hpp>

namespace sasd::ui {

class Container;
class FocusManager;

/** Direction used by deterministic keyboard-focus traversal. */
enum class FocusTraversalDirection {
    forward,
    backward,
};

/**
 * Stateless keyboard-focus traversal policy for one visual focus scope.
 *
 * The traversal order is visual-tree preorder: each visual child appears before its descendants, and
 * siblings follow Container adoption order. Invisible or disabled subtrees are skipped completely.
 *
 * FocusTraversal deliberately does not own focus state and does not participate in normal event
 * bubbling. FocusManager remains responsible for state/lifetime; this helper only chooses a target.
 */
class FocusTraversal final {
public:
    FocusTraversal() = delete;

    /**
     * Moves focus to the next eligible descendant of scope and wraps at either end.
     *
     * When no current focus belongs to the eligible sequence, forward selects the first candidate and
     * backward selects the last. A one-candidate scope is considered successfully traversable and
     * leaves focus on that same widget.
     *
     * @returns true when the scope contains at least one eligible candidate and the resulting focus is
     *          that selected candidate.
     */
    [[nodiscard]] static bool move(FocusManager& focus,
                                   Container& scope,
                                   FocusTraversalDirection direction);

    /**
     * Handles a Tab/Shift+Tab event as focus traversal.
     *
     * Only unmodified Tab and Shift+Tab are claimed. Ctrl/Alt/Meta combinations remain available for
     * application shortcuts. Key release is consumed when the scope has a focus candidate but does not
     * move focus again.
     *
     * This method is intended to compose with Application::processRoutedEvents() as an unhandled-event
     * policy. A control that deliberately consumes Tab prevents traversal naturally.
     */
    [[nodiscard]] static EventResult handleEvent(FocusManager& focus,
                                                  Container& scope,
                                                  const Event& event);
};

} // namespace sasd::ui
