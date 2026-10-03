#pragma once

#include <sasd/ui/command.hpp>
#include <sasd/ui/events/event.hpp>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace sasd::ui {

/**
 * Backend-neutral key gesture that may invoke a semantic Command.
 *
 * Shortcut intentionally reuses KeyEvent identity instead of text input. A shortcut describes a key
 * gesture, not committed Unicode text; TextInputEvent remains reserved for editable textual content.
 * The initial Key enum currently exposes navigation/control/function keys only, so this first shortcut
 * layer is immediately useful for F-key driven terminal administration while printable shortcuts such
 * as Ctrl+S remain a later input-model extension rather than being guessed from text input.
 */
struct Shortcut {
    Key key{Key::unknown};
    KeyModifier modifiers{KeyModifier::none};

    [[nodiscard]] friend constexpr bool operator==(const Shortcut&, const Shortcut&) noexcept = default;

    /**
     * Returns whether this shortcut exactly matches a key-press event.
     *
     * Releases never trigger commands, and modifiers match exactly. Exact matching prevents a binding
     * such as F1 from also consuming Shift+F1 or Ctrl+F1, which applications may assign independently.
     */
    [[nodiscard]] constexpr bool matches(const KeyEvent& event) const noexcept {
        return key != Key::unknown && event.pressed && event.key == key && event.modifiers == modifiers;
    }
};

/**
 * Small explicit collection of Shortcut -> Command bindings.
 *
 * ShortcutMap is intentionally an ordinary object rather than a process-global registry. An
 * application, window, focus scope, or future menu host can therefore own the policy at the correct
 * lifetime/scope. Bindings retain only Command::Reference objects, so commands remain owned by the
 * existing Component tree and may disappear without leaving dangling pointers.
 *
 * The first contract has one binding per exact Shortcut. Rebinding the same gesture replaces the
 * previous command deterministically. Scope precedence, chord sequences, platform conventions,
 * textual key identities and conflict diagnostics remain later policies.
 */
class ShortcutMap final {
public:
    ShortcutMap() = default;

    /**
     * Binds an exact key gesture to a command, replacing an existing binding for that gesture.
     *
     * Key::unknown is rejected because it represents failed/unsupported input normalization rather
     * than an invokable gesture. The binding does not own the Command.
     */
    void bind(Shortcut shortcut, Command& command) {
        if (shortcut.key == Key::unknown) {
            throw std::invalid_argument{"Shortcut key must not be Key::unknown"};
        }

        compactExpired();

        const auto existing = std::find_if(
            bindings_.begin(),
            bindings_.end(),
            [shortcut](const Binding& binding) { return binding.shortcut == shortcut; });

        if (existing != bindings_.end()) {
            existing->command = command.reference();
            return;
        }

        bindings_.push_back(Binding{shortcut, command.reference()});
    }

    /** Removes the exact shortcut binding when present. */
    [[nodiscard]] bool unbind(Shortcut shortcut) noexcept {
        const auto old_size = bindings_.size();
        bindings_.erase(
            std::remove_if(
                bindings_.begin(),
                bindings_.end(),
                [shortcut](const Binding& binding) { return binding.shortcut == shortcut; }),
            bindings_.end());
        return bindings_.size() != old_size;
    }

    /** Removes every shortcut binding. */
    void clear() noexcept { bindings_.clear(); }

    /**
     * Resolves one exact key-press gesture to a lifetime-safe Command reference without executing it.
     *
     * This is the non-reentrant half of shortcut handling. A caller that must dismiss transient UI,
     * finish focus/presentation work, or otherwise stabilize application state before entering client
     * callbacks can resolve the gesture first and execute the returned Command later. The returned
     * reference does not keep either the ShortcutMap or Command alive.
     *
     * A live disabled Command still resolves. Matching and semantic eligibility are intentionally
     * different questions: state may change between resolution and later execution, and Command::execute()
     * remains the single authority that accepts or rejects execution. Releases, unknown keys, unmatched
     * gestures and expired bindings return an empty reference.
     *
     * The map may be mutated or destroyed after this function returns. The copied Command::Reference is
     * independent of binding storage and safely becomes empty if the Command itself is destroyed before
     * the caller executes it.
     */
    [[nodiscard]] Command::Reference resolve(const KeyEvent& event) {
        if (!event.pressed || event.key == Key::unknown) {
            return {};
        }

        compactExpired();

        const auto binding = std::find_if(
            bindings_.begin(),
            bindings_.end(),
            [&event](const Binding& candidate) { return candidate.shortcut.matches(event); });

        return binding != bindings_.end() ? binding->command : Command::Reference{};
    }

    /**
     * Attempts to execute the command bound to an exact key-press event.
     *
     * A matching but disabled command returns false because Command::execute() rejected the semantic
     * action. This deliberately leaves the event available to a caller's surrounding routing policy.
     * An expired command is removed lazily and also returns false.
     *
     * dispatch() is the convenience form for callers that do not require a two-phase transaction. It
     * delegates gesture resolution to resolve(), copies only the lifetime-safe Command::Reference, and
     * then enters application code. No ShortcutMap storage is accessed after Command::execute() begins;
     * client execution may destroy the Command, clear/rebind this map through external code, or otherwise
     * mutate UI state.
     */
    [[nodiscard]] bool dispatch(const KeyEvent& event) {
        const Command::Reference command_reference = resolve(event);
        Command* const command = command_reference.get();
        if (command == nullptr) {
            return false;
        }

        // execute() may synchronously destroy the Command. Do not access command or this map afterward.
        return command->execute();
    }

private:
    struct Binding {
        Shortcut shortcut{};
        Command::Reference command{};
    };

    void compactExpired() noexcept {
        bindings_.erase(
            std::remove_if(
                bindings_.begin(),
                bindings_.end(),
                [](const Binding& binding) { return binding.command.get() == nullptr; }),
            bindings_.end());
    }

    std::vector<Binding> bindings_;
};

} // namespace sasd::ui
