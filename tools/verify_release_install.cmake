# Verify the staged install produced by a Release build.
#
# The external package-consumer test validates target usability. This script complements it by
# checking release contents that a linker test would not notice, such as documentation/license files
# and the presence of Core, Terminal and Rendered libraries.

if(NOT DEFINED SASD_UI_INSTALL_PREFIX OR SASD_UI_INSTALL_PREFIX STREQUAL "")
    message(FATAL_ERROR "SASD_UI_INSTALL_PREFIX must point at the staged installation.")
endif()

cmake_path(NORMAL_PATH SASD_UI_INSTALL_PREFIX OUTPUT_VARIABLE install_prefix)

set(required_files
    "include/sasd/ui/application.hpp"
    "include/sasd/ui/button.hpp"
    "include/sasd/ui/terminal/terminal_backend.hpp"
    "include/sasd/ui/terminal/screen_buffer.hpp"
    "include/sasd/ui/rendered/display_list.hpp"
    "include/sasd/ui/rendered/rendered_measurement_context.hpp"
    "include/sasd/ui/rendered/rendered_presentation_sink.hpp"
    "lib/cmake/SASDUIToolkit/SASDUIToolkitConfig.cmake"
    "lib/cmake/SASDUIToolkit/SASDUIToolkitConfigVersion.cmake"
    "lib/cmake/SASDUIToolkit/SASDUIToolkitTargets.cmake"
    "share/doc/SASD_UI_Toolkit/LICENSE"
    "share/doc/SASD_UI_Toolkit/README.md"
    "share/doc/SASD_UI_Toolkit/CHANGELOG.md"
)

foreach(relative_path IN LISTS required_files)
    set(candidate "${install_prefix}/${relative_path}")
    if(NOT EXISTS "${candidate}")
        message(FATAL_ERROR "Required installed file is missing: ${candidate}")
    endif()
endforeach()

# Static-library suffixes differ across toolchains (.a on Unix-like systems, .lib with MSVC).
# Match by target basename rather than encoding platform assumptions into the release contract.
file(GLOB core_libraries LIST_DIRECTORIES FALSE
    "${install_prefix}/lib/*sasd_ui_core*"
)
file(GLOB terminal_libraries LIST_DIRECTORIES FALSE
    "${install_prefix}/lib/*sasd_ui_terminal*"
)
file(GLOB rendered_libraries LIST_DIRECTORIES FALSE
    "${install_prefix}/lib/*sasd_ui_rendered*"
)

if(NOT core_libraries)
    message(FATAL_ERROR "Installed Core library was not found under ${install_prefix}/lib.")
endif()

if(NOT terminal_libraries)
    message(FATAL_ERROR "Installed Terminal library was not found under ${install_prefix}/lib.")
endif()

if(NOT rendered_libraries)
    message(FATAL_ERROR "Installed Rendered library was not found under ${install_prefix}/lib.")
endif()

# Tests/examples are validation inputs, not part of the install surface. Catch accidental
# install-rule expansion before it becomes a published compatibility expectation.
file(GLOB_RECURSE unexpected_programs LIST_DIRECTORIES FALSE
    "${install_prefix}/bin/*sasd_ui_*test*"
    "${install_prefix}/bin/*sasd_ui_terminal_demo*"
)
if(unexpected_programs)
    list(JOIN unexpected_programs "\n  " formatted)
    message(FATAL_ERROR
        "Tests/examples unexpectedly appeared in the install tree:\n"
        "  ${formatted}")
endif()

message(STATUS "Release install contents: PASS")
message(STATUS "  Core library: ${core_libraries}")
message(STATUS "  Terminal library: ${terminal_libraries}")
message(STATUS "  Rendered library: ${rendered_libraries}")
