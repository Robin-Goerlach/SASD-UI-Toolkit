#pragma once

#include <sasd/ui/widget.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace sasd::ui {

/**
 * First interactive semantic control.
 *
 * Button owns only backend-neutral state: UTF-8 caption text, focusability and an activation
 * callback. Native handles, terminal decorations and rendered styles remain presentation concerns.
 *
 * Keyboard activation intentionally uses KeyEvent rather than TextInputEvent. Enter/Space describe
 * control intent; text input remains reserved for editable textual content such as TextField.
 */
class Button final : public Widget {
public:
    using ActivationHandler = std::function<void()>;

    Button();
    explicit Button(std::string text);

    /** Returns the UTF-8 caption represented by the button. */
    [[nodiscard]] std::string_view text() const noexcept { return text_; }

    /**
     * Replaces the UTF-8 caption.
     *
     * Caption changes can affect intrinsic size and visible presentation, so both caches are
     * invalidated. Assigning the identical byte sequence is a no-op.
     */
    void setText(std::string text);

    /** Replaces the optional synchronous activation callback. */
    void setOnActivated(ActivationHandler handler) { on_activated_ = std::move(handler); }

    /**
     * Performs programmatic semantic activation.
     *
     * Disabled buttons reject activation. Visibility/focus are deliberately not required for a
     * programmatic call; application logic may invoke a command without simulating user input.
     *
     * The callback is copied before invocation. This lets the callback safely replace its handler or
     * release/reparent the Button without destroying the std::function object currently being invoked.
     * Destruction during an active routed event is deliberately not promised because EventDispatcher
     * reports the handling Widget through a non-owning pointer.
     *
     * @returns true when the button was enabled and activation was accepted.
     */
    bool activate();

protected:
    [[nodiscard]] Size onMeasure(const MeasurementContext& context,
                                 const MeasureConstraints& constraints) override;

    /**
     * Handles unmodified Enter/Space key events while this Button owns logical focus.
     *
     * Activation happens on the pressed event, not key release. Terminal input commonly has no
     * reliable key-up event, so release-dependent semantics would make the first backend unusable.
     * Key-release events for Enter/Space are nevertheless consumed while focused so they do not
     * unexpectedly bubble into a parent on desktop backends that do provide them.
     */
    [[nodiscard]] EventResult onEvent(const Event& event) override;

private:
    std::string text_;
    ActivationHandler on_activated_;
};

} // namespace sasd::ui
