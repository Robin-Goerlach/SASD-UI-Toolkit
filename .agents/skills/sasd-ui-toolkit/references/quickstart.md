# Quickstart: consuming SASD UI Toolkit from a CMake application

This guide targets the **currently selected source/installation**, not an assumed milestone. Confirm the current minimum CMake version in the toolkit's root `CMakeLists.txt` and project version in `project(... VERSION ...)`.

## Install a checkout into an isolated prefix

Run from the SASD UI Toolkit repository root (POSIX example):

~~~sh
cmake -S . -B build-sasd -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON -DSASD_UI_BUILD_EXAMPLES=ON
cmake --build build-sasd --parallel
ctest --test-dir build-sasd --output-on-failure
cmake --install build-sasd --prefix "$(pwd)/build-sasd/install"
~~~

This is a **local build prefix**, not a system-wide installation. The toolkit normally builds without SDL3. Use a console or terminal capable of ANSI/VT handling to run the reference demo:

~~~sh
./build-sasd/examples/sasd_ui_terminal_demo
~~~

Do not infer that the demo is a noninteractive program; use an interactive terminal for manual inspection.

## Compile the skill's independent consumer

~~~sh
cmake -S .agents/skills/sasd-ui-toolkit/examples/core-consumer \
  -B build-sasd/skill-consumer \
  -DCMAKE_PREFIX_PATH="$(pwd)/build-sasd/install"
cmake --build build-sasd/skill-consumer --parallel
ctest --test-dir build-sasd/skill-consumer --output-on-failure
~~~

The smoke consumer deliberately exercises **Core without opening a terminal**. Its `CMakeLists.txt` calls `find_package(SASDUIToolkit 0.2 CONFIG REQUIRED)` because it tests the current 0.2.x API line; update the required package version if the actual installed toolkit version changes. Do **not** remove the version check just to hide an incompatible version.

For Visual Studio / other multi-config generators, pass `--config Debug` to builds and `-C Debug` to CTest, or use the appropriate Release configuration. Executable locations also differ.

## Create a new consumer project

In the consumer's `CMakeLists.txt`:

~~~cmake
cmake_minimum_required(VERSION 3.24)
project(my_sasd_app LANGUAGES CXX)
find_package(SASDUIToolkit 0.2 CONFIG REQUIRED)
add_executable(my_sasd_app main.cpp)
target_compile_features(my_sasd_app PRIVATE cxx_std_20)
target_link_libraries(my_sasd_app PRIVATE SASD::UI::Terminal)
~~~

Choose `SASD::UI` for pure Core semantics, `SASD::UI::Terminal` for a terminal host, `SASD::UI::Rendered` for headless rendered presentation. Confirm other optional targets from the actual installed CMake export; do not assume SDL3 or native peers are universally installed.

A successful Core smoke test is not proof of a working visible terminal loop. Use the matching `examples/terminal_form_demo.cpp` for backend initialization, event routing and frames. If the toolkit cannot be installed on the consumer's machine, inspect the compilation error and the selected install prefix instead of inventing build flags.

## First failure checks

1. Wrong `CMAKE_PREFIX_PATH`, stale `build` cache or incompatible toolkit package version.
2. Wrong CMake target or missing transitive dependency.
3. A header/type unavailable in the selected version.
4. Linux/macOS/Windows terminal capability differences.
5. UI event-loop code missing despite a valid semantic widget tree.

Collect the **first useful CMake/compiler diagnostic** and the exact toolkit commit/release before attempting an API workaround.
