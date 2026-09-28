#include <sasd/ui/button.hpp>
#include <sasd/ui/rendered/display_list.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>

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

    buffer.set({0, 0}, {U'X'});
    display_list.drawText({0, 0}, "Rendered package smoke");

    const bool core_linked = button.text() == "Package smoke";
    const bool terminal_linked = buffer.at({0, 0}).code_point == U'X';
    const bool rendered_linked = display_list.size() == 1;

    return core_linked && terminal_linked && rendered_linked ? EXIT_SUCCESS : EXIT_FAILURE;
}
