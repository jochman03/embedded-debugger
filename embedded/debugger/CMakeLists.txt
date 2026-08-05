set(DEBUGGER_ENABLE_LOGGING_DEFAULT OFF)

if(BUILD_TESTING)
    set(DEBUGGER_ENABLE_LOGGING_DEFAULT ON)
endif()

option(
    DEBUGGER_ENABLE_LOGGING
    "Enable Debugger component logging"
    ${DEBUGGER_ENABLE_LOGGING_DEFAULT}
)

add_library(component_debugger STATIC
    debugger.c
)

target_include_directories(component_debugger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(component_debugger PUBLIC
    c_std_11
)

target_compile_definitions(component_debugger PUBLIC
    DEBUGGER_LOGGING_ENABLED=$<BOOL:${DEBUGGER_ENABLE_LOGGING}>
)

target_link_libraries(project_components INTERFACE
    component_debugger
)

if(BUILD_TESTING)
    add_executable(test_debugger
        Tests/test_debugger.c
    )

    target_link_libraries(test_debugger PRIVATE
        component_debugger
        unity
    )

    target_compile_options(test_debugger PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    add_test(
        NAME debugger
        COMMAND test_debugger
    )
endif()
