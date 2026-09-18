add_executable(sasd_ui_tests
    test_main.cpp
    geometry_tests.cpp
    component_tests.cpp
    event_queue_tests.cpp
    event_dispatcher_tests.cpp
    focus_manager_tests.cpp
    layout_tests.cpp
    visual_invalidation_tests.cpp
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
