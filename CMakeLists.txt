cmake_minimum_required(VERSION 3.24)

project(SASD_UI_Toolkit
    VERSION 0.1.0
    DESCRIPTION "Cross-platform C++ UI toolkit for desktop and terminal applications"
    LANGUAGES CXX)

option(SASD_UI_BUILD_TESTS "Build SASD UI Toolkit tests" ON)
option(SASD_UI_BUILD_EXAMPLES "Build SASD UI Toolkit example applications" ON)
option(SASD_UI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SASD_UI_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer where supported" OFF)

add_library(sasd_ui_core
    src/application.cpp
    src/box_layout.cpp
    src/button.cpp
    src/container.cpp
    src/event_dispatcher.cpp
    src/event_queue.cpp
    src/focus_manager.cpp
    src/focus_traversal.cpp
    src/label.cpp
    src/text_field.cpp
    src/utf8.cpp
    src/presentation_coordinator.cpp
    src/widget.cpp
    src/testing/mock_backend.cpp
)
add_library(SASD::UI ALIAS sasd_ui_core)

# The first M2 backend lives in a separately linkable target. Applications that only need the
# platform-neutral core do not acquire terminal-specific implementation code.
add_library(sasd_ui_terminal
    src/terminal/ansi_frame_encoder.cpp
    src/terminal/ansi_input_decoder.cpp
    src/terminal/screen_buffer.cpp
    src/terminal/terminal_backend.cpp
    src/terminal/terminal_event_pump.cpp
    src/terminal/terminal_session.cpp
    src/terminal/terminal_measurement_context.cpp
    src/terminal/terminal_presentation_sink.cpp
    src/terminal/text_metrics.cpp
)
# Native device implementations are intentionally selected inside the backend target so public
# terminal headers remain free of POSIX termios and Win32 HANDLE types.
if(WIN32)
    target_sources(sasd_ui_terminal PRIVATE src/terminal/windows_terminal_device.cpp)
else()
    target_sources(sasd_ui_terminal PRIVATE src/terminal/posix_terminal_device.cpp)
endif()

add_library(SASD::UI::Terminal ALIAS sasd_ui_terminal)
target_link_libraries(sasd_ui_terminal PUBLIC SASD::UI)

target_compile_features(sasd_ui_core PUBLIC cxx_std_20)
target_include_directories(sasd_ui_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(sasd_ui_terminal PUBLIC cxx_std_20)
target_include_directories(sasd_ui_terminal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(sasd_ui_core PRIVATE /W4 /permissive-)
    target_compile_options(sasd_ui_terminal PRIVATE /W4 /permissive-)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_core PRIVATE /WX)
        target_compile_options(sasd_ui_terminal PRIVATE /WX)
    endif()
else()
    target_compile_options(sasd_ui_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(sasd_ui_terminal PRIVATE -Wall -Wextra -Wpedantic)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_core PRIVATE -Werror)
        target_compile_options(sasd_ui_terminal PRIVATE -Werror)
    endif()
endif()

if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(sasd_ui_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_core PRIVATE -fsanitize=address,undefined)

    target_compile_options(sasd_ui_terminal PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_terminal PRIVATE -fsanitize=address,undefined)
endif()

if(SASD_UI_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(SASD_UI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
