cmake_minimum_required(VERSION 3.24)

# Standalone consumer: do not depend on internal source-tree targets.
project(SasdUiAiSkillConsumer LANGUAGES CXX)

# Version compatibility is SameMinorVersion for pre-1.0 SASD UI Toolkit.
find_package(SASDUIToolkit 0.2 CONFIG REQUIRED)

add_executable(sasd_ai_skill_core_consumer main.cpp)
target_compile_features(sasd_ai_skill_core_consumer PRIVATE cxx_std_20)
target_link_libraries(sasd_ai_skill_core_consumer PRIVATE SASD::UI)

enable_testing()
add_test(NAME sasd_ai_skill_core_consumer COMMAND sasd_ai_skill_core_consumer)
