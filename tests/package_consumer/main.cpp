#include <sasd/ui/button.hpp>
#include <sasd/ui/terminal/screen_buffer.hpp>

#include <cstdlib>

int main() {
    /*
     * This executable is deliberately tiny: its purpose is not another widget unit test. It proves
     * that an external project can discover the installed package, include public Core/Terminal
     * headers, and link both exported targets without relying on the source-tree layout.
     */
    sasd::ui::Button button{"Package smoke"};
    sasd::ui::terminal::ScreenBuffer buffer{{2, 1}};

    buffer.set({0, 0}, {U'X'});

    const bool core_linked = button.text() == "Package smoke";
    const bool terminal_linked = buffer.at({0, 0}).code_point == U'X';

    return core_linked && terminal_linked ? EXIT_SUCCESS : EXIT_FAILURE;
}
