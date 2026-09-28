cmake_minimum_required(VERSION 3.24)

project(SASD_UI_Toolkit_Package_Consumer LANGUAGES CXX)

find_package(SASDUIToolkit 0.1 CONFIG REQUIRED)

# Fail at configure time if the package ever stops exporting the public build-tree names. This catches
# packaging regressions earlier and more clearly than a later linker error.
if(NOT TARGET SASD::UI)
    message(FATAL_ERROR "Installed package does not export SASD::UI")
endif()

if(NOT TARGET SASD::UI::Terminal)
    message(FATAL_ERROR "Installed package does not export SASD::UI::Terminal")
endif()

add_executable(sasd_ui_package_consumer main.cpp)
target_link_libraries(
    sasd_ui_package_consumer
    PRIVATE
        SASD::UI
        SASD::UI::Terminal
)

enable_testing()
add_test(NAME sasd_ui_package_consumer COMMAND sasd_ui_package_consumer)
