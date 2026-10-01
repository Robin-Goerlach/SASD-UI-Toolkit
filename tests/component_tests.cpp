#include "test_framework.hpp"

#include <sasd/ui/command.hpp>
#include <sasd/ui/container.hpp>

#include <memory>
#include <stdexcept>
#include <utility>

using namespace sasd::ui;

namespace {

class NonVisualComponent final : public Component {};

class LifetimeProbe final : public Component {
public:
    explicit LifetimeProbe(int& destruction_count) : destruction_count_{destruction_count} {}

    ~LifetimeProbe() override {
        ++destruction_count_;
    }

private:
    int& destruction_count_;
};

} // namespace

TEST_CASE("Container owns components and assigns visual parents only to widgets") {
    Container root;

    auto& widget = root.emplace<Widget>();
    auto& service = root.emplace<NonVisualComponent>();

    CHECK(root.componentCount() == 2);
    CHECK(root.childCount() == 1);
    CHECK(&root.childAt(0) == &widget);
    CHECK(widget.owner() == &root);
    CHECK(widget.parent() == &root);
    CHECK(service.owner() == &root);
    CHECK(root.componentAt(0).owner() == &root);
}

TEST_CASE("Container exposes visual children separately from owned components") {
    Container root;

    auto& first_service = root.emplace<NonVisualComponent>();
    auto& first_widget = root.emplace<Widget>();
    auto& second_service = root.emplace<NonVisualComponent>();
    auto& nested_container = root.emplace<Container>();
    auto& second_widget = root.emplace<Widget>();

    CHECK(root.componentCount() == 5);
    CHECK(root.childCount() == 3);
    CHECK(&root.childAt(0) == &first_widget);
    CHECK(&root.childAt(1) == &nested_container);
    CHECK(&root.childAt(2) == &second_widget);
    CHECK(first_service.owner() == &root);
    CHECK(second_service.owner() == &root);
    CHECK(nested_container.owner() == &root);
    CHECK(nested_container.parent() == &root);

    const Container& const_root = root;
    CHECK(&const_root.childAt(1) == &nested_container);

    bool threw = false;
    try {
        (void)root.childAt(3);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

TEST_CASE("Container release clears ownership and visual parenting") {
    Container root;

    auto& widget = root.emplace<Widget>();
    auto& service = root.emplace<NonVisualComponent>();

    auto released_widget = root.release(widget);
    CHECK(released_widget.get() == &widget);
    CHECK(widget.owner() == nullptr);
    CHECK(widget.parent() == nullptr);
    CHECK(root.componentCount() == 1);
    CHECK(root.childCount() == 0);

    auto released_service = root.release(service);
    CHECK(released_service.get() == &service);
    CHECK(service.owner() == nullptr);
    CHECK(root.componentCount() == 0);
}

TEST_CASE("Released components can be transferred between containers") {
    Container first;
    Container second;

    auto& widget = first.emplace<Widget>();
    auto released = first.release(widget);

    CHECK(released != nullptr);
    CHECK(widget.owner() == nullptr);
    CHECK(widget.parent() == nullptr);

    second.adopt(std::move(released));

    CHECK(widget.owner() == &second);
    CHECK(widget.parent() == &second);
    CHECK(second.componentCount() == 1);
    CHECK(second.childCount() == 1);
    CHECK(&second.childAt(0) == &widget);
}

TEST_CASE("Container release leaves unrelated components untouched") {
    Container first;
    Container second;

    auto& first_widget = first.emplace<Widget>();
    auto& second_widget = second.emplace<Widget>();

    auto released = first.release(second_widget);

    CHECK(released == nullptr);
    CHECK(first_widget.owner() == &first);
    CHECK(first_widget.parent() == &first);
    CHECK(second_widget.owner() == &second);
    CHECK(second_widget.parent() == &second);
    CHECK(first.componentCount() == 1);
    CHECK(second.componentCount() == 1);
}

TEST_CASE("Released component lifetime is controlled by returned unique ownership") {
    int destruction_count = 0;
    std::unique_ptr<Component> released;

    {
        Container root;
        auto& probe = root.emplace<LifetimeProbe>(destruction_count);

        released = root.release(probe);
        CHECK(released != nullptr);
        CHECK(destruction_count == 0);
    }

    // Destruction of the old container must not destroy an object that was explicitly released.
    CHECK(destruction_count == 0);
    released.reset();
    CHECK(destruction_count == 1);
}

TEST_CASE("Container destruction releases all owned component lifetimes exactly once") {
    int destruction_count = 0;

    {
        Container root;
        root.emplace<LifetimeProbe>(destruction_count);
        root.emplace<LifetimeProbe>(destruction_count);
        CHECK(destruction_count == 0);
    }

    CHECK(destruction_count == 2);
}

TEST_CASE("Command is an owned non-visual component") {
    Container root;
    auto& command = root.emplace<Command>("Save");

    /*
     * Command participates in the normal Component ownership graph, but it must not become a visual
     * child merely because a Container owns it. This is the architectural separation that later lets
     * multiple controls or menus refer to one semantic command without introducing backend objects.
     */
    CHECK(root.componentCount() == 1);
    CHECK(root.childCount() == 0);
    CHECK(command.owner() == &root);
    CHECK(command.text() == "Save");
}

TEST_CASE("Command enabled state gates synchronous execution") {
    Command command{"Refresh"};
    int execution_count = 0;
    command.setOnExecuted([&execution_count] { ++execution_count; });

    CHECK(command.isEnabled());
    CHECK(command.execute());
    CHECK(execution_count == 1);

    command.setEnabled(false);
    CHECK(!command.isEnabled());
    CHECK(!command.execute());
    CHECK(execution_count == 1);

    command.setEnabled(true);
    CHECK(command.execute());
    CHECK(execution_count == 2);
}

TEST_CASE("Command copies its handler before invoking mutable client code") {
    Command command{"Mutable handler"};
    int first_handler_calls = 0;
    int replacement_handler_calls = 0;

    command.setOnExecuted([&] {
        ++first_handler_calls;

        /*
         * Replacing the handler from inside itself is a compact lifetime regression. execute() must
         * invoke a local copy; otherwise assigning the member std::function here could invalidate the
         * callable object whose operator() is currently on the stack.
         */
        command.setOnExecuted([&replacement_handler_calls] { ++replacement_handler_calls; });
    });

    CHECK(command.execute());
    CHECK(first_handler_calls == 1);
    CHECK(replacement_handler_calls == 0);

    CHECK(command.execute());
    CHECK(first_handler_calls == 1);
    CHECK(replacement_handler_calls == 1);
}

TEST_CASE("Command text is semantic metadata and does not affect execution eligibility") {
    Command command;
    CHECK(command.text().empty());
    CHECK(command.isEnabled());

    command.setText("Open settings");
    CHECK(command.text() == "Open settings");

    /*
     * An enabled command with no installed observer still accepts execution. This intentionally
     * matches Button::activate(): eligibility is semantic state, not whether client code currently
     * happens to observe the accepted action.
     */
    CHECK(command.execute());
}

TEST_CASE("Command state observers receive only real semantic changes") {
    Command command{"Save"};
    int text_changes = 0;
    int enabled_changes = 0;

    auto subscription = command.observeState([&](Command::StateChange change) {
        if (change == Command::StateChange::text) {
            ++text_changes;
        } else if (change == Command::StateChange::enabled) {
            ++enabled_changes;
        }
    });

    CHECK(subscription.connected());

    /*
     * Establishing an observer is intentionally not an implicit snapshot. Bindings read current state
     * once when connecting and then consume only actual transitions from this notification channel.
     */
    CHECK(text_changes == 0);
    CHECK(enabled_changes == 0);

    command.setText("Save");
    command.setEnabled(true);
    CHECK(text_changes == 0);
    CHECK(enabled_changes == 0);

    command.setText("Save as");
    command.setEnabled(false);
    CHECK(text_changes == 1);
    CHECK(enabled_changes == 1);
}

TEST_CASE("Command subscription lifetime disconnects without owning the Command") {
    Command command{"Open"};
    int changes = 0;

    {
        auto subscription = command.observeState(
            [&changes](Command::StateChange) { ++changes; });
        CHECK(subscription.connected());

        command.setText("Open file");
        CHECK(changes == 1);
    }

    command.setText("Open folder");
    CHECK(changes == 1);

    auto subscription = command.observeState(
        [&changes](Command::StateChange) { ++changes; });
    subscription.reset();
    CHECK(!subscription.connected());
    command.setEnabled(false);
    CHECK(changes == 1);
}

TEST_CASE("Command subscription move assignment retires the previous connection") {
    Command first{"First"};
    Command second{"Second"};
    int first_changes = 0;
    int second_changes = 0;

    auto target = first.observeState(
        [&first_changes](Command::StateChange) { ++first_changes; });
    auto incoming = second.observeState(
        [&second_changes](Command::StateChange) { ++second_changes; });

    target = std::move(incoming);

    CHECK(target.connected());
    CHECK(!incoming.connected());

    first.setText("First changed");
    second.setText("Second changed");

    /*
     * Move-assignment must disconnect target's old slot before taking incoming's slot; otherwise the
     * first callback would remain live but unreachable by any subscription token.
     */
    CHECK(first_changes == 0);
    CHECK(second_changes == 1);
}

TEST_CASE("Command observer can disconnect a later observer during notification") {
    Command command{"Run"};
    int first_calls = 0;
    int second_calls = 0;

    Command::StateSubscription second;
    auto first = command.observeState([&](Command::StateChange) {
        ++first_calls;
        second.reset();
    });
    second = command.observeState(
        [&second_calls](Command::StateChange) { ++second_calls; });

    command.setEnabled(false);

    CHECK(first.connected());
    CHECK(first_calls == 1);
    CHECK(second_calls == 0);
    CHECK(!second.connected());
}

TEST_CASE("Command observer added during notification starts with the next change") {
    Command command{"Run"};
    int first_calls = 0;
    int late_calls = 0;
    Command::StateSubscription late;

    auto first = command.observeState([&](Command::StateChange) {
        ++first_calls;
        if (!late.connected()) {
            late = command.observeState(
                [&late_calls](Command::StateChange) { ++late_calls; });
        }
    });

    command.setEnabled(false);
    CHECK(first_calls == 1);
    CHECK(late_calls == 0);
    CHECK(first.connected());
    CHECK(late.connected());

    command.setEnabled(true);
    CHECK(first_calls == 2);
    CHECK(late_calls == 1);
}

TEST_CASE("Command state notification remains safe when a callback destroys the Command") {
    auto command = std::make_unique<Command>("Temporary");
    int calls = 0;

    auto subscription = command->observeState([&](Command::StateChange) {
        ++calls;

        /*
         * Notification keeps only shared observer-state needed for the in-flight pass and performs no
         * later access through this Command. Destroying the semantic object from client code therefore
         * cannot turn StateSubscription cleanup into a raw-pointer lifetime hazard.
         */
        command.reset();
    });

    command->setText("Destroy me");
    CHECK(command == nullptr);
    CHECK(calls == 1);

    // The token weakly references a slot owned by the destroyed Command and therefore expires safely.
    CHECK(!subscription.connected());
    subscription.reset();
}

TEST_CASE("Widget state is backend neutral") {
    Widget widget;

    CHECK(widget.isVisible());
    CHECK(widget.isEnabled());

    widget.setVisible(false);
    widget.setEnabled(false);
    widget.setBounds({1, 2, 30, 40});

    CHECK(!widget.isVisible());
    CHECK(!widget.isEnabled());
    CHECK(widget.bounds() == Rect{1, 2, 30, 40});
}

TEST_CASE("Container rejects null adoption") {
    Container root;
    bool threw = false;

    try {
        root.adopt(nullptr);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    CHECK(threw);
}
