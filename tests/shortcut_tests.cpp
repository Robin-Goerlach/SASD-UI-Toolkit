#include "test_framework.hpp"

#include <sasd/ui/shortcut.hpp>

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
