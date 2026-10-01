#pragma once

#include <sasd/ui/component.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace sasd::ui {

/**
 * Backend-neutral semantic command that can be owned like any other non-visual Component.
 *
 * Command is deliberately smaller than a full action system. The first M4 contract provides only
 * semantic identity through object lifetime, user-facing text metadata, enabled state, and synchronous
 * execution. Menu binding, shortcuts, checked state, icons, change observation, and automatic control
 * synchronization remain later policies so this primitive does not accidentally become a UI toolkit
 * inside the UI toolkit.
 *
 * Because Command derives from Component rather than Widget, owning it in a Container participates in
 * normal lifetime/ownership semantics without adding anything to that Container's visual child list.
 * Presentation backends therefore never need to know that a Command exists.
 */
class Command final : public Component {
public:
    using ExecutionHandler = std::function<void()>;

    Command() = default;
    explicit Command(std::string text) : text_{std::move(text)} {}

    /** Returns the UTF-8 user-facing command text. */
    [[nodiscard]] std::string_view text() const noexcept { return text_; }

    /**
     * Replaces user-facing text metadata.
     *
     * Command is non-visual and currently has no observer/binding layer, so changing text deliberately
     * performs no Widget invalidation. Future control/menu bindings must subscribe through an explicit
     * state-observation contract rather than making Command depend on a presentation object.
     */
    void setText(std::string text) { text_ = std::move(text); }

    /** Returns whether semantic execution is currently permitted. */
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }

    /**
     * Enables or disables semantic execution.
     *
     * This is command state rather than Widget state. A future control binding may mirror it into a
     * Button/MenuItem, but Command itself remains independent of focus, visibility, and presentation.
     */
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

    /** Replaces the optional synchronous execution callback. */
    void setOnExecuted(ExecutionHandler handler) { on_executed_ = std::move(handler); }

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
    std::string text_;
    ExecutionHandler on_executed_;
    bool enabled_{true};
};

} // namespace sasd::ui
