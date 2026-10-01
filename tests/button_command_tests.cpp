#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/command.hpp>

#include <memory>

using namespace sasd::ui;

TEST_CASE("Button Command binding applies initial semantic state") {
    Command command{"Save"};
    command.setEnabled(false);

    Button button{"Local"};
    button.bindCommand(command);

    CHECK(button.boundCommand() == &command);
    CHECK(button.text() == "Save");
    CHECK(!button.isEnabled());
}

TEST_CASE("Button Command binding follows future text and enabled changes") {
    Command command{"Save"};
    Button button;
    button.bindCommand(command);

    command.setText("Save as");
    CHECK(button.text() == "Save as");

    command.setEnabled(false);
    CHECK(!button.isEnabled());

    command.setEnabled(true);
    CHECK(button.isEnabled());
}

TEST_CASE("Bound Button delegates activation and restores local callback after unbind") {
    Command command{"Run"};
    int command_calls = 0;
    command.setOnExecuted([&command_calls] { ++command_calls; });

    Button button{"Local"};
    int local_calls = 0;
    button.setOnActivated([&local_calls] { ++local_calls; });

    button.bindCommand(command);
    CHECK(button.activate());
    CHECK(command_calls == 1);
    CHECK(local_calls == 0);

    /*
     * The local callback is retained while Command owns activation semantics. Unbinding changes the
     * semantic target, not the Button's previously configured local behavior.
     */
    button.unbindCommand();
    CHECK(button.boundCommand() == nullptr);
    CHECK(button.activate());
    CHECK(command_calls == 1);
    CHECK(local_calls == 1);
}

TEST_CASE("Rebinding Button disconnects the previous Command") {
    Command first{"First"};
    Command second{"Second"};
    Button button;

    button.bindCommand(first);
    button.bindCommand(second);

    CHECK(button.boundCommand() == &second);
    CHECK(button.text() == "Second");

    first.setText("Stale first");
    first.setEnabled(false);

    CHECK(button.text() == "Second");
    CHECK(button.isEnabled());

    second.setText("Current second");
    CHECK(button.text() == "Current second");
}

TEST_CASE("Expired Button Command binding rejects activation without dangling access") {
    Button button{"Local fallback must stay suppressed"};
    int local_calls = 0;
    button.setOnActivated([&local_calls] { ++local_calls; });

    {
        auto command = std::make_unique<Command>("Temporary");
        button.bindCommand(*command);
        CHECK(button.boundCommand() == command.get());
    }

    /*
     * Expiration is deliberately distinct from explicit unbinding. Falling through to the local
     * callback here would silently change application semantics merely because an owned Command was
     * destroyed. The binding therefore becomes non-invokable until client code rebinds or unbinds it.
     */
    CHECK(button.boundCommand() == nullptr);
    CHECK(!button.activate());
    CHECK(local_calls == 0);

    button.unbindCommand();
    CHECK(button.activate());
    CHECK(local_calls == 1);
}

TEST_CASE("Button binding survives Command destruction from an earlier state observer") {
    auto command = std::make_unique<Command>("Before");

    /*
     * Register the destructive observer before the Button. Command notification order therefore
     * destroys the semantic object before the Button's own synchronization callback gets its turn.
     * The in-flight observer snapshot remains alive, but Command::Reference must already resolve to
     * nullptr so the later callback performs no stale dereference.
     */
    auto destroyer = command->observeState(
        [&](Command::StateChange change) {
            if (change == Command::StateChange::text) {
                command.reset();
            }
        });

    Button button;
    button.bindCommand(*command);
    CHECK(button.text() == "Before");

    Command* raw = command.get();
    raw->setText("After");

    CHECK(command == nullptr);
    CHECK(button.boundCommand() == nullptr);
    CHECK(button.text() == "Before");
    CHECK(!button.activate());

    /*
     * Once the in-flight notification snapshot is released, neither the observer registry nor its
     * slots are kept alive by the non-owning tokens. Destruction therefore disconnects both observers.
     */
    CHECK(!destroyer.connected());
}
