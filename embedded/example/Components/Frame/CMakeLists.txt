add_library(component_frame STATIC
    frame.c
)

target_include_directories(component_frame PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(component_frame PUBLIC
    c_std_11
)

target_link_libraries(project_components INTERFACE
    component_frame
)

if(BUILD_TESTING)
    add_executable(test_frame
        Tests/test_frame.c
    )

    target_link_libraries(test_frame PRIVATE
        component_frame
        unity
    )

    target_compile_options(test_frame PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    add_test(
        NAME frame
        COMMAND test_frame
    )
endif()
