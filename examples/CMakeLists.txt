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

add_executable(sasd_ui_component_metadata_demo
    component_metadata_demo.cpp
)

target_link_libraries(sasd_ui_component_metadata_demo PRIVATE SASD::UI)
target_compile_features(sasd_ui_component_metadata_demo PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(sasd_ui_component_metadata_demo PRIVATE /W4 /permissive-)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_component_metadata_demo PRIVATE /WX)
    endif()
else()
    target_compile_options(sasd_ui_component_metadata_demo PRIVATE -Wall -Wextra -Wpedantic)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_component_metadata_demo PRIVATE -Werror)
    endif()
endif()

if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(sasd_ui_component_metadata_demo
        PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_component_metadata_demo PRIVATE -fsanitize=address,undefined)
endif()

if(WIN32)
    add_executable(sasd_ui_win32_file_dialog_demo native_win32_file_dialog_demo.cpp)
    target_link_libraries(sasd_ui_win32_file_dialog_demo PRIVATE SASD::UI::Native::Win32)
    target_compile_features(sasd_ui_win32_file_dialog_demo PRIVATE cxx_std_20)
    target_compile_definitions(sasd_ui_win32_file_dialog_demo PRIVATE UNICODE _UNICODE NOMINMAX
        WIN32_LEAN_AND_MEAN)
    if(MSVC)
        target_compile_options(sasd_ui_win32_file_dialog_demo PRIVATE /W4 /permissive-)
        if(SASD_UI_WARNINGS_AS_ERRORS)
            target_compile_options(sasd_ui_win32_file_dialog_demo PRIVATE /WX)
        endif()
    endif()
endif()

if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(sasd_ui_terminal_demo
        PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_terminal_demo PRIVATE -fsanitize=address,undefined)
endif()


# The M3 desktop demo remains tied to the experimental build-tree SDL3 adapter. It is not part of
# the installed package yet because SDL dependency/runtime/font packaging is still being validated.
if(SASD_UI_BUILD_SDL3_ADAPTER)
    add_executable(sasd_ui_sdl3_demo
        rendered_sdl3_form_demo.cpp
    )

    target_link_libraries(sasd_ui_sdl3_demo PRIVATE SASD::UI::Rendered::SDL3)
    target_compile_features(sasd_ui_sdl3_demo PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(sasd_ui_sdl3_demo PRIVATE /W4 /permissive-)
        if(SASD_UI_WARNINGS_AS_ERRORS)
            target_compile_options(sasd_ui_sdl3_demo PRIVATE /WX)
        endif()
    else()
        target_compile_options(sasd_ui_sdl3_demo PRIVATE -Wall -Wextra -Wpedantic)
        if(SASD_UI_WARNINGS_AS_ERRORS)
            target_compile_options(sasd_ui_sdl3_demo PRIVATE -Werror)
        endif()
    endif()

    if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
        target_compile_options(sasd_ui_sdl3_demo
            PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(sasd_ui_sdl3_demo PRIVATE -fsanitize=address,undefined)
    endif()
endif()

if(SASD_UI_BUILD_GTK_ADAPTER)
    add_executable(sasd_ui_gtk_demo native_gtk_form_demo.cpp)
    target_link_libraries(sasd_ui_gtk_demo PRIVATE SASD::UI::Native::GTK)
    target_compile_features(sasd_ui_gtk_demo PRIVATE cxx_std_20)
    if(MSVC)
        target_compile_options(sasd_ui_gtk_demo PRIVATE /W4 /permissive-)
    else()
        target_compile_options(sasd_ui_gtk_demo PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(APPLE AND SASD_UI_BUILD_APPKIT_ADAPTER)
    add_executable(sasd_ui_appkit_demo native_appkit_form_demo.mm)
    target_link_libraries(sasd_ui_appkit_demo PRIVATE SASD::UI::Native::AppKit)
    target_compile_features(sasd_ui_appkit_demo PRIVATE cxx_std_20)
    target_compile_options(sasd_ui_appkit_demo PRIVATE -Wall -Wextra -Wpedantic -fobjc-arc)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_appkit_demo PRIVATE -Werror)
    endif()
endif()
