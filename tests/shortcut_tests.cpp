#include "test_framework.hpp"

#include <sasd/ui/shortcut.hpp>
#include <sasd/ui/shortcut_display.hpp>

#include <memory>
#include <stdexcept>

using namespace sasd::ui;

TEST_CASE("Shortcut matches only exact key presses") {
    const Shortcut help{Key::f1, KeyModifier::none};

    CHECK(help.matches(KeyEvent{Key::f1, true, KeyModifier::none}));
    CHECK(!help.matches(KeyEvent{Key::f1, false, KeyModifier::none}));
    CHECK(!help.matches(KeyEvent{Key::f1, true, KeyModifier::shift}));
    CHECK(!help.matches(KeyEvent{Key::f2, true, KeyModifier::none}));
}

TEST_CASE("Logical letter shortcuts preserve explicit modifiers and press identity") {
    const Shortcut select_all{Key::a, KeyModifier::control};
    const Shortcut shifted_copy{
        Key::c, KeyModifier::control | KeyModifier::shift};

    CHECK(select_all.matches(KeyEvent{Key::a, true, KeyModifier::control}));
    CHECK(!select_all.matches(KeyEvent{Key::a, false, KeyModifier::control}));
    CHECK(!select_all.matches(
        KeyEvent{Key::a, true, KeyModifier::control | KeyModifier::shift}));
    CHECK(shifted_copy.matches(
        KeyEvent{Key::c, true, KeyModifier::control | KeyModifier::shift}));
}

TEST_CASE("ShortcutMap resolves and dispatches conventional editing letters") {
    Command select_all{"Select All"};
    Command copy{"Copy"};
    Command cut{"Cut"};
    Command paste{"Paste"};
    ShortcutMap shortcuts;
    int selected = 0;
    int copied = 0;
    int cut_count = 0;
    int pasted = 0;

    select_all.setOnExecuted([&selected] { ++selected; });
    copy.setOnExecuted([&copied] { ++copied; });
    cut.setOnExecuted([&cut_count] { ++cut_count; });
    paste.setOnExecuted([&pasted] { ++pasted; });
    shortcuts.bind({Key::a, KeyModifier::control}, select_all);
    shortcuts.bind({Key::c, KeyModifier::control}, copy);
    shortcuts.bind({Key::x, KeyModifier::control}, cut);
    shortcuts.bind({Key::v, KeyModifier::control}, paste);

    CHECK(shortcuts.resolve(KeyEvent{Key::a, true, KeyModifier::control}).get() ==
          &select_all);
    CHECK(shortcuts.dispatch(KeyEvent{Key::c, true, KeyModifier::control}));
    CHECK(shortcuts.dispatch(KeyEvent{Key::x, true, KeyModifier::control}));
    CHECK(shortcuts.dispatch(KeyEvent{Key::v, true, KeyModifier::control}));
    CHECK(selected == 0);
    CHECK(copied == 1);
    CHECK(cut_count == 1);
    CHECK(pasted == 1);
}

TEST_CASE("ShortcutMap resolves bindings without entering application callbacks") {
    Command command{"Deferred"};
    ShortcutMap shortcuts;
    int executions = 0;
    command.setOnExecuted([&executions] { ++executions; });

    const Shortcut gesture{Key::f8, KeyModifier::control};
    shortcuts.bind(gesture, command);

    const Command::Reference resolved =
        shortcuts.resolve(KeyEvent{Key::f8, true, KeyModifier::control});

    CHECK(resolved.get() == &command);
    CHECK(executions == 0);

    /*
     * Resolution deliberately returns a copied lifetime-safe semantic reference rather than a pointer
     * into ShortcutMap storage. A transient routing scope may therefore be torn down or rebound before
     * the caller enters application code, which is useful for menu/overlay dismissal transactions.
     */
    shortcuts.clear();
    CHECK(resolved.get() == &command);
    CHECK(resolved.get()->execute());
    CHECK(executions == 1);
}

TEST_CASE("ShortcutMap resolved references expire safely before deferred execution") {
    ShortcutMap shortcuts;
    auto command = std::make_unique<Command>("Temporary");

    shortcuts.bind({Key::f9, KeyModifier::none}, *command);
    const Command::Reference resolved =
        shortcuts.resolve(KeyEvent{Key::f9, true, KeyModifier::none});

    CHECK(resolved.get() == command.get());
    command.reset();

    /*
     * Deferred execution must re-check semantic lifetime. Command::Reference carries no ownership and
     * therefore becomes empty instead of leaving a raw pointer dangling after application-side teardown.
     */
    CHECK(resolved.get() == nullptr);
}

TEST_CASE("ShortcutMap resolve keeps matching separate from command eligibility") {
    Command command{"Disabled"};
    ShortcutMap shortcuts;
    command.setEnabled(false);
    shortcuts.bind({Key::f5, KeyModifier::alt}, command);

    const Command::Reference resolved =
        shortcuts.resolve(KeyEvent{Key::f5, true, KeyModifier::alt});

    CHECK(resolved.get() == &command);
    CHECK(!resolved.get()->execute());
    CHECK(!shortcuts.resolve(KeyEvent{Key::f5, false, KeyModifier::alt}));
    CHECK(!shortcuts.resolve(KeyEvent{Key::f5, true, KeyModifier::shift}));
}

TEST_CASE("ShortcutMap dispatches exact bindings to Command") {
    Command command{"Help"};
    ShortcutMap shortcuts;
    int executions = 0;
    command.setOnExecuted([&executions] { ++executions; });

    shortcuts.bind({Key::f1, KeyModifier::none}, command);

    CHECK(shortcuts.dispatch(KeyEvent{Key::f1, true, KeyModifier::none}));
    CHECK(executions == 1);

    /*
     * Exact modifier matching is intentional. F1, Shift+F1 and Ctrl+F1 are independent semantic
     * gestures rather than a loose subset match in which a less-specific binding steals the event.
     */
    CHECK(!shortcuts.dispatch(KeyEvent{Key::f1, true, KeyModifier::shift}));
    CHECK(!shortcuts.dispatch(KeyEvent{Key::f1, false, KeyModifier::none}));
    CHECK(executions == 1);
}

TEST_CASE("ShortcutMap leaves disabled command gestures unaccepted") {
    Command command{"Refresh"};
    ShortcutMap shortcuts;
    int executions = 0;
    command.setOnExecuted([&executions] { ++executions; });
    command.setEnabled(false);

    shortcuts.bind({Key::f5, KeyModifier::none}, command);

    /*
     * Matching a key is not itself equivalent to handling the semantic action. Returning false here
     * lets the caller decide whether another scope or ordinary event routing should see the gesture
     * after the disabled Command declined execution.
     */
    CHECK(!shortcuts.dispatch(KeyEvent{Key::f5, true, KeyModifier::none}));
    CHECK(executions == 0);
}

TEST_CASE("ShortcutMap rebinding replaces the previous command deterministically") {
    Command first{"First"};
    Command second{"Second"};
    ShortcutMap shortcuts;
    int first_calls = 0;
    int second_calls = 0;

    first.setOnExecuted([&first_calls] { ++first_calls; });
    second.setOnExecuted([&second_calls] { ++second_calls; });

    const Shortcut gesture{Key::f2, KeyModifier::control};
    shortcuts.bind(gesture, first);
    shortcuts.bind(gesture, second);

    CHECK(shortcuts.dispatch(KeyEvent{Key::f2, true, KeyModifier::control}));
    CHECK(first_calls == 0);
    CHECK(second_calls == 1);
}

TEST_CASE("ShortcutMap safely ignores commands destroyed after binding") {
    ShortcutMap shortcuts;
    auto command = std::make_unique<Command>("Temporary");

    shortcuts.bind({Key::f3, KeyModifier::alt}, *command);
    command.reset();

    /*
     * The map stores Command::Reference rather than a raw pointer or shared ownership. Destruction of
     * the Component-owned semantic command therefore invalidates the binding without extending its
     * lifetime and without making dispatch dereference freed storage.
     */
    CHECK(!shortcuts.dispatch(KeyEvent{Key::f3, true, KeyModifier::alt}));
}

TEST_CASE("ShortcutMap tolerates command destruction during execution") {
    ShortcutMap shortcuts;
    auto command = std::make_unique<Command>("Self destruct");
    int executions = 0;

    command->setOnExecuted([&] {
        ++executions;
        command.reset();
    });
    shortcuts.bind({Key::f4, KeyModifier::none}, *command);

    CHECK(shortcuts.dispatch(KeyEvent{Key::f4, true, KeyModifier::none}));
    CHECK(executions == 1);

    // The expired reference is pruned on the next dispatch and cannot execute again.
    CHECK(!shortcuts.dispatch(KeyEvent{Key::f4, true, KeyModifier::none}));
    CHECK(executions == 1);
}

TEST_CASE("ShortcutMap unbind and clear remove semantic mappings") {
    Command first{"First"};
    Command second{"Second"};
    ShortcutMap shortcuts;

    const Shortcut first_gesture{Key::f6, KeyModifier::none};
    const Shortcut second_gesture{Key::f7, KeyModifier::none};
    shortcuts.bind(first_gesture, first);
    shortcuts.bind(second_gesture, second);

    CHECK(shortcuts.unbind(first_gesture));
    CHECK(!shortcuts.unbind(first_gesture));
    CHECK(!shortcuts.dispatch(KeyEvent{Key::f6, true, KeyModifier::none}));

    shortcuts.clear();
    CHECK(!shortcuts.dispatch(KeyEvent{Key::f7, true, KeyModifier::none}));
}

TEST_CASE("ShortcutMap rejects unknown key bindings") {
    Command command{"Invalid"};
    ShortcutMap shortcuts;
    bool threw = false;

    try {
        shortcuts.bind({Key::unknown, KeyModifier::control}, command);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
}

TEST_CASE("Shortcut display text formats every current function-key identity") {
    CHECK(shortcutDisplayText({Key::f1, KeyModifier::none}) == "F1");
    CHECK(shortcutDisplayText({Key::f5, KeyModifier::none}) == "F5");
    CHECK(shortcutDisplayText({Key::f10, KeyModifier::none}) == "F10");
    CHECK(shortcutDisplayText({Key::f12, KeyModifier::none}) == "F12");
}

TEST_CASE("Shortcut display text formats logical letters deterministically") {
    CHECK(shortcutDisplayText({Key::a, KeyModifier::control}) == "Ctrl+A");
    CHECK(shortcutDisplayText({Key::c, KeyModifier::control | KeyModifier::shift}) ==
          "Ctrl+Shift+C");
    CHECK(shortcutDisplayText({Key::x, KeyModifier::meta}) == "Meta+X");
    CHECK(shortcutDisplayText({Key::v, KeyModifier::control}) == "Ctrl+V");
}

TEST_CASE("Shortcut display text uses deterministic modifier order") {
    const KeyModifier modifiers = KeyModifier::shift | KeyModifier::meta |
                                  KeyModifier::control | KeyModifier::alt;

    /*
     * The input bit order is irrelevant. Presentation order is one toolkit-wide convention so a
     * Terminal menu and a Rendered menu cannot label the same Shortcut differently merely because
     * their renderers were implemented at different times.
     */
    CHECK(shortcutDisplayText({Key::f5, modifiers}) == "Ctrl+Alt+Shift+Meta+F5");
    CHECK(shortcutDisplayText({Key::escape, KeyModifier::control | KeyModifier::shift}) ==
          "Ctrl+Shift+Esc");
}

TEST_CASE("Shortcut display text covers current navigation keys and rejects unknown") {
    CHECK(shortcutDisplayText({Key::enter, KeyModifier::none}) == "Enter");
    CHECK(shortcutDisplayText({Key::delete_forward, KeyModifier::alt}) == "Alt+Delete");
    CHECK(shortcutDisplayText({Key::page_up, KeyModifier::control}) == "Ctrl+PageUp");
    CHECK(shortcutDisplayText({Key::left, KeyModifier::shift}) == "Shift+Left");

    /*
     * Key::unknown is deliberately not rendered as a modifier-only gesture. Shortcut::matches()
     * rejects unknown identity, so displaying "Ctrl" here would advertise an action that cannot fire.
     */
    CHECK(shortcutDisplayText({Key::unknown, KeyModifier::control}).empty());
}
