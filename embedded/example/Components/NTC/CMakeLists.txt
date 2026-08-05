add_library(component_ntc STATIC
    ntc.c
)

target_include_directories(component_ntc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(component_ntc PUBLIC
    c_std_11
)

target_link_libraries(project_components INTERFACE
    component_ntc
)

target_link_libraries(component_ntc PUBLIC
    m
)

if(BUILD_TESTING)
    add_executable(test_ntc
        Tests/test_ntc.c
    )

    target_link_libraries(test_ntc PRIVATE
        component_ntc
        unity
    )

    target_compile_options(test_ntc PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    add_test(
        NAME ntc
        COMMAND test_ntc
    )
endif()
