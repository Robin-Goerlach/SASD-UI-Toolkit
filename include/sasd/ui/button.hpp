#pragma once

#include <sasd/ui/command.hpp>
#include <sasd/ui/style.hpp>
#include <sasd/ui/widget.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace sasd::ui {

/**
 * First interactive semantic control.
 *
 * Button owns backend-neutral visual/control state and may optionally bind to one semantic Command.
 * Native handles, terminal decorations and rendered styles remain presentation concerns.
 *
 * Keyboard activation intentionally uses KeyEvent rather than TextInputEvent. Enter/Space describe
 * control intent; text input remains reserved for editable textual content such as TextField.
 *
 * Pointer activation uses an armed press/release gesture. PointerRouter capture guarantees the
 * matching release reaches the same route even after leaving the Button; the Button then activates
 * only when that release is still geometrically inside its clipped visual bounds.
 */
class Button final : public Widget {
public:
    using ActivationHandler = std::function<void()>;

    Button();
    explicit Button(std::string text);

    /** Returns the UTF-8 caption represented by the button. */
    [[nodiscard]] std::string_view text() const noexcept { return text_; }

    /** Returns presentation-only text/chrome styling for this button. */
    [[nodiscard]] const TextStyle& textStyle() const noexcept { return text_style_; }

    /**
     * Returns the transient semantic pressed state consumed by presentation backends.
     *
     * It is true only while a primary pointer gesture is armed, the captured pointer remains inside
     * this Button's clipped bounds, and the Button is visible/enabled. Keyboard activation remains
     * stateless because terminal input cannot reliably provide paired key-up events.
     */
    [[nodiscard]] bool isPressed() const noexcept {
        return pointer_armed_ && pointer_inside_ && isVisible() && isEnabled();
    }

    /** Changes style without affecting intrinsic size. */
    void setTextStyle(TextStyle style);

    /**
     * Replaces the UTF-8 caption.
     *
     * Caption changes can affect intrinsic size and visible presentation, so both caches are
     * invalidated. Assigning the identical byte sequence is a no-op.
     *
     * When a Command is bound this remains a legal local mutation; the next relevant Command text
     * change synchronizes the caption again. Binding is intentionally one-way rather than turning the
     * Button into an implicit two-way property system.
     */
    void setText(std::string text);

    /** Replaces the optional local synchronous activation callback. */
    void setOnActivated(ActivationHandler handler) { on_activated_ = std::move(handler); }

    /**
     * Binds this Button to a semantic Command without taking ownership of that Command.
     *
     * Binding immediately copies the Command text/enabled snapshot and then observes future changes.
     * Button activation delegates to Command::execute() while the binding is active. The local
     * onActivated handler is retained but suppressed until unbindCommand() is called.
     *
     * The relationship is lifetime-safe and non-owning. If the Command is destroyed first,
     * boundCommand() becomes nullptr and later activation is rejected rather than dereferencing stale
     * storage. The Button deliberately keeps the last synchronized text/enabled snapshot; automatic
     * visual fallback policy after semantic-object destruction is left to a later richer binding layer.
     */
    void bindCommand(Command& command);

    /**
     * Removes the current Command binding.
     *
     * The Button keeps its current text and enabled state. Any local onActivated handler becomes the
     * activation target again. Repeated calls are harmless.
     */
    void unbindCommand() noexcept;

    /** Returns the currently live bound Command, or nullptr when unbound/expired. */
    [[nodiscard]] Command* boundCommand() noexcept;
    [[nodiscard]] const Command* boundCommand() const noexcept;

    /**
     * Performs programmatic semantic activation.
     *
     * Disabled buttons reject activation. Visibility/focus are deliberately not required for a
     * programmatic call. When a Command binding is active, activation delegates to that Command and
     * returns its acceptance result. An expired binding rejects activation until explicitly unbound or
     * rebound; it never falls through to an unrelated local callback.
     *
     * For an unbound Button the local callback is copied before invocation. This lets the callback
     * safely replace its handler or release/reparent the Button without destroying the std::function
     * object currently being invoked.
     *
     * @returns true when the active semantic target accepted activation.
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
    void onPointerCaptureLost() noexcept override;

private:
    void setPointerGestureState(bool armed, bool inside) noexcept;
    void synchronizeBoundCommandState(Command::StateChange change);

    std::string text_;
    TextStyle text_style_{};
    ActivationHandler on_activated_;

    /*
     * The Reference protects invocation from Command lifetime, while StateSubscription owns exactly
     * the incremental synchronization connection. command_binding_active_ distinguishes an explicitly
     * expired binding from a Button that was never bound: expiration must reject activation rather than
     * silently changing semantics by falling back to the local callback.
     */
    Command::Reference command_reference_;
    Command::StateSubscription command_subscription_;
    bool command_binding_active_{false};

    /*
     * Armed records ownership of the gesture; inside records its current geometric position.
     * Presentation observes only isPressed(), keeping capture mechanics private.
     */
    bool pointer_armed_{false};
    bool pointer_inside_{false};
};

} // namespace sasd::ui
