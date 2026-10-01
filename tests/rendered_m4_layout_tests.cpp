#include "test_framework.hpp"

#include <sasd/ui/container.hpp>
#include <sasd/ui/form_layout.hpp>
#include <sasd/ui/grid_layout.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/stack_layout.hpp>
#include <sasd/ui/window.hpp>

#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

namespace {

/**
 * Deliberately unknown Container subtype used to guard the rendered sink's fail-safe boundary.
 *
 * A future composite control may derive from Container while still owning visual chrome of its own.
 * Treating every Container subtype as presentation-free would acknowledge such a control before the
 * rendered backend actually knows how to draw it. This probe therefore must remain deferred.
 */
class UnknownComposite final : public Container {};

} // namespace

TEST_CASE("Rendered presentation recognizes M4 layouts as structural nodes") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    GridLayout grid{2};
    const auto grid_pass = PresentationCoordinator::synchronize(grid, sink);
    CHECK(grid_pass.complete());
    CHECK(grid_pass.deferred == 0);
    CHECK(display.empty());

    FormLayout form;
    const auto form_pass = PresentationCoordinator::synchronize(form, sink);
    CHECK(form_pass.complete());
    CHECK(form_pass.deferred == 0);
    CHECK(display.empty());

    StackLayout stack;
    const auto stack_pass = PresentationCoordinator::synchronize(stack, sink);
    CHECK(stack_pass.complete());
    CHECK(stack_pass.deferred == 0);
    CHECK(display.empty());

    /*
     * Structural acknowledgement must not manufacture drawing commands. The layouts contribute
     * geometry and traversal policy; their visible children remain responsible for actual rendered
     * content. This keeps Core layout semantics independent from DisplayList primitives.
     */
}

TEST_CASE("Rendered presentation keeps unknown Container subtypes deferred") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};
    UnknownComposite composite;

    const auto pass = PresentationCoordinator::synchronize(composite, sink);

    CHECK(!pass.complete());
    CHECK(pass.deferred == 1);
    CHECK(composite.isVisualUpdatePending());
    CHECK(display.empty());

    /*
     * This is an architectural regression guard, not merely a type-list test. If the rendered sink
     * is ever simplified to accept every Container subclass, this test fails before a new composite
     * control can silently lose its pending presentation state.
     */
}

TEST_CASE("Rendered StackLayout presents later layers last") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 160, 80});

    auto& stack = window.emplace<StackLayout>();
    auto& bottom = stack.emplace<Label>("bottom");
    auto& top = stack.emplace<Label>("top");
    stack.arrange({10, 15, 80, 20});

    const auto pass = PresentationCoordinator::synchronize(window, sink);
    CHECK(pass.complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 5);

    /*
     * Window contributes the surface clear. StackLayout itself contributes no command. Both Labels
     * occupy the same absolute rectangle, and preorder traversal follows adoption order, so the top
     * layer's erase/text pair is appended after the bottom layer's pair.
     */
    CHECK(std::get<FillRectCommand>(commands[1]).bounds == Rect{10, 15, 80, 20});
    CHECK(std::get<DrawTextCommand>(commands[2]).text == "bottom");
    CHECK(std::get<FillRectCommand>(commands[3]).bounds == Rect{10, 15, 80, 20});
    CHECK(std::get<DrawTextCommand>(commands[4]).text == "top");

    CHECK(bottom.bounds() == Rect{0, 0, 80, 20});
    CHECK(top.bounds() == Rect{0, 0, 80, 20});
}

TEST_CASE("Hiding the top rendered StackLayout layer replays the revealed lower layer") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 160, 80});

    auto& stack = window.emplace<StackLayout>();
    auto& bottom = stack.emplace<Label>("bottom");
    auto& top = stack.emplace<Label>("top");
    stack.arrange({10, 15, 80, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    top.setVisible(false);

    const auto replay = PresentationCoordinator::synchronize(window, sink);
    CHECK(replay.complete());
    CHECK(replay.forced >= 1);

    const auto commands = display.commands();
    CHECK(commands.size() == 3);

    /*
     * Visibility is conservative presentation damage: the Window is cleared, then the still-visible
     * lower layer is replayed. PresentationCoordinator prunes the hidden top layer during this forced
     * traversal, so it cannot erase the freshly restored lower content again.
     */
    CHECK(std::get<FillRectCommand>(commands[0]).bounds == Rect{0, 0, 160, 80});
    CHECK(std::get<FillRectCommand>(commands[1]).bounds == Rect{10, 15, 80, 20});
    CHECK(std::get<DrawTextCommand>(commands[2]).text == "bottom");
    CHECK(!top.isVisualUpdatePending());
    CHECK(!bottom.isVisualUpdatePending());
}
