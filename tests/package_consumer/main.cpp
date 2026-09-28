#include <sasd/ui/button.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/rendered/rendered_presentation_sink.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>
#include <sasd/ui/window.hpp>

#include <cstdlib>

int main() {
    /*
     * This executable is deliberately tiny: its purpose is not another widget unit test. It proves
     * that an external project can discover the installed package, include public Core/Terminal/
     * Rendered headers, and link all exported targets without relying on the source-tree layout.
     */
    sasd::ui::Button button{"Package smoke"};
    sasd::ui::terminal::ScreenBuffer buffer{{2, 1}};
    sasd::ui::rendered::DisplayList display_list;
    sasd::ui::rendered::RenderedPresentationSink rendered_sink{display_list};
    sasd::ui::Window rendered_window;

    buffer.set({0, 0}, {U'X'});

    /*
     * Exercise a non-inline Rendered symbol from the installed library, not just DisplayList headers.
     * Arranging the Window requests a subtree refresh, so synchronizing it records one root clear.
     */
    rendered_window.arrange({0, 0, 32, 16});
    const auto rendered_result = rendered_sink.synchronize(rendered_window);

    const bool core_linked = button.text() == "Package smoke";
    const bool terminal_linked = buffer.at({0, 0}).code_point == U'X';
    const bool rendered_linked =
        rendered_result == sasd::ui::PresentationUpdateResult::synchronized &&
        display_list.size() == 1;

    return core_linked && terminal_linked && rendered_linked ? EXIT_SUCCESS : EXIT_FAILURE;
}
