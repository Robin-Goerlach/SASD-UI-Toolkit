cmake_minimum_required(VERSION 3.24)

project(SASD_UI_Toolkit
    VERSION 0.1.0
    DESCRIPTION "Cross-platform C++ UI toolkit for desktop and terminal applications"
    LANGUAGES CXX)

option(SASD_UI_BUILD_TESTS "Build SASD UI Toolkit tests" ON)
option(SASD_UI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SASD_UI_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer where supported" OFF)

add_library(sasd_ui_core
    src/application.cpp
    src/container.cpp
    src/event_queue.cpp
    src/testing/mock_backend.cpp
)
add_library(SASD::UI ALIAS sasd_ui_core)

target_compile_features(sasd_ui_core PUBLIC cxx_std_20)
target_include_directories(sasd_ui_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(sasd_ui_core PRIVATE /W4 /permissive-)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_core PRIVATE /WX)
    endif()
else()
    target_compile_options(sasd_ui_core PRIVATE -Wall -Wextra -Wpedantic)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_core PRIVATE -Werror)
    endif()
endif()

if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(sasd_ui_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_core PRIVATE -fsanitize=address,undefined)
endif()

if(SASD_UI_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
