#include "test_framework.hpp"

#include <sasd/ui/button.hpp>
#include <sasd/ui/container.hpp>
#include <sasd/ui/focus_manager.hpp>
#include <sasd/ui/label.hpp>
#include <sasd/ui/presentation/presentation_coordinator.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/text_field.hpp>
#include <sasd/ui/window.hpp>

#include <limits>
#include <optional>
#include <string>
#include <variant>

using namespace sasd::ui;
using namespace sasd::ui::rendered;

TEST_CASE("RenderedPresentationSink builds deterministic Window and Label commands") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 320, 200});

    auto& panel = window.emplace<Container>();
    panel.arrange({10, 20, 200, 100});

    auto& label = panel.emplace<Label>("Hello");
    label.arrange({5, 6, 80, 20});

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(pass.complete());
    CHECK(display.size() == 3);

    const auto commands = display.commands();

    // Window geometry changed before the first pass, so subtree refresh starts from a clean surface.
    CHECK(std::get<FillRectCommand>(commands[0]) ==
          FillRectCommand{Rect{0, 0, 320, 200}, Color::black});

    // Label coordinates are resolved through the visual-parent chain: (10 + 5, 20 + 6).
    CHECK(std::get<FillRectCommand>(commands[1]) ==
          FillRectCommand{Rect{15, 26, 80, 20}, Color::black});

    const auto& text = std::get<DrawTextCommand>(commands[2]);
    CHECK(text.origin == Point{15, 26});
    CHECK(text.text == "Hello");
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{15, 26, 80, 20}});
}

TEST_CASE("RenderedPresentationSink keeps ordinary Label repaint incremental") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 300, 120});
    auto& label = window.emplace<Label>("long old value");
    label.arrange({20, 10, 120, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    label.setText("new");

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(display.size() == 2);

    /*
     * Label::invalidateVisual() also marks Window pending, but it is not a subtree refresh. The sink
     * therefore clears only the Label rectangle, not the entire Window and unrelated clean siblings.
     */
    CHECK(std::get<FillRectCommand>(display.commands()[0]).bounds ==
          Rect{20, 10, 120, 20});
    CHECK(std::get<DrawTextCommand>(display.commands()[1]).text == "new");
}

TEST_CASE("RenderedPresentationSink rebuilds complete surface after geometry damage") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 300, 120});

    auto& first = window.emplace<Label>("first");
    first.arrange({10, 10, 80, 20});

    auto& second = window.emplace<Label>("second");
    second.arrange({10, 40, 80, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    first.arrange({30, 10, 80, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 5);

    // Root clear followed by both Labels proves the clean sibling was replayed by subtree refresh.
    CHECK(std::get<FillRectCommand>(commands[0]).bounds == Rect{0, 0, 300, 120});
    CHECK(std::get<FillRectCommand>(commands[1]).bounds == Rect{30, 10, 80, 20});
    CHECK(std::get<DrawTextCommand>(commands[2]).text == "first");
    CHECK(std::get<FillRectCommand>(commands[3]).bounds == Rect{10, 40, 80, 20});
    CHECK(std::get<DrawTextCommand>(commands[4]).text == "second");
}

TEST_CASE("RenderedPresentationSink renders Button chrome without mutating semantic style") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};
    FocusManager focus;

    Window window;
    window.arrange({0, 0, 200, 80});

    auto& button = window.emplace<Button>("Run");
    button.arrange({10, 10, 80, 30});

    TextStyle base;
    base.foreground = Color::bright_green;
    base.bold = true;
    button.setTextStyle(base);

    CHECK(focus.requestFocus(button));
    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    const auto commands = display.commands();
    CHECK(commands.size() == 4);

    CHECK(std::get<FillRectCommand>(commands[1]).bounds == Rect{10, 10, 80, 30});
    CHECK(std::get<StrokeRectCommand>(commands[2]) ==
          StrokeRectCommand{Rect{10, 10, 80, 30}, Color::bright_green, 1});

    const auto& text = std::get<DrawTextCommand>(commands[3]);
    CHECK(text.origin == Point{11, 11});
    CHECK(text.text == "Run");
    CHECK(text.style.bold);
    CHECK(text.style.inverse);
    CHECK(text.clip_bounds == std::optional<Rect>{Rect{11, 11, 78, 28}});

    // Focus is a presentation overlay. User-supplied semantic style remains unchanged.
    CHECK(button.textStyle() == base);
}

TEST_CASE("RenderedPresentationSink erases a hidden Label without drawing stale text") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 200, 80});
    auto& label = window.emplace<Label>("visible");
    label.arrange({15, 20, 100, 20});

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());

    display.clear();
    label.setVisible(false);

    CHECK(PresentationCoordinator::synchronize(window, sink).complete());
    CHECK(display.size() == 1);
    CHECK(std::get<FillRectCommand>(display.commands()[0]) ==
          FillRectCommand{Rect{15, 20, 100, 20}, Color::black});
}

TEST_CASE("RenderedPresentationSink conservatively defers visible TextField") {
    DisplayList display;
    RenderedPresentationSink sink{display, Color::black};

    Window window;
    window.arrange({0, 0, 240, 100});
    auto& field = window.emplace<TextField>("Robin");
    field.arrange({10, 10, 160, 30});

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(!pass.complete());
    CHECK(pass.deferred == 1);

    /*
     * The root surface can still be cleared safely, but no partial TextField commands are emitted.
     * The field remains pending until rendered font metrics/caret geometry are implemented.
     */
    CHECK(display.size() == 1);
    CHECK(std::get<FillRectCommand>(display.commands()[0]).bounds ==
          Rect{0, 0, 240, 100});
    CHECK(field.isVisualUpdatePending());
}

TEST_CASE("RenderedPresentationSink defers unrepresentable accumulated coordinates") {
    DisplayList display;
    RenderedPresentationSink sink{display};

    Window window;
    window.arrange({std::numeric_limits<Coordinate>::max(), 0, 10, 10});

    auto& label = window.emplace<Label>("overflow");
    label.arrange({1, 0, 5, 5});

    const auto pass = PresentationCoordinator::synchronize(window, sink);

    CHECK(!pass.complete());
    CHECK(label.isVisualUpdatePending());
}
