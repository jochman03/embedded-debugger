add_library(debugger_core STATIC
    src/serial_scan.cpp
    src/serial_port.cpp
    src/protocol_parser.cpp
    src/protocol_command.cpp
    src/protocol.cpp
    src/log_type.cpp
)

target_include_directories(debugger_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_options(debugger_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
