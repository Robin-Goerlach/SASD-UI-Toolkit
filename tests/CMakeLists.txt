add_executable(sasd_ui_tests
    test_main.cpp
    geometry_tests.cpp
    component_tests.cpp
    box_layout_tests.cpp
    button_tests.cpp
    event_queue_tests.cpp
    event_dispatcher_tests.cpp
    focus_manager_tests.cpp
    focus_traversal_tests.cpp
    layout_tests.cpp
    text_field_tests.cpp
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
    terminal_button_tests.cpp
    terminal_text_field_tests.cpp
    terminal_text_metrics_tests.cpp
    terminal_presentation_sink_tests.cpp
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
