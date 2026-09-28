# Release-source hygiene check.
#
# This script intentionally uses Git's tracked-file list rather than scanning the working directory.
# A CI build naturally creates untracked build trees; the release concern is whether generated
# binaries/caches have accidentally become part of the versioned source package.

if(NOT DEFINED SASD_UI_SOURCE_DIR OR SASD_UI_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "SASD_UI_SOURCE_DIR must point at the repository root.")
endif()

find_program(GIT_EXECUTABLE git REQUIRED)

execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${SASD_UI_SOURCE_DIR}" ls-files
    RESULT_VARIABLE git_result
    OUTPUT_VARIABLE tracked_files_raw
    ERROR_VARIABLE git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT git_result EQUAL 0)
    message(FATAL_ERROR "git ls-files failed: ${git_error}")
endif()

string(REPLACE "\r\n" "\n" tracked_files_raw "${tracked_files_raw}")
string(REPLACE "\r" "\n" tracked_files_raw "${tracked_files_raw}")
string(REPLACE "\n" ";" tracked_files "${tracked_files_raw}")

set(forbidden_tracked_files)

foreach(path IN LISTS tracked_files)
    if(path STREQUAL "")
        continue()
    endif()

    # Build directories and CMake runtime state never belong in a source release.
    if(path MATCHES "(^|/)(build|build-[^/]*|cmake-build-[^/]*|CMakeFiles|Testing)(/|$)" OR
       path MATCHES "(^|/)CMakeCache\\.txt$")
        list(APPEND forbidden_tracked_files "${path}")
        continue()
    endif()

    # Likewise reject common native build/linker outputs. Source fixtures with one of these suffixes
    # would need an explicit policy exception instead of silently weakening this release check.
    if(path MATCHES "\\.(o|obj|a|lib|dll|so|dylib|exe|pdb|ilk|exp)$")
        list(APPEND forbidden_tracked_files "${path}")
    endif()
endforeach()

if(forbidden_tracked_files)
    list(JOIN forbidden_tracked_files "\n  " formatted)
    message(FATAL_ERROR
        "Generated/build artifacts are tracked and would contaminate the release source tree:\n"
        "  ${formatted}")
endif()

message(STATUS "Release source-tree hygiene: PASS")
