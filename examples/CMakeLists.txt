add_executable(sasd_ui_terminal_demo
    terminal_form_demo.cpp
)

target_link_libraries(sasd_ui_terminal_demo PRIVATE SASD::UI::Terminal)
target_compile_features(sasd_ui_terminal_demo PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(sasd_ui_terminal_demo PRIVATE /W4 /permissive-)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_terminal_demo PRIVATE /WX)
    endif()
else()
    target_compile_options(sasd_ui_terminal_demo PRIVATE -Wall -Wextra -Wpedantic)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_terminal_demo PRIVATE -Werror)
    endif()
endif()

if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(sasd_ui_terminal_demo
        PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_terminal_demo PRIVATE -fsanitize=address,undefined)
endif()
