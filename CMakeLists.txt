cmake_minimum_required(VERSION 3.24)

project(SASD_UI_Toolkit
    VERSION 0.2.0
    DESCRIPTION "Cross-platform C++ UI toolkit for desktop and terminal applications"
    LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(SASD_UI_BUILD_TESTS "Build SASD UI Toolkit tests" ON)
option(SASD_UI_BUILD_EXAMPLES "Build SASD UI Toolkit example applications" ON)
option(SASD_UI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SASD_UI_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer where supported" OFF)
option(SASD_UI_BUILD_SDL3_ADAPTER "Build the experimental SDL3 rendered adapter" OFF)
option(SASD_UI_FETCH_SDL3 "Fetch pinned SDL3/SDL3_ttf sources for the experimental adapter" OFF)
set(SASD_UI_SDL3_TEST_FONT "" CACHE FILEPATH
    "Font file used by SDL3 adapter tests; leave empty to skip those tests")

add_library(sasd_ui_core
    src/application.cpp
    src/box_layout.cpp
    src/button.cpp
    src/check_box.cpp
    src/combo_box.cpp
    src/radio_button.cpp
    src/radio_group.cpp
    src/radio_group_navigation.cpp
    src/container.cpp
    src/event_dispatcher.cpp
    src/event_queue.cpp
    src/focus_manager.cpp
    src/focus_traversal.cpp
    src/form_layout.cpp
    src/grid_layout.cpp
    src/hit_test.cpp
    src/label.cpp
    src/list_model.cpp
    src/list_selection_model.cpp
    src/list_view.cpp
    src/text_field.cpp
    src/utf8.cpp
    src/pointer_router.cpp
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

# M3 starts with a deterministic rendered command layer that has no SDL3 dependency. A future
# SDL3 adapter consumes this target instead of exposing SDL handles/types through the Core API.
add_library(sasd_ui_rendered
    src/rendered/combo_box_popup_presentation.cpp
    src/rendered/display_list.cpp
    src/rendered/display_list_executor.cpp
    src/rendered/menu_bar_presentation.cpp
    src/rendered/menu_frame_presentation.cpp
    src/rendered/menu_presentation.cpp
    src/rendered/rendered_geometry.cpp
    src/rendered/rendered_presentation_sink.cpp
    src/rendered/rendered_text_field_hit_test.cpp
    src/rendered/rendered_text_field_viewport.cpp
)
add_library(SASD::UI::Rendered ALIAS sasd_ui_rendered)
target_link_libraries(sasd_ui_rendered PUBLIC SASD::UI)

# The first concrete M3 adapter is intentionally optional and build-tree-only. It proves that the
# generic DisplayList/RenderDevice contract can drive a real SDL3 renderer and SDL_ttf metrics before
# we commit to desktop-window/event-loop packaging. Keeping it out of the normal install/export set
# also prevents pre-release SDL dependency policy from becoming an accidental public guarantee.
if(SASD_UI_BUILD_SDL3_ADAPTER)
    if(SASD_UI_FETCH_SDL3)
        include(FetchContent)

        # Pin known release tags so CI and local adapter experiments do not silently change behavior
        # when upstream main branches move. SDL install/tests/examples are disabled because these
        # sources are implementation dependencies of this build, not part of our package surface.
        #
        # The fetched dependencies are always static implementation details, while SASD itself still
        # respects a parent BUILD_SHARED_LIBS policy. If that parent requests shared SASD libraries,
        # these static objects may be linked into sasd_ui_rendered_sdl3.so/dylib. Build them as PIC
        # from the start instead of forcing the SASD adapter static merely to satisfy its dependencies.
        #
        # Keep this as a directory-scope normal variable rather than a cache override. Target PIC is
        # initialized when SDL/SDL_ttf create their static targets; after both dependencies exist we
        # restore the caller-visible effective value for all later SASD targets.
        set(_sasd_ui_pic_was_defined FALSE)
        if(DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
            set(_sasd_ui_pic_was_defined TRUE)
            set(_sasd_ui_saved_pic "${CMAKE_POSITION_INDEPENDENT_CODE}")
        endif()
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)

        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            sasd_sdl3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.4.16
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(sasd_sdl3)

        # SDL_ttf is kept static and non-installing. HarfBuzz stays enabled because ADR 0028 requires
        # the caret/measurement side to respect shaping context. PlutoSVG/color-emoji support is not
        # needed for this first software-device slice and would add unrelated dependencies.
        #
        # SDL_ttf 3.2.2 intentionally uses CMake's generic BUILD_SHARED_LIBS switch instead of a
        # SDLTTF-specific static/shared option. That makes dependency isolation slightly unusual:
        # both normal-variable lookup (CMP0077) and cache writes performed by nested projects have to
        # be contained. SASD may be embedded with add_subdirectory(), so its dependency choice must
        # never become the parent project's linkage policy.
        set(_sasd_ui_build_shared_libs_was_defined FALSE)
        if(DEFINED BUILD_SHARED_LIBS)
            set(_sasd_ui_build_shared_libs_was_defined TRUE)
            set(_sasd_ui_saved_build_shared_libs "${BUILD_SHARED_LIBS}")
        endif()

        set(_sasd_ui_build_shared_libs_cache_was_defined FALSE)
        if(DEFINED CACHE{BUILD_SHARED_LIBS})
            set(_sasd_ui_build_shared_libs_cache_was_defined TRUE)
            get_property(
                _sasd_ui_saved_build_shared_libs_cache
                CACHE BUILD_SHARED_LIBS PROPERTY VALUE)
        endif()

        # Supply OFF through both lookup paths while configuring SDL_ttf. The CACHE write is
        # intentionally temporary and is always undone immediately after FetchContent returns.
        set(BUILD_SHARED_LIBS OFF)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL
            "Temporary SASD SDL_ttf dependency linkage policy" FORCE)

        set(SDLTTF_INSTALL OFF CACHE BOOL "" FORCE)
        set(SDLTTF_VENDORED OFF CACHE BOOL "" FORCE)
        set(SDLTTF_HARFBUZZ ON CACHE BOOL "" FORCE)
        set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)
        set(SDLTTF_SAMPLES OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            sasd_sdl3_ttf
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
            GIT_TAG release-3.2.2
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(sasd_sdl3_ttf)

        # Restore the caller-visible cache first. BUILD_SHARED_LIBS is conventionally a BOOL option;
        # preserving its value is the important contract. If the caller had no cache entry at all,
        # remove the temporary one instead of leaving SASD's dependency policy behind.
        if(_sasd_ui_build_shared_libs_cache_was_defined)
            set(BUILD_SHARED_LIBS "${_sasd_ui_saved_build_shared_libs_cache}" CACHE BOOL
                "Build libraries as shared libraries" FORCE)
        else()
            unset(BUILD_SHARED_LIBS CACHE)
        endif()

        # Restore the effective directory-scope policy used by SASD targets declared below. This also
        # preserves a parent normal variable that intentionally shadows a different cache value.
        if(_sasd_ui_build_shared_libs_was_defined)
            set(BUILD_SHARED_LIBS "${_sasd_ui_saved_build_shared_libs}")
        else()
            unset(BUILD_SHARED_LIBS)
        endif()

        unset(_sasd_ui_saved_build_shared_libs_cache)
        unset(_sasd_ui_build_shared_libs_cache_was_defined)
        unset(_sasd_ui_saved_build_shared_libs)
        unset(_sasd_ui_build_shared_libs_was_defined)

        # The dependency targets have already captured the PIC setting. Restore the surrounding
        # directory so SASD does not impose position-independent-code policy on unrelated targets in
        # an embedding project.
        if(_sasd_ui_pic_was_defined)
            set(CMAKE_POSITION_INDEPENDENT_CODE "${_sasd_ui_saved_pic}")
        else()
            unset(CMAKE_POSITION_INDEPENDENT_CODE)
        endif()
        unset(_sasd_ui_saved_pic)
        unset(_sasd_ui_pic_was_defined)
    else()
        find_package(SDL3 3.2 REQUIRED CONFIG)
        find_package(SDL3_ttf 3.2 REQUIRED CONFIG)
    endif()

    add_library(sasd_ui_rendered_sdl3
        src/rendered/sdl3/sdl3_pointer_event_translation.cpp
        src/rendered/sdl3/sdl3_render_context.cpp
        src/rendered/sdl3/sdl3_software_device.cpp
        src/rendered/sdl3/sdl3_window_backend.cpp
    )
    add_library(SASD::UI::Rendered::SDL3 ALIAS sasd_ui_rendered_sdl3)

    target_link_libraries(sasd_ui_rendered_sdl3
        PUBLIC SASD::UI::Rendered
        PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf
    )
    target_compile_features(sasd_ui_rendered_sdl3 PUBLIC cxx_std_20)

    # The adapter header intentionally remains a build-tree experiment for now. Exposing src/ only on
    # this target lets examples/tests consume it without installing SDL-specific API prematurely.
    target_include_directories(sasd_ui_rendered_sdl3
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    )

    if(MSVC)
        target_compile_options(sasd_ui_rendered_sdl3 PRIVATE /W4 /permissive-)
        if(SASD_UI_WARNINGS_AS_ERRORS)
            target_compile_options(sasd_ui_rendered_sdl3 PRIVATE /WX)
        endif()
    else()
        target_compile_options(sasd_ui_rendered_sdl3 PRIVATE -Wall -Wextra -Wpedantic)
        if(SASD_UI_WARNINGS_AS_ERRORS)
            target_compile_options(sasd_ui_rendered_sdl3 PRIVATE -Werror)
        endif()
    endif()

    if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
        target_compile_options(sasd_ui_rendered_sdl3
            PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(sasd_ui_rendered_sdl3 PRIVATE -fsanitize=address,undefined)
    endif()
endif()

# Preserve the same public target names for both build-tree and installed-package consumers.
# EXPORT_NAME may contain "::" because exported targets are imported targets when consumed.
set_target_properties(sasd_ui_core PROPERTIES EXPORT_NAME UI)
set_target_properties(sasd_ui_terminal PROPERTIES EXPORT_NAME "UI::Terminal")
set_target_properties(sasd_ui_rendered PROPERTIES EXPORT_NAME "UI::Rendered")

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

target_compile_features(sasd_ui_rendered PUBLIC cxx_std_20)
target_include_directories(sasd_ui_rendered
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(sasd_ui_core PRIVATE /W4 /permissive-)
    target_compile_options(sasd_ui_terminal PRIVATE /W4 /permissive-)
    target_compile_options(sasd_ui_rendered PRIVATE /W4 /permissive-)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_core PRIVATE /WX)
        target_compile_options(sasd_ui_terminal PRIVATE /WX)
        target_compile_options(sasd_ui_rendered PRIVATE /WX)
    endif()
else()
    target_compile_options(sasd_ui_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(sasd_ui_terminal PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(sasd_ui_rendered PRIVATE -Wall -Wextra -Wpedantic)
    if(SASD_UI_WARNINGS_AS_ERRORS)
        target_compile_options(sasd_ui_core PRIVATE -Werror)
        target_compile_options(sasd_ui_terminal PRIVATE -Werror)
        target_compile_options(sasd_ui_rendered PRIVATE -Werror)
    endif()
endif()

if(SASD_UI_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(sasd_ui_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_core PRIVATE -fsanitize=address,undefined)

    target_compile_options(sasd_ui_terminal PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_terminal PRIVATE -fsanitize=address,undefined)

    target_compile_options(sasd_ui_rendered PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sasd_ui_rendered PRIVATE -fsanitize=address,undefined)
endif()

if(SASD_UI_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(SASD_UI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Pre-1.0 releases are intended to be consumable through normal CMake package discovery rather than
# requiring applications to copy source files or depend on this repository layout. Keep installation metadata
# next to the targets so future backends can join the same export set without changing consumers.
set(SASD_UI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/SASDUIToolkit")

install(
    TARGETS sasd_ui_core sasd_ui_terminal sasd_ui_rendered
    EXPORT SASDUIToolkitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Public headers intentionally mirror the source include/ tree. No backend-private OS headers are
# installed from src/, which keeps termios/Win32 implementation details outside the public package.
install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

configure_package_config_file(
    cmake/SASDUIToolkitConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/SASDUIToolkitConfig.cmake"
    INSTALL_DESTINATION "${SASD_UI_INSTALL_CMAKEDIR}"
)

# Pre-1.0 source compatibility is intentionally not promised across feature releases. SameMinorVersion
# therefore keeps consumers within one feature line (for example 0.2.x for a 0.2 request).
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SASDUIToolkitConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion
)

install(
    EXPORT SASDUIToolkitTargets
    FILE SASDUIToolkitTargets.cmake
    NAMESPACE SASD::
    DESTINATION "${SASD_UI_INSTALL_CMAKEDIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/SASDUIToolkitConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/SASDUIToolkitConfigVersion.cmake"
    DESTINATION "${SASD_UI_INSTALL_CMAKEDIR}"
)

install(
    FILES LICENSE README.md CHANGELOG.md
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)
