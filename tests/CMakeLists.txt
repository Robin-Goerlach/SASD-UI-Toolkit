add_executable(sasd_ui_tests
    test_main.cpp
    geometry_tests.cpp
    presentation_anchored_popup_layout_tests.cpp
    component_tests.cpp
    box_layout_tests.cpp
    button_tests.cpp
    button_command_tests.cpp
    shortcut_tests.cpp
    menu_model_tests.cpp
    menu_bar_model_tests.cpp
    menu_interaction_view_tests.cpp
    menu_navigation_tests.cpp
    list_model_tests.cpp
    list_view_tests.cpp
    check_box_tests.cpp
    combo_box_tests.cpp
    radio_button_tests.cpp
    radio_group_navigation_tests.cpp
    event_queue_tests.cpp
    event_dispatcher_tests.cpp
    focus_manager_tests.cpp
    focus_traversal_tests.cpp
    grid_layout_tests.cpp
    grid_layout_regression_tests.cpp
    form_layout_tests.cpp
    hit_test_tests.cpp
    layout_tests.cpp
    pointer_router_tests.cpp
    text_field_tests.cpp
    text_field_select_all_tests.cpp
    text_field_clipboard_tests.cpp
    utf8_tests.cpp
    widget_controls_tests.cpp
    visual_invalidation_tests.cpp
    presentation_coordinator_tests.cpp
    application_tests.cpp
    mock_backend_tests.cpp
)

target_link_libraries(sasd_ui_tests PRIVATE SASD::UI)
target_compile_features(sasd_ui_tests PRIVATE cxx_std_20)
target_include_directories(sasd_ui_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(sasd_ui_tests PRIVATE /W4 /permissive-)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_tests PRIVATE /WX)
    endif()
else()
    target_compile_options(sasd_ui_tests PRIVATE -Wall -Wextra -Wpedantic)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_tests PRIVATE -Werror)
    endif()
endif()

if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(sasd_ui_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_tests PRIVATE -fsanitize=address,undefined)
endif()

add_test(NAME sasd_ui_core_tests COMMAND sasd_ui_tests)


# Keep terminal-backend tests in a separate executable so the core test binary remains a pure
# platform-neutral contract test. CTest still runs both suites in every CI job.
add_executable(sasd_ui_terminal_tests
    test_main.cpp
    terminal_ansi_frame_encoder_tests.cpp
    terminal_ansi_input_decoder_tests.cpp
    terminal_backend_tests.cpp
    terminal_event_pump_tests.cpp
    terminal_screen_buffer_tests.cpp
    terminal_measurement_context_tests.cpp
    terminal_box_layout_tests.cpp
    terminal_grid_layout_tests.cpp
    terminal_stack_layout_tests.cpp
    terminal_menu_presentation_tests.cpp
    terminal_menu_placement_tests.cpp
    terminal_menu_directional_presentation_tests.cpp
    terminal_menu_frame_builder_tests.cpp
    terminal_menu_hit_test_tests.cpp
    terminal_menu_pointer_interaction_tests.cpp
    terminal_menu_pointer_motion_tests.cpp
    terminal_menu_pointer_intent_tests.cpp
    terminal_menu_pointer_deferral_interaction_tests.cpp
    terminal_menu_hover_interaction_tests.cpp
    terminal_menu_close_interaction_tests.cpp
    terminal_button_tests.cpp
    terminal_check_box_tests.cpp
    terminal_combo_box_tests.cpp
    terminal_combo_box_popup_presentation_tests.cpp
    terminal_combo_box_popup_hit_test_tests.cpp
    terminal_combo_box_popup_pointer_interaction_tests.cpp
    terminal_radio_button_tests.cpp
    terminal_text_field_tests.cpp
    terminal_text_field_selection_tests.cpp
    terminal_text_field_pointer_selection_tests.cpp
    terminal_text_field_triple_click_tests.cpp
    terminal_text_field_word_drag_tests.cpp
    terminal_text_metrics_tests.cpp
    terminal_presentation_sink_tests.cpp
    terminal_list_view_presentation_tests.cpp
    terminal_session_tests.cpp
)

target_link_libraries(sasd_ui_terminal_tests PRIVATE SASD::UI::Terminal)
target_compile_features(sasd_ui_terminal_tests PRIVATE cxx_std_20)
target_include_directories(sasd_ui_terminal_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(sasd_ui_terminal_tests PRIVATE /W4 /permissive-)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_terminal_tests PRIVATE /WX)
    endif()
else()
    target_compile_options(sasd_ui_terminal_tests PRIVATE -Wall -Wextra -Wpedantic)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_terminal_tests PRIVATE -Werror)
    endif()
endif()

if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(sasd_ui_terminal_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_terminal_tests PRIVATE -fsanitize=address,undefined)
endif()

add_test(NAME sasd_ui_terminal_tests COMMAND sasd_ui_terminal_tests)



# M3 rendered-command tests stay independent from SDL/window-system availability. They validate the
# deterministic presentation intermediate representation on every compiler/OS in the normal matrix.
add_executable(sasd_ui_rendered_tests
    test_main.cpp
    rendered_display_list_tests.cpp
    rendered_list_view_presentation_tests.cpp
    rendered_display_list_executor_tests.cpp
    rendered_menu_bar_presentation_tests.cpp
    rendered_menu_frame_presentation_tests.cpp
    rendered_menu_pointer_interaction_tests.cpp
    rendered_menu_presentation_tests.cpp
    rendered_presentation_sink_tests.cpp
    rendered_m4_layout_tests.cpp
    rendered_check_box_tests.cpp
    rendered_combo_box_tests.cpp
    rendered_combo_box_popup_presentation_tests.cpp
    rendered_combo_box_popup_pointer_interaction_tests.cpp
    rendered_radio_button_tests.cpp
    rendered_text_field_hit_test_tests.cpp
    rendered_text_field_scalar_drag_tests.cpp
    rendered_text_field_selection_tests.cpp
    rendered_text_field_shift_click_tests.cpp
)

target_link_libraries(sasd_ui_rendered_tests PRIVATE SASD::UI::Rendered)
target_compile_features(sasd_ui_rendered_tests PRIVATE cxx_std_20)
target_include_directories(sasd_ui_rendered_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(sasd_ui_rendered_tests PRIVATE /W4 /permissive-)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_rendered_tests PRIVATE /WX)
    endif()
else()
    target_compile_options(sasd_ui_rendered_tests PRIVATE -Wall -Wextra -Wpedantic)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_rendered_tests PRIVATE -Werror)
    endif()
endif()

if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(sasd_ui_rendered_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_rendered_tests PRIVATE -fsanitize=address,undefined)
endif()

add_test(NAME sasd_ui_rendered_tests COMMAND sasd_ui_rendered_tests)

# Compile/run the first real SDL3 + SDL_ttf adapter only when explicitly requested and when a test
# font is supplied. The normal Core/Terminal/Rendered test matrix therefore remains dependency-free,
# while the dedicated adapter CI job exercises actual upstream SDL code headlessly.
if(SASD_UI_BUILD_SDL3_ADAPTER AND NOT SASD_UI_SDL3_TEST_FONT STREQUAL "")
    add_executable(sasd_ui_sdl3_tests
        test_main.cpp
        rendered_sdl3_software_device_tests.cpp
        rendered_sdl3_window_backend_tests.cpp
        rendered_sdl3_text_field_word_drag_tests.cpp
    )

    target_link_libraries(sasd_ui_sdl3_tests
        PRIVATE
            SASD::UI::Rendered::SDL3
            # The adapter tests intentionally inject SDL_Event values through SDL's native queue.
            # Keep that native test dependency local instead of making SDL3 a PUBLIC dependency of
            # the experimental adapter target merely to expose SDL headers to tests.
            SDL3::SDL3
    )
    target_compile_features(sasd_ui_sdl3_tests PRIVATE cxx_std_20)
    target_include_directories(sasd_ui_sdl3_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Pass a configured path instead of embedding/distributing a font in this repository. Font
    # licensing remains the responsibility of the environment that enables the adapter tests.
    target_compile_definitions(sasd_ui_sdl3_tests
        PRIVATE SASD_UI_SDL3_TEST_FONT_PATH="${SASD_UI_SDL3_TEST_FONT}"
    )

    if(MSVC)
        target_compile_options(sasd_ui_sdl3_tests PRIVATE /W4 /permissive-)
        if(SASD_UI_WARNINGS_AS_ERRORS)
            target_compile_options(sasd_ui_sdl3_tests PRIVATE /WX)
        endif()
    else()
        target_compile_options(sasd_ui_sdl3_tests PRIVATE -Wall -Wextra -Wpedantic)
        if(SASD_UI_WARNINGS_AS_ERRORS)
            target_compile_options(sasd_ui_sdl3_tests PRIVATE -Werror)
        endif()
    endif()

    if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
        target_compile_options(sasd_ui_sdl3_tests
            PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(sasd_ui_sdl3_tests PRIVATE -fsanitize=address,undefined)
    endif()

    add_test(NAME sasd_ui_sdl3_tests COMMAND sasd_ui_sdl3_tests)

    # The window-backed tests create an actual SDL_Window/SDL_Renderer. SDL's offscreen video driver
    # gives CI the same lifecycle/event code without requiring an X11/Wayland server.
    set_tests_properties(sasd_ui_sdl3_tests PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
    )
endif()

# Exercise the real POSIX terminal adapter against a kernel pseudo-terminal. This is intentionally a
# separate process-level smoke test: it mutates stdin/stdout termios state and forks a child, concerns
# that should not leak into the ordinary deterministic unit-test executable.
if(UNIX AND NOT WIN32)
    add_executable(sasd_ui_posix_pty_smoke_tests
        terminal_posix_pty_smoke_tests.cpp
    )

    target_link_libraries(sasd_ui_posix_pty_smoke_tests PRIVATE SASD::UI::Terminal)
    target_compile_features(sasd_ui_posix_pty_smoke_tests PRIVATE cxx_std_20)

    # glibc exposes posix_openpt/grantpt/unlockpt/ptsname under the XSI feature set. The definition is
    # harmless on macOS and keeps the test source free of Linux-only openpty/libutil dependencies.
    target_compile_definitions(sasd_ui_posix_pty_smoke_tests PRIVATE _XOPEN_SOURCE=600)

    target_compile_options(sasd_ui_posix_pty_smoke_tests PRIVATE -Wall -Wextra -Wpedantic)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_posix_pty_smoke_tests PRIVATE -Werror)
    endif()

    if(SASD_UI_ENABLE_SANITIZERS)
        target_compile_options(sasd_ui_posix_pty_smoke_tests
            PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(sasd_ui_posix_pty_smoke_tests PRIVATE -fsanitize=address,undefined)
    endif()

    add_test(NAME sasd_ui_posix_pty_smoke_tests COMMAND sasd_ui_posix_pty_smoke_tests)

    # A PTY regression must fail fast instead of hanging the complete CI worker if a raw-mode/read
    # invariant breaks. Normal execution is far below this limit.
    set_tests_properties(sasd_ui_posix_pty_smoke_tests PROPERTIES TIMEOUT 10)
endif()


# Exercise the real Windows terminal adapter against a Windows Pseudoconsole (ConPTY). Like the
# POSIX PTY test above, this is a separate process-level smoke test because it changes native console
# modes/code pages and launches a child attached to a real kernel terminal endpoint.
if(WIN32)
    add_executable(sasd_ui_windows_conpty_smoke_tests
        terminal_windows_conpty_smoke_tests.cpp
    )

    target_link_libraries(sasd_ui_windows_conpty_smoke_tests PRIVATE SASD::UI::Terminal)
    target_compile_features(sasd_ui_windows_conpty_smoke_tests PRIVATE cxx_std_20)

    # CreatePseudoConsole/PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE are Windows 10 APIs.
    target_compile_definitions(sasd_ui_windows_conpty_smoke_tests
        PRIVATE _WIN32_WINNT=0x0A00 NOMINMAX)

    target_compile_options(sasd_ui_windows_conpty_smoke_tests PRIVATE /W4 /permissive-)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_windows_conpty_smoke_tests PRIVATE /WX)
    endif()

    add_test(NAME sasd_ui_windows_conpty_smoke_tests
             COMMAND sasd_ui_windows_conpty_smoke_tests)

    # ConPTY teardown is process/pipe based. A regression must fail the test rather than hang the
    # hosted Windows runner indefinitely.
    set_tests_properties(sasd_ui_windows_conpty_smoke_tests PROPERTIES TIMEOUT 15)
endif()
