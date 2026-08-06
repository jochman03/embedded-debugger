add_executable(debugger_cli
    src/main.cpp
)

target_link_libraries(debugger_cli PRIVATE
    debugger_core
)

target_compile_options(debugger_cli PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
