add_executable(debugger_cli
    src/main.cpp
)

target_include_directories(debugger_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(debugger_cli PRIVATE
    debugger_core
    readline
)

target_compile_options(debugger_cli PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
